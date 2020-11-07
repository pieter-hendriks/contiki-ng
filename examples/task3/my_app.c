/*
 * Copyright (c) 2017, RISE SICS.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         NullNet broadcast example
 * \author
*         Simon Duquennoy <simon.duquennoy@ri.se>
 *
 */
#include "conf_my_app.h"
#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <string.h>
#include <stdio.h> /* For printf() */
#include "inttypes.h"

#include "sys/energest.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

/* Configuration */
#define SEND_INTERVAL (8 * CLOCK_SECOND)

#include "net/mac/tsch/tsch.h"
// #if MAC_CONF_WITH_TSCH
static linkaddr_t coordinator_addr =  {{ 0x00,0x12,0x4b,0x00,0x19,0x32,0xe2,0x61 }};
// #endif /* MAC_CONF_WITH_TSCH */



/* All eneergy consumptions are in mW, data from:
   Accurate Online Energy Consumption Estimation of IoT Devices Using Energest
*/
#define VCC 3.3f
#define wc_CPU 0.01535f * VCC * 1000
#define wc_LPM 0.00959f * VCC * 1000
#define wc_deep_LPM 0.00258f * VCC * 1000
#define wc_LISTEN 0.02832f * VCC * 1000
#define wc_Rx 0.03014f * VCC * 1000
#define wc_Tx 0.03112f * VCC * 1000



/*---------------------------------------------------------------------------*/
PROCESS(my_app, "MyApplication");
/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
  if(len == sizeof(unsigned)) {
    unsigned count;
    memcpy(&count, data, sizeof(count));
    LOG_INFO("<--- Received %u from ", count);
    LOG_INFO_LLADDR(src);
    LOG_INFO_("\n");
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(my_app, ev, data)
{
	#if MYAPP_AS_COORDINATOR == 0
  static struct etimer periodic_timer;
	#endif
  static unsigned count = 0;

  PROCESS_BEGIN();
	tsch_set_coordinator(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr));

  /* Initialize NullNet */
  nullnet_buf = (uint8_t *)&count;
  nullnet_len = sizeof(count);
  nullnet_set_input_callback(input_callback);

	#if MYAPP_AS_COORDINATOR == 0
  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
		// Don't try to send when we don't have an associated network. 
		if (tsch_is_associated) {
			LOG_INFO("---> Sending %u to ", count);
			LOG_INFO_LLADDR(NULL);
			LOG_INFO_("\n");
			
			memcpy(nullnet_buf, &count, sizeof(count));
			nullnet_len = sizeof(count);

			NETSTACK_NETWORK.output(NULL);
			count++;
		} 
    etimer_reset(&periodic_timer);
  }
	#endif

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS(myapp_energest_record, "energest example process");
/*---------------------------------------------------------------------------*/
static inline unsigned long to_seconds(uint64_t time)
{
  return (unsigned long)(time / (double)(ENERGEST_SECOND));
}
/*---------------------------------------------------------------------------*/
/*
 * This Process will periodically print energest values for the last minute.
 *
 */
PROCESS_THREAD(myapp_energest_record, ev, data)
{
  static struct etimer periodic_timer;
  static unsigned long sec = 0;
  #define step 10
	static uint64_t results[5] = {0, 0, 0, 0, 0};
  PROCESS_BEGIN();

  etimer_set(&periodic_timer, CLOCK_SECOND * step);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    sec += step;
    etimer_reset(&periodic_timer);

    /*
     * Update all energest times. Should always be called before energest
     * times are read.
     */
    energest_flush();

		results[0] = energest_type_time(ENERGEST_TYPE_CPU)*wc_CPU;
		results[1] = energest_type_time(ENERGEST_TYPE_LPM)*wc_LPM;
		results[2] = energest_type_time(ENERGEST_TYPE_DEEP_LPM)*wc_deep_LPM;
		results[3] = energest_type_time(ENERGEST_TYPE_TRANSMIT)*wc_Tx;
		LOG_INFO("Got energest transmit time: %"PRIu64, energest_type_time(ENERGEST_TYPE_TRANSMIT));
		LOG_INFO(", which is %"PRIu64" in seconds.", energest_type_time(ENERGEST_TYPE_TRANSMIT));
		const float x = wc_Tx;
		LOG_INFO(". We then multiply that by %f, to get %f", x, energest_type_time(ENERGEST_TYPE_TRANSMIT)*x);
		results[4] = energest_type_time(ENERGEST_TYPE_LISTEN)*wc_Rx;

    LOG_INFO("ENERGEST: %lu sec\n", sec);
    LOG_INFO("\tCPU:\t%"PRIu64" mJ\n", results[0]);
    LOG_INFO("\tLPM:\t%"PRIu64" mJ\n", results[1]);
    LOG_INFO("\tDeep:\t%"PRIu64" mJ\n", results[2]);
    LOG_INFO("\tTx:\t%"PRIu64" mJ\n", results[3]);
    LOG_INFO("\tRx:\t%"PRIu64" mJ\n", results[4]);
    LOG_INFO("Total:\t%"PRIu64" mJ\n", results[0] + results[1] + results[2] + results[3] + results[4]);
  }
  PROCESS_END();
	#undef step
}

AUTOSTART_PROCESSES(&my_app,&myapp_energest_record);
