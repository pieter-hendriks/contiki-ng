#pragma once 
// Double include guard in case #pragma once not supported
#ifndef SENSORNET_INC_CONF_MY_APP_H_
#define SENSORNET_INC_CONF_MY_APP_H_

// Disable link-layer security
#define LLSEC802154_CONF_ENABLED 0
#define TSCH_CONF_JOIN_SECURED_ONLY 0 

// Definitions taken from examples/6tisch/simple-node/project-conf.h
/* IEEE802.15.4 PANID */
#define IEEE802154_CONF_PANID 0xabcd

/* Put all cells on the same slotframe */
#define APP_SLOTFRAME_HANDLE 1

// See https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-TSCH-and-6TiSCH for relevant docs
// Has configuration macros etc

#define PACKET_SIZE 64 // in bytes, data section
#define MYFILENAME "MYAPP_UNIVANTW_OUTPUTFILE"
#define FILE_WRITE_ERROR "Error - output too long, probably. Possibly some other issue."
#define FILE_WRITE_ERROR_SIZE 62

#define PACKET_COUNT 60
#define EXPERIMENT_COUNT 1

// Vary between 7 dBm for high, 0 dBm for default and -7 dBm for low
#define APP_TX_POWER_DEFAULT 0
#define APP_TX_POWER_LOW -7
#define APP_TX_POWER_HIGH 7

// Define some sentinel values to mark certain events
#define SENTINEL_INITIAL 255
#define SENTINEL_FINAL 254
#define SENTINEL_EXPERIMENT_DONE 253

#endif