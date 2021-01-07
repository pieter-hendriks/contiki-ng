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
#define LOG_MODULE "ROOT"
#define LOG_LEVEL LOG_LEVEL_INFO

#include "filewrite.h"
static linkaddr_t sender_addr = {{ 0x00,0x12,0x4b,0x00,0x19,0x32,0xe4,0x89 }};
#define RUNTIME CLOCK_SECOND * 60
static bool hasReceived = false;
#define SUFFIX "_coordinator"
static struct etimer runtime_timer;
static int filehandle = -1;

static uint64_t rxTime[60];

void input_callback(const void *data, uint16_t len,
	const linkaddr_t *src, const linkaddr_t *dest)
{
	LOG_INFO("Receive entered\n");
	LOG_INFO("Current: %lu\n", clock_seconds());
	// Sanity check, ensure the packet is the size we expect.
	if (len == PACKET_SIZE) 
	{

		//logPowerUse();

		if (!hasReceived)
		{
			for (int i = 0; i < 60; ++i)
				rxTime[i] = 0; // 0 is before the node connects so invalid value for any received packet.

			hasReceived = true;
			energest_init();
			LOG_INFO("Received first packet - assuming network convergence, now resetting power info.\n");
			// struct tsch_slotframe* sf = tsch_schedule_get_slotframe_by_handle(APP_SLOTFRAME_HANDLE);
			// if (sf == 0) {
			// 	LOG_ERR("No tsch_slotframe with app handle, this should be impossible.");
			// }
			// sf valid, check for links now
			// Might need to implement while (link == 0) --> try different channel/timeslots, in order to avoid removing other links.
			// for param1:  * b0 = Transmit, b1 = Receive, b2 = Shared, b3 = Timekeeping, b4 = reserved
			// tsch_schedule_add_link(sf, LINK_OPTION_RX, LINK_TYPE_NORMAL, src, 1, 1, 1);
			tsch_schedule_print();
			// LOG_WARN("Setting root alive time to 65 wall seconds");
			// LOG_INFO("Current: %lu\n", clock_seconds());
			// etimer_set(&runtime_timer, CLOCK_SECOND * 65); // 60 second run time + some extra leeway
		}
		else 
		{
			uint8_t index = ((char*)(data))[0]; // Get packet index
			LOG_INFO("Received packet index %u\n", index);
			rxTime[index] = tsch_get_network_uptime_ticks();
		}
	} 
	else 
	{
		LOG_INFO("Received packet with different length (%u), from ", len);
		LOG_INFO_LLADDR(src);
		LOG_INFO_(".\n");
	}
}


PROCESS(rootApplication, "RootApplication");

PROCESS_THREAD(rootApplication, ev, data)
{
	PROCESS_BEGIN();
	LOG_INFO("Main thread starting.\n");
	{
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
	char* readBuffer = malloc(1536);
	memset(readBuffer, 0, 1536);
	int ret = cfs_read(filehandle, readBuffer, 1536);
	LOG_INFO("Outputting file read (ret=%i): \n", ret);
	while (ret == 1536) {
		LOG_INFO_(readBuffer);
		LOG_INFO_("\n");
		ret = cfs_read(filehandle, readBuffer, 1536);
	}
	LOG_INFO_(readBuffer);
	LOG_INFO_("\n----------------------------\nFile read output above.\n");
	
	free(readBuffer);

	cfs_close(filehandle);
	filehandle = -1;
	cfs_remove(MYFILENAME);
	}
	{
		rtimer_clock_t ticksPerSlot = tsch_timing[tsch_ts_timeslot_length];
		unsigned timePerSlot = tsch_timing_us[tsch_ts_timeslot_length];

		char* buffer = malloc(128);
		int ret = snprintf(buffer, 127, "slotTime = %u\nslotTicks = %lu\nticksPerSecond = %u\n", timePerSlot, ticksPerSlot, CLOCK_SECOND);
		if (ret > 0 && ret < 127) {
			filehandle = cfs_open(MYFILENAME, CFS_WRITE | CFS_APPEND);
			cfs_write(filehandle, buffer, ret);
			cfs_close(filehandle);
			filehandle = -1;
			memset(buffer, 0, 128);
			int temp = cfs_open(MYFILENAME, CFS_READ);
			cfs_read(temp, buffer, ret);
			LOG_INFO("Read just written stuff:\n");
			LOG_INFO(buffer);
		} else {
			LOG_ERR("SLOT TICK WRITE ERROR");
		}
	free(buffer);
	}
	energest_init();
	NETSTACK_MAC.init();
	NETSTACK_RADIO.init();
	NETSTACK_NETWORK.init();
	NETSTACK_ROUTING.init();
	tsch_set_coordinator(1);

	{
		// Set tsch schedule
		{
			struct tsch_slotframe *sf = tsch_schedule_add_slotframe(APP_SLOTFRAME_HANDLE, TSCH_SCHEDULE_DEFAULT_LENGTH);

			// Add advertising link to slot (0, 0)
			tsch_schedule_add_link(sf, LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED | LINK_OPTION_TIME_KEEPING, LINK_TYPE_ADVERTISING_ONLY, &tsch_broadcast_address, 0, 0, 1);
			// Add RX dedicated link to (1, 1)
			tsch_schedule_add_link(sf, LINK_OPTION_RX, LINK_TYPE_NORMAL, &sender_addr, 1, 1, 1);
		}
		//etimer_set(&runtime_timer, CLOCK_SECOND * 120);
		nullnet_set_input_callback(&input_callback);

		// Check network convergence once per second,
		while (!hasReceived) {
			etimer_set(&runtime_timer, CLOCK_SECOND);
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&runtime_timer));
		}
		// Once we're alive, stay alive for 65 seconds, then write to file.
		etimer_set(&runtime_timer, RUNTIME);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&runtime_timer));
		LOG_INFO("Coordinator wait over.\n");
		LOG_INFO("Current: %lu\n", clock_seconds());
		LOG_INFO("Writing coordinator's file:");
		
		{
			int size = 1024;
			char buffer[size];
			char* bufferptr = buffer;

			int ret = snprintf(bufferptr, size, "rxTimes = [");
			size -= ret; bufferptr += ret;
			for (int i = 0; i < 59; ++i) {
				ret = snprintf(bufferptr, size, "%"PRIu64", ", rxTime[i]);
				bufferptr += ret; size -= ret;
			}
			ret = snprintf(bufferptr, size, "%"PRIu64"]\n", rxTime[59]);
			bufferptr += ret; size -= ret;
			if (ret > 0 && size > 0) {
				filehandle = cfs_open(MYFILENAME, CFS_WRITE | CFS_APPEND);
				cfs_write(filehandle, buffer, 1024 - size);
				cfs_close(filehandle);
				filehandle = -1;
				LOG_INFO("Wrote to file: ");
				LOG_INFO_(buffer);
				LOG_INFO_("\n");
			} else {
				LOG_ERR("Write out failed.");
			}
		}
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
			filehandle = -1;
		}
		free(buffer);
	}
	// LOG_INFO("Process ending, shutting down main thread, setting input_callback to null.\n");
	// nullnet_set_input_callback(NULL);
	nullnet_set_input_callback(NULL);
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
AUTOSTART_PROCESSES(&rootApplication);
