#include "conf_my_app.h"
#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "os/net/routing/routing.h"
#include <string.h>
#include <stdio.h> /* For printf() */
#include <stdlib.h> // malloc
#include "inttypes.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/tsch-schedule.h"
#include "net/mac/tsch/tsch-types.h"
#include "os/sys/platform.h"
#include "os/net/packetbuf.h"
#include "os/storage/cfs/cfs-coffee.h"
#include "os/dev/radio.h"
#include "helpers.h"
#include "sys/energest.h"
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "LEAF"
#define LOG_LEVEL LOG_LEVEL_INFO

/* Configuration */
#define SEND_INTERVAL CLOCK_SECOND

// Define the runtime, coordinator will run longer to allow attachment etc.
// We start counting non-coord runtime from when attachment occurs.
#define RUNTIME CLOCK_SECOND * 60
#define SUFFIX "_leaf"

// Helper functions (readability, mostly)
void sendInitialPacket(linkaddr_t* address);
void send_callback(void *ptr, int status, int num_tx);
void sendPacket(uint8_t index, linkaddr_t* address);
void sendNextExperimentPacket(linkaddr_t* address);
void sendFinalPacket(linkaddr_t* address);

void setTxPower(uint8_t index);

void initialize();
void finalize();

void pollMyApp();

// Helper variables
// Store PACKET_COUNT packets, record amount of sends, time of send and time of schedule. 
static uint8_t numtx[PACKET_COUNT];
static uint64_t sendTimes[PACKET_COUNT];
static uint64_t scheduleTimes[PACKET_COUNT];

// Bool to wait for over program duration.
// Lets us wait for a callback function to be called before we move on
static bool ready;

/*---------------------------------------------------------------------------*/
PROCESS(leafApplication, "LeafApplication");
PROCESS_THREAD(leafApplication, ev, data)
{
	static linkaddr_t coordinator_addr =  {{ 0x00,0x12,0x4b,0x00,0x19,0x32,0xe2,0x61 }};
	static struct etimer periodic_timer;
	static uint8_t count = 0; // count the amount of packets we've sent
	static uint8_t experimentIndex = 0;
	static uint8_t txPowerIndex = 0;
	PROCESS_BEGIN();
	LOG_INFO("Main thread starting.\n");

	outputPreviousStatistics();
	outputTimerInformation();

	// Basic setup
	initialize();
	// Set needspoll flag when we associate to the tsch network
	tsch_set_association_callback(&pollMyApp);

	// Need to set this so that we get polled even if we are already associated.
	// Since this flag is otherwise only set when we associate with the network (through callback), which would lead to indefinite wait
	leafApplication.needspoll = true; 
	LOG_INFO("Main sleeping until associated!\n");
	PROCESS_YIELD_UNTIL(tsch_is_associated);
	LOG_INFO("Associated!\n");
	// Create the TSCH schedule, featuring a single TX cell and a single advertising_only cell
	createTschSchedule(false, &coordinator_addr);
	
	for (txPowerIndex = 0; txPowerIndex < 3; ++txPowerIndex) {
		// set TX power as required (cycles default -> high -> low)
		setTxPower(txPowerIndex);
		LOG_INFO("Set TX power (#%u)!\n", txPowerIndex);
		for (experimentIndex = 0; experimentIndex < EXPERIMENT_COUNT; ++experimentIndex) {
			// undo ready mark in case it was set. 
			// Will be re-set by initialpacket callback when we're ready for new experiment.
			ready = false;
			LOG_INFO("Running experiment #%u!\n", experimentIndex);
			// This packet marks the start of functionality. 
			// Root node will also start recording when it receives this.
			sendInitialPacket(&coordinator_addr); 
			// Then start waiting and running the actual packet tests
			leafApplication.needspoll = true;
			PROCESS_YIELD_UNTIL(ready);
			ready = false;
			// Reset power information
			energest_flush();
			energest_init();
			// Set the timer
			etimer_set(&periodic_timer, CLOCK_SECOND);
			for (count = 0; count < PACKET_COUNT; ++count) {
				PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
				LOG_INFO("Sending packet #%u!\n", count);
				// Send the packet
				// This also records the statistics we need for the packets
				sendPacket(count, &coordinator_addr);
				PROCESS_YIELD_UNTIL(ready);
				ready = false;
				// Reset timer (uses same interval)
				etimer_reset(&periodic_timer);
			}
			sendNextExperimentPacket(&coordinator_addr);
			PROCESS_YIELD_UNTIL(ready);
			ready = false;
		}
	}
	// Send final packet with max tx power to get best odds of root receiving packet
	setTxPower(1); 
	sendFinalPacket(&coordinator_addr);
	PROCESS_YIELD_UNTIL(ready);
	ready = false;
	// End our run, turn off radio etc to save energy
	finalize();
	LOG_INFO("Process end!\n");
	PROCESS_END();
}
AUTOSTART_PROCESSES(&leafApplication);

void markDone() {
	ready = true;
	pollMyApp();
}

void sendPacket(uint8_t index, linkaddr_t* address) {
	// Clear any previously made packets.
	packetbuf_clear();
	// Allocate a PACKET_SIZE-sized buffer to use as packet
	// Each byte is set to current index
	uint8_t myBuffer[PACKET_SIZE] = {index};
	LOG_INFO("Sending packet with index = %u\n", index);
	// Set packetbuf from our buffer and set the address
	packetbuf_copyfrom(myBuffer, PACKET_SIZE);
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, address);
	// Add our packet to TSCH send queue.
	NETSTACK_MAC.send(&send_callback, packetbuf_dataptr());
	// Record the time we scheduled that packet
	scheduleTimes[index] = tsch_get_network_uptime_ticks();
}

void sendFinalPacket(linkaddr_t* address) {
	uint8_t myBuffer = SENTINEL_FINAL;
	packetbuf_copyfrom(&myBuffer, 1);
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, address);
	// Add our packet to TSCH send queue.
	// No callback fn
	NETSTACK_MAC.send(markDone, packetbuf_dataptr());
}

void finalize() {
	tsch_disassociate();
	NETSTACK_RADIO.off();
	NETSTACK_MAC.off();
}

void pollMyApp() {
	leafApplication.needspoll = true;
}
void initialize() {
	// Initialize energest & all parts of network stack
	energest_init();
	NETSTACK_MAC.init();
	NETSTACK_RADIO.init();
	NETSTACK_NETWORK.init();
	NETSTACK_ROUTING.init();
	// We're a leaf, not the coordinator
	tsch_set_coordinator(0);
}

void sendInitialPacket(linkaddr_t* address) {
	uint8_t myBuffer = SENTINEL_INITIAL;
	packetbuf_copyfrom(&myBuffer, 1);
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, address);
	// Add our packet to TSCH send queue.
	NETSTACK_MAC.send(&markDone, packetbuf_dataptr());
}

void sendNextExperimentPacket(linkaddr_t* address) {
	uint8_t myBuffer = SENTINEL_EXPERIMENT_DONE;
	packetbuf_copyfrom(&myBuffer, 1);
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, address);
	// Add our packet to TSCH send queue.
	NETSTACK_MAC.send(&markDone, packetbuf_dataptr());
}

void send_callback(void *ptr, int status, int num_tx) {
	// Offset the header bytes, find the value we'd actually put in there.
	char* p = (char*) ptr + 21;
	if (status == MAC_TX_OK) {
		uint8_t count;
		memcpy(&count, p, 1);
		LOG_INFO("SEND CALLBACK: COUNT = %u\n", count);
		if (count == SENTINEL_INITIAL) {
			LOG_INFO("SENTINEL_INITIAL SENT\n");
			return; 
		} else if (count == SENTINEL_FINAL) {
			LOG_INFO("SENTINEL FINAL SENT\n");
			return;
		}
		if (count <= PACKET_COUNT - 1) {
			LOG_INFO("COUNT <= %i\n", PACKET_COUNT - 1);
			// Record data
			numtx[count] = num_tx;
			sendTimes[count] = tsch_get_network_uptime_ticks();
		}
		if (count == PACKET_COUNT - 1) {
			LOG_INFO("COUNT == %i\n", PACKET_COUNT - 1);
			// output all recorded data (to file. Reboot will output to serial)
			// Allows leaf to be disconnected from computer, then connected later to obtain all stored info
			int filehandle = cfs_open(MYFILENAME, CFS_WRITE | CFS_APPEND);
			if (filehandle < 0) {
				LOG_ERR("Error in file opening. Can't write output statistics.\n");
			} else {
				writeFileUint8(filehandle, "numtx = [", "]\n", numtx, PACKET_COUNT, "error during numtx file write");
				writeFileUint64(filehandle, "scheduleTimes = [", "]\n", scheduleTimes,PACKET_COUNT, "error during scheduleTime file write");
				writeFileUint64(filehandle, "sendTimes = [", "]\n", sendTimes, PACKET_COUNT, "error during scheduleTime file write");
				recordEnergyStatistics(filehandle, SUFFIX);
			}
			cfs_close(filehandle);
			// Reset our data to avoid accidentally using same value twice in case of e.g. failure
			memset(scheduleTimes, 0, PACKET_COUNT * 8);
			memset(sendTimes, 0, PACKET_COUNT * 8);
			memset(numtx, 0, PACKET_COUNT * 1);
		}
	} else {
		LOG_ERR("Transmit failed with error code %i. num_tx = %i\n", status, num_tx);
	}
	ready = true;
	pollMyApp();
}