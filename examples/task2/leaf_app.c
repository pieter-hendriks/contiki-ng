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

static int filehandle = -1;

//static linkaddr_t receiver_addr = {{ 0x00,0x12,0x4b,0x00,0x19,0x32,0xe4,0x89 }};

void removeOldFiles() {
	cfs_remove(MYCPUFILENAME);
	cfs_remove(MYLPMFILENAME);
	cfs_remove(MYDEEPSLEEPFILENAME);
	cfs_remove(MYTXFILENAME);
	cfs_remove(MYRXFILENAME);
	cfs_remove(MYTICKSFILENAME);
	cfs_remove(MYSECONDSFILENAME);

}
void resetInterfaces() {
	NETSTACK_RADIO.off();
	NETSTACK_MAC.off();
	NETSTACK_RADIO.init();
	NETSTACK_RADIO.on();
	NETSTACK_MAC.init();
	NETSTACK_MAC.on();
	NETSTACK_NETWORK.init();
}

void logToFile(char* filename, uint64_t value) 
{
	filehandle = cfs_open(filename, CFS_WRITE | CFS_APPEND);
	char buffer[128] = {0};
	int len = snprintf(buffer, 128, "%"PRIu64", ", value);
	LOG_INFO("Writing ");
	LOG_INFO_(buffer);
	LOG_INFO_(" to ");
	LOG_INFO_(filename);
	LOG_INFO_(" (length=%i)", len);
	LOG_INFO_("\n");
	cfs_write(filehandle, buffer, len);
	cfs_close(filehandle);
	filehandle = -1;
}

void outputFileContents(char* filename) {
	filehandle = cfs_open(filename, CFS_READ);
	char* buffer = calloc(128, 1);
	int read = cfs_read(filehandle, buffer, 128);
	LOG_INFO("Reading ");
	LOG_INFO_(filename);
	LOG_INFO_(". Output below.\n--------------------------------------------\n");
	while (read == 128) {
		LOG_INFO_(&(buffer[0]));
		read = cfs_read(filehandle, buffer, 128);
	}
	LOG_INFO_(buffer);
	LOG_INFO_("\n--------------------------------------------\n");
	free(buffer);
	cfs_close(filehandle);
	filehandle = -1;
}

void outputAllFiles() {
	LOG_INFO("Outputting recorded energy uses!\n");
	outputFileContents(MYCPUFILENAME);
	outputFileContents(MYLPMFILENAME);
	outputFileContents(MYDEEPSLEEPFILENAME);
	outputFileContents(MYTXFILENAME);
	outputFileContents(MYRXFILENAME);
	LOG_INFO("Outputting recorded times!\n");
	outputFileContents(MYTICKSFILENAME);
	outputFileContents(MYSECONDSFILENAME);
}


void logEnergy() {
	logToFile(MYCPUFILENAME, energest_type_time(ENERGEST_TYPE_CPU));
	logToFile(MYLPMFILENAME, energest_type_time(ENERGEST_TYPE_LPM));
	logToFile(MYDEEPSLEEPFILENAME, energest_type_time(ENERGEST_TYPE_DEEP_LPM));
	logToFile(MYTXFILENAME, energest_type_time(ENERGEST_TYPE_TRANSMIT));
	logToFile(MYRXFILENAME, energest_type_time(ENERGEST_TYPE_LISTEN));
}

void logTime(rtimer_clock_t start, rtimer_clock_t end) {
	logToFile(MYTICKSFILENAME, (uint64_t)end - start);
	logToFile(MYSECONDSFILENAME, (uint64_t) ((1.0 * end - start) / RTIMER_ARCH_SECOND));
}

PROCESS(leaf_app, "leaf_app");
PROCESS_THREAD(leaf_app, ev, data)
{
	PROCESS_BEGIN();
	removeOldFiles();
	// Leaf node in the task2 scenario. 
	{
		static rtimer_clock_t startTime, endTime;
		static struct etimer periodic_timer;
		static uint8_t i;
		for (i = 0; i < 2; ++i) {
			// Run 100 trials so we can average
			{
				// Configure as not the TSCH coordinator.
				tsch_set_coordinator(0);
				// Reset energest, and initialize all layers.
				energest_init();
				NETSTACK_RADIO.init();
				NETSTACK_NETWORK.init();
				NETSTACK_MAC.init();
			}
			LOG_INFO("One iteration performed! Currently at %u of 100\n", i);

			// Record the starting to look for network time-
			startTime = rtimer_arch_now();
			// Reset energest recordings 
			energest_flush();
			energest_init();
			while (!tsch_is_associated) {
				LOG_INFO("Not yet associated!\n");
				// Check association 4 times per second
				etimer_set(&periodic_timer, CLOCK_SECOND / 4); 
				PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
			}
			LOG_INFO("Associated!\n");
			endTime = rtimer_arch_now();
			
			logEnergy();
			logTime(startTime, endTime);

			tsch_disassociate();

		}
	}
	LOG_INFO("End of the line! Outputting all data:\n");
	outputAllFiles();


	PROCESS_END();
}


AUTOSTART_PROCESSES(&leaf_app);