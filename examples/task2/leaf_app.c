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
#define LOG_MODULE "LEAF_APP"
#define LOG_LEVEL LOG_LEVEL_INFO


PROCESS(leaf_app, "LEAF_APP");
static linkaddr_t receiver_addr = {{ 0x00,0x12,0x4b,0x00,0x19,0x32,0xe4,0x89 }};

void resetInterfaces() {
	NETSTACK_RADIO.off();
	NETSTACK_MAC.off();
	NETSTACK_RADIO.init();
	NETSTACK_RADIO.on();
	NETSTACK_MAC.init();
	NETSTACK_MAC.on();
	NETSTACK_NETWORK.init();
}

void logEnergy() {
	LOG_INFO("Energy used by app:\n");
	LOG_INFO("CPU: %"PRIu64"\n", energest_type_time(ENERGEST_TYPE_CPU));
	LOG_INFO("CPU LPM: %"PRIu64"\n", energest_type_time(ENERGEST_TYPE_CPU));
	LOG_INFO("CPU Deep LPM: %"PRIu64"\n", energest_type_time(ENERGEST_TYPE_DEEP_LPM));
	LOG_INFO("TX: %"PRIu64"\n", energest_type_time(ENERGEST_TYPE_TRANSMIT))
	LOG_INFO("RX: %"PRIu64"\n", energest_type_time(ENERGEST_TYPE_LISTEN))
	
}

void logTime(rtimer_clock_t start, rtimer_clock_t end) {
	LOG_INFO("Time elapsed: %u ticks , or %u seconds\n", end - start, 1.0 * (end - start) / RTIMER_ARCH_SECOND);
}

PROCESS_THREAD(leaf_app, ev, data)
{
	PROCESS_BEGIN();
	// Leaf node in the task2 scenario. 
	{
		// Configure as not the TSCH coordinator.
		tsch_set_coordinator(0);
		// Reset energest, and initialize all layers.
		energest_init();
		NETSTACK_RADIO.init();
		NETSTACK_NETWORK.init();
	}
	{
		static rtimer_clock_t startTime, endTime;
		static struct etimer periodic_timer;

		for (int i = 0; i < 50; ++i) {
			// Run 50 trials so we can average
			// Re-initialize the MAC layer, should provide uniform measurements.
			NETSTACK_MAC.init();

			// Record the starting to look for network time
			startTime = rtimer_arch_now();
			// Reset energest recordings 
			energest_flush();
			energest_init();
			while (!tsch_is_associated()) {
				// Check association sixteen times per second
				etimer_set(&periodic_timer, CLOCK_SECOND / 16); 
				PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
			}
			endTime = rtimer_arch_now();
			
			logEnergy();
			logTime(startTime, endTime);

			tsch_disassociate();
		}
	}


AUTOSTART_PROCESSES(&leaf_app);