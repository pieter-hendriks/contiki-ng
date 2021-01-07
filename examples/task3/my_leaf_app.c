#include "conf_my_app.h"
#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "os/net/routing/routing.h"
#include <string.h>
#include <stdio.h> /* For printf() */
#include <stdlib.h> // malloc
#include "inttypes.h"
#include "power_logging.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/tsch-schedule.h"
#include "net/mac/tsch/tsch-types.h"
#include "os/sys/platform.h"
#include "os/net/packetbuf.h"
#include "os/storage/cfs/cfs-coffee.h"
#include "os/dev/radio.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "LEAF"
#define LOG_LEVEL LOG_LEVEL_INFO

#include "filewrite.h"

/* Configuration */
#define SEND_INTERVAL CLOCK_SECOND

// Define the runtime, coordinator will run longer to allow attachment etc.
// We start counting non-coord runtime from when attachment occurs.
#define RUNTIME CLOCK_SECOND * 60
#define SUFFIX "_leaf"


/*	
Output power options, per arch/cpu/cc2538/dev/cc2538-rf.c
static const output_config_t output_power[] = {
	{  7, 0xFF },
	{  5, 0xED },
	{  3, 0xD5 },
	{  1, 0xC5 },
	{  0, 0xB6 },
	{ -1, 0xB0 },
	{ -3, 0xA1 },
	{ -5, 0x91 },
	{ -7, 0x88 },
	{ -9, 0x72 },
	{-11, 0x62 },
	{-13, 0x58 },
	{-15, 0x42 },
	{-24, 0x00 },
};
*/

// Vary between 0 dBm (0xB6) and others (0 dBm easy to find energy figures for). 
#define APP_TX_POWER 0xB6

static struct etimer runtime_timer;

static linkaddr_t coordinator_addr =  {{ 0x00,0x12,0x4b,0x00,0x19,0x32,0xe2,0x61 }};
static int filehandle = -1;
/*---------------------------------------------------------------------------*/
PROCESS(leafApplication, "LeafApplication");

// Store 60 packets, record amount of sends, time of send and time of schedule. 
// 
static uint8_t numtx[60];
static uint64_t sendTime[60];
static uint64_t scheduleTimes[60];



void send_callback(void *ptr, int status, int num_tx) {
	static uint8_t count = 0;
	if (count < 60) {
		numtx[count] = num_tx;
		sendTime[count] = tsch_get_network_uptime_ticks();
	}
	if (count == 59) {
		writeFile("numtx = [", "]\n", numtx, "%u", 1, "error during numtx file write");
		writeFile("scheduleTimes = [", "]\n", scheduleTimes, "%"PRIu64, 8, "error during scheduleTime file write");
		writeFile("sendTimes = [", "]\n", scheduleTimes, "%"PRIu64, 8, "error during scheduleTime file write");
	} else if (count > 60) {
		LOG_ERR("Sent more than 60 packets!");
	}
	++count;
}

PROCESS_THREAD(leafApplication, ev, data)
{
	static struct etimer periodic_timer;
	static unsigned count = 0; // count the amount of packets we've sent
	PROCESS_BEGIN();
	LOG_INFO("Main thread starting.\n");
	{
		// Set tx power
		//radio_result_t res = NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, 7);
		//radio_result_t res = NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, 3);
		radio_result_t res = NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, -1);
		if (res != RADIO_RESULT_OK) {
			LOG_ERR("Failed to set tx power");
		} 
	}

	{
		filehandle = cfs_open(MYFILENAME, CFS_READ);
		char* readBuffer = malloc(512);
		memset(readBuffer, 0, 512);
		int ret = cfs_read(filehandle, readBuffer, 512);
		LOG_INFO_(readBuffer);
		LOG_INFO("Outputting file read:\n");
		while (ret == 512) {
			ret = cfs_read(filehandle, readBuffer, 512);
			LOG_INFO_(readBuffer);
		}
		LOG_INFO_("\n----------------------------\nFile read output above.\n");
		
		free(readBuffer);

		cfs_close(filehandle);
		filehandle = -1;
		cfs_remove(MYFILENAME);
	}

	energest_init();
	NETSTACK_MAC.init();
	NETSTACK_RADIO.init();
	NETSTACK_NETWORK.init();
	tsch_set_coordinator(0);
	etimer_set(&periodic_timer, SEND_INTERVAL);
	while(1) {
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
		LOG_INFO("Periodic timer ran out, performing one send iteration.\n");
		// Don't try to send when we don't have an associated network. 
		if (tsch_is_associated) {
			if (count == 0) {
				//Connected to network, set runtime
				etimer_set(&runtime_timer, RUNTIME);
				// Initialize our static link once we're associated
				{
					// First, the slotframe
					struct tsch_slotframe *sf = tsch_schedule_add_slotframe(APP_SLOTFRAME_HANDLE, TSCH_SCHEDULE_DEFAULT_LENGTH);
					// Create a TX dedicated cell in (1, 1) 
					tsch_schedule_add_link(sf, LINK_OPTION_TX, LINK_TYPE_NORMAL, &coordinator_addr, 1, 1, 1);
					// Print the created schedule
					tsch_schedule_print();
					LOG_INFO("Created TSCH schedule. Should be printed above.\n");
				}
				// Send an initial packet to mark start of recording.
				{
					uint8_t* myBuffer = malloc(PACKET_SIZE);
					memset(myBuffer, 0, PACKET_SIZE);
					packetbuf_copyfrom(myBuffer, PACKET_SIZE);
					packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &coordinator_addr);
					// Add our packet to TSCH send queue.
					NETSTACK_MAC.send(&send_callback, packetbuf_dataptr());
					free(myBuffer);
				}
				// reset energest after network convergence
				energest_flush();
				energest_init();
			}	else if (count < 60) {
				// Clear any previously made packets.
				packetbuf_clear();
				// Put our stuff into the packetbuf
				LOG_INFO("Scheduling packet!\n");
				uint8_t* myBuffer = malloc(PACKET_SIZE);
				uint64_t t0 = tsch_get_network_uptime_ticks();
				scheduleTimes[count] = t0;
				memset(myBuffer, (uint8_t)(count), PACKET_SIZE);

				packetbuf_copyfrom(myBuffer, PACKET_SIZE);
				packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &coordinator_addr);
				// Add our packet to TSCH send queue.
				NETSTACK_MAC.send(&send_callback, packetbuf_dataptr());

				++count;
				free(myBuffer);

			}

			if (etimer_expired(&runtime_timer)) {
				LOG_INFO("Runtime over!\n");
				break;
			}
		}
		etimer_reset(&periodic_timer);
	}
	
	{
		// Update energest values.
		energest_flush();
		char* buffer = malloc(1024);
		int ret = snprintf(buffer, 1024, "cpu_time"SUFFIX" = %"PRIu64"\ncpu_sleep_time"SUFFIX" = %"PRIu64"\ncpu_deepsleep_time"SUFFIX" = %"PRIu64"\nradio_rx_time"SUFFIX" = %"PRIu64"\nradio_tx_time"SUFFIX" = %"PRIu64"\nrtimerTicksPerSecond = %u\n", 
																energest_type_time(ENERGEST_TYPE_CPU),
																energest_type_time(ENERGEST_TYPE_LPM),
																energest_type_time(ENERGEST_TYPE_DEEP_LPM),
																energest_type_time(ENERGEST_TYPE_TRANSMIT),
																energest_type_time(ENERGEST_TYPE_LISTEN),
																RTIMER_ARCH_SECOND
											);
		if (ret < 0 || ret > 1024) {
			LOG_ERR("Failed to write power states.");
		} else {
			filehandle = cfs_open(MYFILENAME, CFS_WRITE | CFS_APPEND);
			cfs_write(filehandle, buffer, ret);
			cfs_close(filehandle);
		}

	}
	// LOG_INFO("Process ending, shutting down main thread, setting input_callback to null.\n");
	// nullnet_set_input_callback(NULL);
	nullnet_set_input_callback(NULL);
	cfs_close(filehandle);
	LOG_INFO("Process end!\n");
	// Turn off radio to save energy after we conclude one experiment run.
	tsch_disassociate();
	NETSTACK_RADIO.off();
	NETSTACK_MAC.off();
	// Seems the TSCH files do resurect the coordinator in this case. Not really my problem, though.
	LOG_INFO("Turned off radio & MAC layer.\n");
	//Re-set energest values.
	energest_init();
	PROCESS_END();
}
AUTOSTART_PROCESSES(&leafApplication);
