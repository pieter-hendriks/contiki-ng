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
#define LOG_MODULE "ROOT"
#define LOG_LEVEL LOG_LEVEL_INFO

static linkaddr_t sender_addr = {{ 0x00,0x12,0x4b,0x00,0x19,0x32,0xe4,0x89 }};
#define RUNTIME CLOCK_SECOND * 60
#define SUFFIX "_coordinator"
static uint64_t rxTime[60];

static bool done = false;
static bool experimentDone = false;

void pollMyApp();
void initialize();
void finalize();
void markNextExperiment();
void markExperimentDone();
void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest);

PROCESS(rootApplication, "RootApplication");
PROCESS_THREAD(rootApplication, ev, data)
{
	PROCESS_BEGIN();
	outputPreviousStatistics();
	outputTimerInformation();

	// Start all interfaces and configure self as coordinator
	initialize();
	// Create the tsch schedule
	createTschSchedule(true, &sender_addr);

	// Wait for everything to be over, then finalize and end
	PROCESS_YIELD_UNTIL(done);
	LOG_INFO("ENDING\n");
	finalize();
	LOG_INFO("ENDING2\n");
	PROCESS_END();
}
AUTOSTART_PROCESSES(&rootApplication);

void writeRxTimes(int filehandle) {
	writeFileUint64(filehandle, "rxTimes = [", "]\n", rxTime, PACKET_COUNT, "Failed to write RX times.");
}
void markExperimentDone() {
	int filehandle = cfs_open(MYFILENAME, CFS_WRITE | CFS_APPEND);
	recordEnergyStatistics(filehandle, SUFFIX);
	writeRxTimes(filehandle);
	cfs_close(filehandle);
	memset(rxTime, 0, 60*8);
	experimentDone = true;
}
void markFinalize() {
	if (!experimentDone) {
		markExperimentDone();
	} // if we missed experiment done packet, still save
	done = true;
	pollMyApp();
}
void markNextExperiment() {
	if (!experimentDone) {
		markExperimentDone();
	}// if we missed experiment done packet, still save
	energest_flush();
	energest_init();
}

void pollMyApp() {
	rootApplication.needspoll = true;
}
void initialize() {
	// Initialize energest & all parts of network stack
	energest_init();
	NETSTACK_MAC.init();
	NETSTACK_RADIO.init();
	NETSTACK_NETWORK.init();
	NETSTACK_ROUTING.init();
	// Mark us as the TSCH coordinator
	tsch_set_coordinator(1);
	// Configure callback for incoming packets
	nullnet_set_input_callback(&input_callback);
}

void recordData(uint8_t index) {
	if (index < PACKET_COUNT) {
		rxTime[index] = tsch_get_network_uptime_ticks();
	} else {
		LOG_ERR("RECEIVED PACKET WITH INDEX >= PACKET_COUNT\n");
	}
}

void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest)
{
	uint8_t value;
	// linkaddr_cmp returns non-zero if the same, zero if different
	if (linkaddr_cmp(src, &sender_addr) != 0) {
		LOG_INFO("Input from correct source\n");
		if (len > 2) {
			LOG_INFO("INPUT: LEN >2\n");
			// actual data packet, contains its index in every byte
			memcpy(&value, data, 1); 
			recordData(value);
			LOG_INFO("Received data packet #%u!\n", value);
			
		} else if (len == 2) { // sentinel values
			LOG_INFO("INPUT: LEN 2\n");
			// tx power packet
			memcpy(&value, ((char*)(data)) + 1, 1);
			LOG_INFO("Set tx power to #%u!\n", value);
			setTxPower(value);
		} else if (len == 1) {
			LOG_INFO("INPUT: LEN 1\n");
			// len == 1
			memcpy(&value, data, 1);
			if (value == SENTINEL_INITIAL) {
				// Mark of next experiment
				LOG_INFO("mark next experiment (%u)\n", SENTINEL_INITIAL);
				markNextExperiment();
			} else if (value == SENTINEL_FINAL) {
				// Mark end of run - can shut down
				LOG_INFO("Mark finalize (%u)\n", SENTINEL_FINAL);
				markFinalize();
			} else if (value == SENTINEL_EXPERIMENT_DONE) {
				LOG_INFO("Mark exp done (%u)\n", SENTINEL_EXPERIMENT_DONE);
				markExperimentDone();
			} else {
				LOG_INFO("STRANGE VALUE RECEIVED: %u\n", value);
			}
		} // len == 0 is also an option apparently. 
		// Those aren't packets we send intentionally though, so just gonna let it happen.
	} else {
		LOG_WARN("Received packet from unexpected source: ");
		LOG_WARN_LLADDR(src);
		LOG_WARN_("\n");
	}
}

void finalize() {
	LOG_INFO("Finalizing!!\n");
	// Stop being coordinator (ensures no wake up to start sending EBs after we finish)
	tsch_set_coordinator(0);
	// Probably overkill, but eh
	tsch_disassociate();
	// Turn off what we can and remove the callback
	NETSTACK_MAC.off();
	NETSTACK_RADIO.off();
	nullnet_set_input_callback(NULL);
	LOG_INFO("Done Finalizing!!\n");
}