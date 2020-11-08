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
 * 		my_app.c
 * \author 
 * 		Pieter Hendriks <pieter.hendriks@student.uantwerpen.be>
 */
#include "conf_my_app.h"
#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <string.h>
#include <stdio.h> /* For printf() */
#include <stdlib.h> // malloc
#include "inttypes.h"
#include "power_logging.h"
#include "net/mac/tsch/tsch.h"
#include "os/sys/platform.h"
#include "os/net/packetbuf.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "MyApp"
#define LOG_LEVEL LOG_LEVEL_INFO

/* Configuration */
#define SEND_INTERVAL 8 * CLOCK_SECOND
#define PACKET_SIZE 32 // in bytes, data section
// Define the runtime, coordinator will run longer to allow attachment etc.
// We start counting non-coord runtime from when attachment occurs.
#if MYAPP_AS_COORDINATOR == 1
#define RUNTIME CLOCK_SECOND * 240
#elif MYAPP_AS_COORDINATOR == 0
#define RUNTIME CLOCK_SECOND * 120
#endif

// #if MAC_CONF_WITH_TSCH
#if MYAPP_AS_COORDINATOR == 0
static linkaddr_t coordinator_addr =  {{ 0x00,0x12,0x4b,0x00,0x19,0x32,0xe2,0x61 }};
//static linkaddr_t sender_addr = {{ 0x00,0x12,0x4b,0x00,0x19,0x32,0xe4,0x89 }};
#endif
// #endif /* MAC_CONF_WITH_TSCH */

/*---------------------------------------------------------------------------*/
PROCESS(my_app, "MyApplication");
/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
	// Sanity check, ensure the packet is the size we expect.
  // if (len == PACKET_SiZE) // Commented because was having issues with this parameter. 
	// TODO: REMOVE ABOVE COMMENT, needs to work with length check somehow.
    unsigned count;

    memcpy(&count, data, sizeof(count));
		rtimer_clock_t sentTime;
		memcpy(&sentTime, data+sizeof(count), sizeof(sentTime));
    LOG_INFO("<--- Received %u from ", count);
    LOG_INFO_LLADDR(src);
		LOG_INFO_(" at time %lu, sent at time %lu (latency is %lu)", RTIMER_NOW(), sentTime, RTIMER_NOW() - sentTime);
    LOG_INFO_("\n");
	if(len != PACKET_SIZE) {
  //} else {
		LOG_INFO("Received packet with different length (%u), from ", len);
		LOG_INFO_LLADDR(src);
		LOG_INFO_(".\n");
	}
}
/*---------------------------------------------------------------------------*/

void send_callback(void *ptr, int status, int num_tx) {
	rtimer_clock_t time = RTIMER_NOW();
	LOG_INFO("Packet actually sent (send callback) at %lu. Status = %i, num_tx = %i \n", time, status, num_tx);
}

PROCESS_THREAD(my_app, ev, data)
{
	static struct etimer shutdown_timer;
	#if MYAPP_AS_COORDINATOR == 0
  static struct etimer periodic_timer;
	static unsigned count = 0; // count the amount of packets we've sent
	#endif
  //static uint8_t packetBuffer[PACKET_SIZE];
	
  PROCESS_BEGIN();
	LOG_INFO("Main thread starting.\n");
	etimer_set(&shutdown_timer, RUNTIME);
	NETSTACK_MAC.init();
	NETSTACK_RADIO.init();
	NETSTACK_NETWORK.init();
	tsch_set_coordinator(MYAPP_AS_COORDINATOR);
	nullnet_set_input_callback(&input_callback);

#if MYAPP_AS_COORDINATOR == 0
	LOG_INFO("Not the coordinator (DEF = %u), beginning periodic sending loop.\n", MYAPP_AS_COORDINATOR);
  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
		LOG_INFO("Periodic timer ran out, performing one send iteration.\n");
		// Don't try to send when we don't have an associated network. 
		if (tsch_is_associated) {
			// Clear any previously made packets.
			packetbuf_clear();
			// Put our stuff into the packetbuf
			uint8_t* myBuffer = malloc(PACKET_SIZE);
			rtimer_clock_t t0 = RTIMER_NOW();
			memcpy(myBuffer, &count, sizeof(count));
			memcpy(myBuffer+sizeof(count), &t0, sizeof(t0));
			memset(myBuffer+sizeof(count)+sizeof(t0), 0, PACKET_SIZE-sizeof(count)-sizeof(t0));
			packetbuf_copyfrom(myBuffer, PACKET_SIZE);
			packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &coordinator_addr);
			// Add our packet to TSCH send queue.
			NETSTACK_MAC.send(&send_callback, packetbuf_dataptr());

			LOG_INFO("---> Sending %u to ", count);
			LOG_INFO_LLADDR(&coordinator_addr);
			LOG_INFO_(" at time %lu", t0);
			LOG_INFO_("\n");

			// Update packet count
			++count;
			free(myBuffer);
		} else {
			if (!tsch_is_coordinator) {
				LOG_INFO("Not sending because not associated yet.\n");
				etimer_reset(&shutdown_timer);
			} else {
				LOG_INFO("Not associated, am coordinator. Should be impossible.\n");
			}
		}
		if (etimer_expired(&shutdown_timer)) {
			break;
		}
    etimer_reset(&periodic_timer);
  }
#else
	LOG_INFO("The coordinator (DEF = %u), waiting for the shutdown timer.\n", MYAPP_AS_COORDINATOR);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&shutdown_timer));
#endif
	LOG_INFO("Shutdown timer has expired, shutting down main thread, setting input_callback to null.\n");
  nullnet_set_input_callback(NULL);
  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
PROCESS(myapp_power_logging, "Power use logging");
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*
 * This Process will periodically print power use.
 *
 */
PROCESS_THREAD(myapp_power_logging, ev, data)
{
	static struct etimer shutdown_timer;
  static struct etimer periodic_timer;
  #define step 10
  PROCESS_BEGIN();
	etimer_set(&shutdown_timer, CLOCK_SECOND * 120);
  etimer_set(&periodic_timer, CLOCK_SECOND * step);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
		logPowerUse();
		if (etimer_expired(&shutdown_timer)) {
			break;
		}
  }
	LOG_INFO("Shutdown timer has expired, shutting down power log thread.\n");
  PROCESS_END();
	#undef step
}

AUTOSTART_PROCESSES(&my_app,&myapp_power_logging);
