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

#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <string.h>
#include <stdio.h> /* For printf() */

#include "sys/energest.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

/* Configuration */
#define SEND_INTERVAL (8 * CLOCK_SECOND)

#if MAC_CONF_WITH_TSCH
#include "net/mac/tsch/tsch.h"
static linkaddr_t coordinator_addr =  {{ 0x00,0x12,0x4b,0x00,0x19,0x32,0xe1,0x69 }};
#endif /* MAC_CONF_WITH_TSCH */



/* All eneergy consumptions are in mW, data from:
   Accurate Online Energy Consumption Estimation of IoT Devices Using Energest
*/
const float vcc = 3.3;
const float wc_CPU =  0.01535 * vcc * 1000;
const float wc_LPM =  0.00959 * vcc * 1000;
const float wc_deep_LPM = 0.00258 * vcc * 1000;
const float wc_LISTEN =  0.02832 * vcc * 1000;
const float wc_Rx =  0.03014 * vcc * 1000;
const float wc_Tx =  0.03112 * vcc * 1000;

static inline unsigned long to_seconds(uint64_t time)
{
  return (unsigned long)(time / ENERGEST_SECOND);
}


/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
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
PROCESS_THREAD(nullnet_example_process, ev, data)
{
  static struct etimer periodic_timer;
  static unsigned count = 0;

  PROCESS_BEGIN();

  energest_init();
  ENERGEST_ON(ENERGEST_TYPE_CPU);
  ENERGEST_ON(ENERGEST_TYPE_LPM);
  ENERGEST_ON(ENERGEST_TYPE_DEEP_LPM);
  ENERGEST_ON(ENERGEST_TYPE_TRANSMIT);
  ENERGEST_ON(ENERGEST_TYPE_LISTEN);

#if MAC_CONF_WITH_TSCH
  tsch_set_coordinator(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr));
#endif /* MAC_CONF_WITH_TSCH */

  /* Initialize NullNet */
  nullnet_buf = (uint8_t *)&count;
  nullnet_len = sizeof(count);
  nullnet_set_input_callback(input_callback);

  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    LOG_INFO("---> Sending %u to ", count);
    LOG_INFO_LLADDR(NULL);
    LOG_INFO_("\n");
    
    memcpy(nullnet_buf, &count, sizeof(count));
    nullnet_len = sizeof(count);

    NETSTACK_NETWORK.output(NULL);
    count++;
    
    etimer_reset(&periodic_timer);

    energest_flush();

    unsigned long mj_CPU = (unsigned long)(to_seconds(energest_type_time(ENERGEST_TYPE_CPU))*wc_CPU);
    unsigned long mj_LPM = (unsigned long)(to_seconds(energest_type_time(ENERGEST_TYPE_LPM))*wc_LPM);
    unsigned long mj_deep_LPM = (unsigned long)(to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM))*wc_deep_LPM);
    unsigned long mj_Tx = (unsigned long)(to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT))*wc_Tx);
    unsigned long mj_Rx = (unsigned long)(to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN))*wc_Rx);
    LOG_INFO("ENERGEST: %lu sec\n", to_seconds(ENERGEST_GET_TOTAL_TIME()));

    LOG_INFO("\tCPU:\t%lu mJ\n", mj_CPU);
    LOG_INFO("\tLPM:\t%lu mJ\n", mj_LPM);
    LOG_INFO("\tDeep:\t%lu mJ\n", mj_deep_LPM);
    LOG_INFO("\tTx:\t%lu mJ\n", mj_Tx);
    LOG_INFO("\tRx:\t%lu mJ\n", mj_Rx);
    LOG_INFO("Total:\t%lu mJ\n", mj_CPU + mj_LPM + mj_deep_LPM + mj_Tx + mj_Rx);
  }

  PROCESS_END();
}

AUTOSTART_PROCESSES(&nullnet_example_process);