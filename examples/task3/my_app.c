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

/* Configuration */
#define SEND_INTERVAL CLOCK_SECOND
#define PACKET_SIZE 52 // in bytes, data section
// Define the runtime, coordinator will run longer to allow attachment etc.
// We start counting non-coord runtime from when attachment occurs.
#if MYAPP_AS_COORDINATOR == 1
#define RUNTIME CLOCK_SECOND * 120
static bool hasReceived = false;
#elif MYAPP_AS_COORDINATOR == 0
#define RUNTIME CLOCK_SECOND * 60
#endif

#define MYFILENAME "MYAPP_UNIVANTW_OUTPUTFILE"
#define FILE_WRITE_ERROR "Error - output too long, probably. Possibly some other issue."
#define FILE_WRITE_ERROR_SIZE 62

static struct etimer runtime_timer;

// #if MAC_CONF_WITH_TSCH
#if MYAPP_AS_COORDINATOR == 0
static linkaddr_t coordinator_addr =  {{ 0x00,0x12,0x4b,0x00,0x19,0x32,0xe2,0x61 }};
//static linkaddr_t sender_addr = {{ 0x00,0x12,0x4b,0x00,0x19,0x32,0xe4,0x89 }};
#endif
// #endif /* MAC_CONF_WITH_TSCH */
static int filehandle = -1;
/*---------------------------------------------------------------------------*/
PROCESS(my_app, "MyApplication");
/*---------------------------------------------------------------------------*/
#if MYAPP_AS_COORDINATOR == 1
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
			struct tsch_slotframe* sf = tsch_schedule_get_slotframe_by_handle(0);
			if (sf == 0) {
				LOG_ERR("No tsch_slotframe with handle 0, this should be impossible.");
			}
			// sf valid, check for links now
			// Might need to implement while (link == 0) --> try different channel/timeslots, in order to avoid removing other links.
			// for param1:  * b0 = Transmit, b1 = Receive, b2 = Shared, b3 = Timekeeping, b4 = reserved
			tsch_schedule_add_link(sf, LINK_OPTION_RX, LINK_TYPE_NORMAL, src, 1, 1, 1);
			tsch_schedule_print();
			LOG_WARN("Setting root alive time to 65 wall seconds");
			LOG_INFO("Current: %lu\n", clock_seconds());
			etimer_set(&runtime_timer, CLOCK_SECOND * 65); // 60 second run time + some extra leeway
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
#endif
/*---------------------------------------------------------------------------*/
#if MYAPP_AS_COORDINATOR == 0 
static uint8_t numtx[60];
static uint64_t sendTime[60];
static uint64_t scheduleTime[60];

void send_callback(void *ptr, int status, int num_tx) {
	static uint8_t count = -1;
	LOG_INFO("Sending! Count = %u\n", count);
	if (count != (uint8_t)(-1)) {
		numtx[count] = num_tx;
		sendTime[count] = tsch_get_network_uptime_ticks();
	}
	++count;
	if (count == 60) {

		char buffer[1024];
		char* bufferptr = buffer;
		memset(bufferptr, 0, 1024);
		int size = 1024;
		int ret = snprintf(bufferptr, size, "numtx = [");
		bufferptr += ret; size -= ret;
		for (uint8_t i = 0; i < 59; ++i) {
			// output all but last
			ret = snprintf(bufferptr, size, "%u, ", numtx[i]);
			bufferptr += ret; size -= ret;
		}
		ret = snprintf(bufferptr, size, "%u]\n", numtx[59]);
		bufferptr += ret; size -= ret;
		if (ret > 0 && size > 0) {
			filehandle = cfs_open(MYFILENAME, CFS_WRITE | CFS_APPEND);
			ret = cfs_write(filehandle, buffer, 1024-size);
			cfs_close(filehandle);
		} else {
			LOG_ERR("Failed write");
		}

		size = 1024; bufferptr = buffer;
		memset(bufferptr, 0, 1024);
		ret = snprintf(bufferptr, size, "sendTimes = [");
		bufferptr += ret; size -= ret;
		for (uint8_t i = 0; i < 59; ++i) {
			// output all but last
			ret = snprintf(bufferptr, size, "%"PRIu64", ", sendTime[i]);
			bufferptr += ret; size -= ret;
		}
		ret = snprintf(bufferptr, size, "%"PRIu64"]\n", sendTime[59]);
		bufferptr += ret; size -= ret;
		if (ret > 0 && size > 0) {
			filehandle = cfs_open(MYFILENAME, CFS_WRITE | CFS_APPEND);
			ret = cfs_write(filehandle, buffer, 1024-size);
			cfs_close(filehandle);
		} else {
			LOG_ERR("Failed write");
		}

		size = 1024; bufferptr = buffer;
		memset(bufferptr, 0, 1024);
		ret = snprintf(bufferptr, size, "scheduleTimes = [");
		bufferptr += ret; size -= ret;
		for (uint8_t i = 0; i < 59; ++i) {
			// output all but last
			ret = snprintf(bufferptr, size, "%"PRIu64", ", scheduleTime[i]);
			bufferptr += ret; size -= ret;
		}
		ret = snprintf(bufferptr, size, "%"PRIu64"]\n", scheduleTime[59]);
		bufferptr += ret; size -= ret;
		if (ret > 0 && size > 0) {
			filehandle = cfs_open(MYFILENAME, CFS_WRITE | CFS_APPEND);
			ret = cfs_write(filehandle, buffer, 1024-size);
			cfs_close(filehandle);
			uint16_t i;
			for ( i= 0; i < 2048; ++i) {
				if (buffer[i] == 0) break;
			}
		} else {
			LOG_ERR("Failed write");
		}
	} else if (count > 60) {
		LOG_ERR("Sent more than 60 packets!");
	}
}
#endif

PROCESS_THREAD(my_app, ev, data)
{
	
	#if MYAPP_AS_COORDINATOR == 0
		static struct etimer periodic_timer;
		static unsigned count = 0; // count the amount of packets we've sent
	#endif
	
	PROCESS_BEGIN();
	etimer_set(&runtime_timer, RUNTIME);
	LOG_INFO("Main thread starting.\n");
	{
	filehandle = cfs_open(MYFILENAME, CFS_READ);
	char* readBuffer = malloc(2048);
	memset(readBuffer, 0, 2048);
	int ret = cfs_read(filehandle, readBuffer, 2048);
	LOG_INFO("Outputting file read (ret=%i): \n", ret);
	while (ret == 2048) {
		LOG_INFO_(readBuffer);
		LOG_INFO_("\n");
		ret = cfs_read(filehandle, readBuffer, 2048);
	}
	LOG_INFO_(readBuffer);
	LOG_INFO_("\n----------------------------\nFile read output above.\n");
	
	free(readBuffer);

	cfs_close(filehandle);
	cfs_remove(MYFILENAME);
	}
	{
	rtimer_clock_t ticksPerSlot = tsch_timing[tsch_ts_timeslot_length];
	unsigned timePerSlot = tsch_timing_us[tsch_ts_timeslot_length];

	char* buffer = malloc(128);
	int ret = snprintf(buffer, 127, "SLOTTIME: %u, SLOTTICKS: %lu\n", timePerSlot, ticksPerSlot);
	if (ret > 0 && ret < 127) {
		filehandle = cfs_open(MYFILENAME, CFS_WRITE | CFS_APPEND);
		cfs_write(filehandle, buffer, ret);
		cfs_close(filehandle);
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
	tsch_set_coordinator(MYAPP_AS_COORDINATOR);
	#if MYAPP_AS_COORDINATOR == 0
		LOG_INFO("Not the coordinator (DEF = %u), beginning periodic sending loop.\n", MYAPP_AS_COORDINATOR);
		etimer_set(&periodic_timer, SEND_INTERVAL);
		while(1) {
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
			LOG_INFO("Periodic timer ran out, performing one send iteration.\n");
			// Don't try to send when we don't have an associated network. 
			if (tsch_is_associated) {
				if (count == 0) {
					//Connected to network, set runtime
					etimer_set(&runtime_timer, CLOCK_SECOND * 60);

					// Send an initial packet to mark start of recording.
					uint8_t* myBuffer = malloc(PACKET_SIZE);
					memset(myBuffer, 0, PACKET_SIZE);
					packetbuf_copyfrom(myBuffer, PACKET_SIZE);
					packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &coordinator_addr);
					// Add our packet to TSCH send queue.
					NETSTACK_MAC.send(&send_callback, packetbuf_dataptr());

					// reset energest after network convergence
					energest_init();
					// Initialize our static link once we're associated
					struct tsch_slotframe* sf = tsch_schedule_get_slotframe_by_handle(0);
					if (sf == 0) {
						LOG_ERR("No tsch_slotframe with handle 0, this should be impossible.");
						break;
					}
					// sf valid, check for links now
					// struct tsch_link* link = 0;
					// Might need to implement while (link == 0) --> try different channel/timeslots, in order to avoid removing other links.
					tsch_schedule_add_link(sf, LINK_OPTION_TX, LINK_TYPE_NORMAL, &tsch_broadcast_address/*&coordinator_addr*/, 1, 1, 1);
					LOG_INFO("Link added!\n");

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
					radio_result_t res = NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, 3);
					if (res != RADIO_RESULT_OK) {
						LOG_ERR("Bad tx set");
						break;
					}
				} 
				if (count < 60) {
					// Clear any previously made packets.
					packetbuf_clear();
					// Put our stuff into the packetbuf
					LOG_INFO("Scheduling packet!\n");
					uint8_t* myBuffer = malloc(PACKET_SIZE);
					uint64_t t0 = tsch_get_network_uptime_ticks();
					scheduleTime[count] = t0;
					memset(myBuffer, (uint8_t)(count), PACKET_SIZE);

					packetbuf_copyfrom(myBuffer, PACKET_SIZE);
					packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &coordinator_addr);
					// Add our packet to TSCH send queue.
					NETSTACK_MAC.send(&send_callback, packetbuf_dataptr());

					//print tsch schedule
					tsch_schedule_print();
					++count;
					free(myBuffer);
				} else {
					uint8_t size = 200;
					char* buffer = malloc(size);
					int ret = snprintf(buffer, size, "[%"PRIu64", %"PRIu64", %"PRIu64", %"PRIu64", %"PRIu64"]\n", 
																													energest_type_time(ENERGEST_TYPE_CPU), 
																													energest_type_time(ENERGEST_TYPE_LPM), 
																													energest_type_time(ENERGEST_TYPE_DEEP_LPM), 
																													energest_type_time(ENERGEST_TYPE_LISTEN), 
																													energest_type_time(ENERGEST_TYPE_TRANSMIT)
												);
					if (ret < 0 || ret > size) {
						LOG_ERR("Write error: %i\n", ret);
						LOG_ERR("%i < 0 or %i > %u\n", ret, ret, size);
					} else {
						filehandle = cfs_open(MYFILENAME, CFS_APPEND);
						cfs_write(filehandle, buffer, ret);
						cfs_close(filehandle);
					}
					free(buffer);

					if (etimer_expired(&runtime_timer))
					{
						LOG_INFO("Runtime over!\n");
						break;
					}
				}
			} else {
				if (!tsch_is_coordinator) {
					LOG_INFO("Not sending because not associated yet.\n");
					count = 0;
				} else {
					LOG_INFO("Not associated, am coordinator. Should be impossible.\n");
				}
			}
			etimer_reset(&periodic_timer);
		}
		
	#else
	{
		// Give root max send power, hope to speed up attach
		radio_result_t res = NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, 7);
		if (res != RADIO_RESULT_OK) {
			LOG_ERR("Failed to set root tx power");
		}
		etimer_set(&runtime_timer, CLOCK_SECOND * 120);
		nullnet_set_input_callback(&input_callback);
		LOG_INFO("The coordinator (DEF = %u).\n", MYAPP_AS_COORDINATOR);

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&runtime_timer));

		LOG_INFO("Coordinator wait over.\n");
		LOG_INFO("Current: %lu\n", clock_seconds());
		LOG_INFO("Writing coordinator's file:");

		int size = 1024;
		char buffer[size];
		char* bufferptr = buffer;

		int ret = snprintf(buffer, size, "rxTimes = [");
		size -= ret; bufferptr += ret;
		for (int i = 0; i < 59; ++i) {
			ret = snprintf(buffer, size, "%"PRIu64", ", rxTime[i]);
			bufferptr += ret; size -= ret;
		}
		ret = snprintf(buffer, size, "%"PRIu64"]\n", rxTime[59]);
		bufferptr += ret; size -= ret;
		if (ret > 0 && size > 0) {
			filehandle = cfs_open(MYFILENAME, CFS_WRITE | CFS_APPEND);
			cfs_write(filehandle, buffer, 1024 - size);
			cfs_close(filehandle);
			LOG_INFO("Wrote to file.");
		} else {
			LOG_ERR("Write out failed.");
		}
	}
	#endif
	// LOG_INFO("Process ending, shutting down main thread, setting input_callback to null.\n");
	// nullnet_set_input_callback(NULL);
	nullnet_set_input_callback(NULL);
	cfs_close(filehandle);
	LOG_INFO("Process end!\n");
	PROCESS_END();
}
AUTOSTART_PROCESSES(&my_app);
