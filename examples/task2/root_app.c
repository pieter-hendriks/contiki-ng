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
#define LOG_MODULE "MyApp"
#define LOG_LEVEL LOG_LEVEL_INFO

void input_callback(const void *data, uint16_t len,
	const linkaddr_t *src, const linkaddr_t *dest)
{
	
}



PROCESS(root_app, "ROOT_APP");
static linkaddr_t sender_addr = {{ 0x00,0x12,0x4b,0x00,0x19,0x32,0xe4,0x89 }};

PROCESS_THREAD(root_app, ev, data)
{
	PROCESS_BEGIN();
	// Root node in the task2 scenario. 
	{
		// Set the callback for packet arrival
		nullnet_set_input_callback(&input_callback);
		// Set as TSCH coordinator.
		tsch_set_coordinator(1);
		// Reset energest, and initialize all layers.
		energest_init();
		NETSTACK_RADIO.init();
		NETSTACK_MAC.init();
		NETSTACK_NETWORK.init();
	}
	// Set tsch schedule
	{
		struct tsch_slotframe *sf = tsch_schedule_add_slotframe(1, TSCH_SCHEDULE_DEFAULT_LENGTH);
		// Add advertising link to slot (0, 0)
		tsch_schedule_add_link(sf, LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED | LINK_OPTION_TIME_KEEPING, LINK_TYPE_ADVERTISING_ONLY, &tsch_broadcast_address, 0, 0, 1);
		// Add RX dedicated link to (1, 1)
		tsch_schedule_add_link(sf, LINK_OPTION_RX, LINK_TYPE_NORMAL, &sender_addr, 1, 1, 1);
	}
	// Root doesn't need to do anything else, we just need to make sure it's broadcasting the EBs.
	PROCESS_END();
}

AUTOSTART_PROCESSES(&root_app);