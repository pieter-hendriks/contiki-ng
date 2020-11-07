#pragma once 
// Double include guard in case #pragma once not supported
#ifndef SENSORNET_INC_CONF_MY_APP_H_
#define SENSORNET_INC_CONF_MY_APP_H_

// Disable link-layer security
#define LLSEC802154_CONF_ENABLED 0
#define TSCH_CONF_JOIN_SECURED_ONLY 0 
// Check if MYAPP_AS_COORDINATOR flag has been set in makefile. 
// If not, set it to zero.
#ifndef MYAPP_AS_COORDINATOR
	#define MYAPP_AS_COORDINATOR 0
#endif


// Definitions taken from examples/6tisch/simple-node/project-conf.h
/* IEEE802.15.4 PANID */
#define IEEE802154_CONF_PANID 0xabcd
/* Do not start TSCH at init, wait for NETSTACK_MAC.on() */
#define TSCH_CONF_AUTOSTART 0
/* 6TiSCH minimal schedule length.
 * Larger values result in less frequent active slots: reduces capacity and saves energy. */
#define TSCH_SCHEDULE_CONF_DEFAULT_LENGTH 3
// Do we need this? Also in 6tisch implementation thing, so moving it over just in case.
/* USB serial takes space, free more space elsewhere */
#define SICSLOWPAN_CONF_FRAG 0
#define UIP_CONF_BUFFER_SIZE 160

//Definitions taken from examples/6tisch/custom-schedule/project-conf.h
/* Disable the 6TiSCH minimal schedule */
// Now passed through Makefile to make it be the same throughout
//#define TSCH_SCHEDULE_CONF_WITH_6TISCH_MINIMAL 0
/* Size of the application-specific schedule; a number relatively prime to the hopseq length */
#define APP_SLOTFRAME_SIZE 17


// Definitions taken from examples/nullnet/nullnet-unicast.c
// Send interval modified to 1 second, as per assignment.

/* Put all cells on the same slotframe */
#define APP_SLOTFRAME_HANDLE 1
/* Put all unicast cells on the same timeslot (for demonstration purposes only) */
#define APP_UNICAST_TIMESLOT 1

// See https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-TSCH-and-6TiSCH for relevant docs
// Has configuration macros etc


#endif