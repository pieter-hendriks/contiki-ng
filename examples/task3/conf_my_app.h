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

/* Put all cells on the same slotframe */
#define APP_SLOTFRAME_HANDLE 1

// See https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-TSCH-and-6TiSCH for relevant docs
// Has configuration macros etc


#endif