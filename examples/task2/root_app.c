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
#include "helpers.h"
#define LOG_MODULE "MyApp"
#define LOG_LEVEL LOG_LEVEL_INFO

static bool done = false;

// In this task, we should consider the energy used during the connection process.
// In order to do so, the node will send a packet when it has connected, so we should record the 
// statistics when we receive that packet. Immediately after, we will reset the parameters 
// so that the next interval can be recorded.
void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest);

PROCESS(root_app, "ROOT_APP");
PROCESS_THREAD(root_app, ev, data)
{
	PROCESS_BEGIN();
	// Root node in the task2 scenario. 
	{
		// Perform interface and energy resets/setup
		doInitialSetup();
		// Set the callback for packet arrival
		nullnet_set_input_callback(&input_callback);
		// Set as TSCH coordinator.
		tsch_set_coordinator(1);
	}
	LOG_INFO("Root going to sleep!");
	// Root doesn't need to do anything else, we just need to make sure it's broadcasting the EBs.
	// And responding to any received packets.
	PROCESS_WAIT_UNTIL(done);
	PROCESS_END();
}
AUTOSTART_PROCESSES(&root_app);

void input_callback(const void *data, uint16_t len,
	const linkaddr_t *src, const linkaddr_t *dest)
{
	LOG_INFO("INCOMING PACKET RECEIVED FROM ");
	LOG_INFO_LLADDR(src);
	LOG_INFO_("\n");
	uint8_t buffer; 
	memcpy(&buffer, data, 1);
	if (buffer == 0) {
		// First iteration, simply reset our numbers and move on
		resetEnergy();
		return;
	}
	// For all other iterations, we should log information
	// Time is recorded on the leaf side, we only care about energy.
	logEnergySerial();
	if (buffer == TASK2_ITERATIONCOUNT) {
		done = true;
		root_app.needspoll = true;
	}
}