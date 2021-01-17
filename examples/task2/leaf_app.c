#include "conf_my_app.h"
#include "helpers.h"
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
#include "os/storage/cfs/cfs-coffee.h"
#include "os/dev/radio.h"
/* Log configuration */
#include "sys/log.h"
#define LOG_LEVEL LOG_LEVEL_INFO
#define LOG_MODULE "SENSORNETS"
static linkaddr_t receiver_addr = {{ 0x00,0x12,0x4b,0x00,0x19,0x32,0xe4,0x89 }};

static bool sendCompleted = false;


static rtimer_clock_t startTime, endTime;
void logTime() {
	logToFile(MYTICKSFILENAME, (uint64_t) (endTime - startTime));
	logToFile(MYSECONDSFILENAME, (uint64_t) ((1.0 * endTime - startTime) / RTIMER_ARCH_SECOND + 0.5));
}

void sendPacket(uint8_t index, linkaddr_t* addr);

void send_callback(void *ptr, int status, int num_tx) {
	uint8_t buffer;
	memcpy(&buffer, ptr, 1);
	if (status != 0) {
		sendPacket(buffer, &receiver_addr);
		LOG_WARN("Re-sent marker packet for index %u. Readings may not be reliable.", buffer);
		return;
	} 
	// Only take iterations beyond the first into account
	// Root's recording on first iteration isn't valid, so we exclude the leaf's recording as well.
	if (buffer != 0) {
		endTime = rtimer_arch_now();
		// Reset energest recordings 
		logTime(startTime, endTime);
		logEnergy();
		LOG_INFO("send_callback: Recorded data.");
	} else {
		LOG_INFO("First iteration send_callback. Not recording data.");
	}
	// After send is complete, disassociate
	tsch_disassociate();
	sendCompleted = true;
}

void sendPacket(uint8_t index, linkaddr_t* addr) {
	packetbuf_clear();
	packetbuf_copyfrom(&index, 1);
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, addr);
	NETSTACK_MAC.send(&send_callback, packetbuf_dataptr());
}

PROCESS(leaf_app, "leaf_app");
PROCESS_THREAD(leaf_app, ev, data)
{
	PROCESS_BEGIN();
	doInitialSetup();
	// Leaf node in the task2 scenario. 
	{
		static struct etimer periodic_timer;
		static uint8_t i;
		// Results of the first iteration aren't valid because we use it to reset the root's power settings
		for (i = 0; i <= TASK2_ITERATIONCOUNT; ++i) {
			// Run many trials for average/reliable results
			{
				// Reset energest, and initialize all layers.
				resetEnergy();
				NETSTACK_RADIO.init();
				NETSTACK_NETWORK.init();
				NETSTACK_MAC.init();
				// Configure as leaf
				tsch_set_coordinator(0);
			}
			// Wait for association
			while (!tsch_is_associated) {
				LOG_INFO("Not yet associated!\n");
				// Check association 4 times per second
				etimer_set(&periodic_timer, CLOCK_SECOND / 4); 
				PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
			}
			LOG_INFO("Associated!\n");
			// Then send packet to root.
			sendPacket(i, &receiver_addr);
			while (!sendCompleted) {
				LOG_INFO("Iteration %u completed. Sleeping for 2 seconds to allow root to receive packet.", i);
				etimer_set(&periodic_timer, 2 * CLOCK_SECOND); 
				PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
			}
			sendCompleted = false;
			
		}
	}
	LOG_INFO("End of the line! Outputting all data:\n");
	outputAllFiles();


	PROCESS_END();
}


AUTOSTART_PROCESSES(&leaf_app);