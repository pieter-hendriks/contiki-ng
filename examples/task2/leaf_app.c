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
static linkaddr_t receiver_addr =  {{ 0x00,0x12,0x4b,0x00,0x19,0x32,0xe2,0x61 }};
static bool readyForNext = true;
static uint8_t loopIndex = 0;
void pollMyApp();
static rtimer_clock_t startTime, endTime;


void logTime(int filehandle) {
	logToFile(filehandle, TICKSPREFIX, (uint64_t) (endTime - startTime));
	logToFile(filehandle, SECSPREFIX, (uint64_t) ((1.0 * endTime - startTime) / RTIMER_ARCH_SECOND + 0.5));
}
void logTimeSerial() {
	logSerial(TICKSPREFIX, (uint64_t) (endTime - startTime));
	logSerial(SECSPREFIX, (uint64_t) ((1.0 * endTime - startTime) / RTIMER_ARCH_SECOND + 0.5));
}

void send_callback(void *ptr, int status, int num_tx);
void sendPacket() {
	packetbuf_clear();
	packetbuf_copyfrom(&loopIndex, 1);
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &receiver_addr);
	NETSTACK_MAC.send(&send_callback, packetbuf_dataptr());
}
void resetTime() {
	startTime = rtimer_arch_now();
	endTime = 0;
}

PROCESS(leaf_app, "leaf_app");
PROCESS_THREAD(leaf_app, ev, data)
{
	PROCESS_BEGIN();
	doInitialSetup();
	tsch_set_association_callback(&pollMyApp);
	tsch_set_disassociation_callback(&pollMyApp);
	LOG_INFO("Leaf started with channel hopping sequence size: %i\n", sizeof(TSCH_DEFAULT_HOPPING_SEQUENCE) / sizeof(TSCH_DEFAULT_HOPPING_SEQUENCE[0]));
	LOG_INFO("With EB period = %i / 128 seconds\n", TSCH_EB_PERIOD);
	// Leaf node in the task2 scenario.
	{
		// Perform one extra iteration because we discard results from the first one
		for (loopIndex = 0; loopIndex <= TASK2_ITERATIONCOUNT; ++loopIndex) {
			LOG_INFO("Leaf loop start!\n");
			// Run many trials for average/reliable results
			{
				// Reset energest, and initialize all layers.
				resetEnergy();
				resetTime();
			}
			LOG_INFO("Leaf waiting for association!\n");
			// Wait for association
			PROCESS_YIELD_UNTIL(tsch_is_associated);
			// Add the packet to the TSCH queue and wait for it to send
			sendPacket();
			LOG_INFO("Leaf packet sent!\n");
			// Send packet is run from here. Callback will be called by MAC.
			// Callback will then disassociate the TSCH network and mark readyForNext
			// which we await here and then continue
			PROCESS_YIELD_UNTIL(readyForNext);
			readyForNext = false;
		}
	}
	tsch_set_association_callback(NULL);
	tsch_set_disassociation_callback(NULL);
	PROCESS_END();
}

void send_callback(void *ptr, int status, int num_tx) {
	uint8_t buffer;
	memcpy(&buffer, ptr, 1);
	if (status != 0) {
		sendPacket();
		LOG_WARN("Re-sent marker packet for index %u. Readings may not be reliable.\n", buffer);
		return;
	}
	// Only take iterations beyond the first into account
	// Root's recording on first iteration isn't valid, so we exclude the leaf's recording as well.
	if (buffer != 0) {
		endTime = rtimer_arch_now();
		logTimeSerial();
		logEnergySerial();
	} else {
		LOG_INFO("First iteration send_callback. Not recording data.\n");
	}
	// After send is complete, disassociate
	tsch_disassociate();
	readyForNext = true;
	pollMyApp();
}

void pollMyApp() {
	leaf_app.needspoll = true;
}

AUTOSTART_PROCESSES(&leaf_app);