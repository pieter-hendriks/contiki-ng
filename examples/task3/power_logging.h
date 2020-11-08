#pragma once 
// include guard, in case compiler doesn't support #pragma once
#ifndef SENSORNETS_INC_POWER_LOGGING_H_
#define SENSORNETS_INC_POWER_LOGGING_H_
#include "sys/energest.h"
#include "sys/log.h"

#define LOG_MODULE "PowerLogging"
#define LOG_LEVEL LOG_LEVEL_INFO

/* All eneergy consumptions are in mW, data from:
   Accurate Online Energy Consumption Estimation of IoT Devices Using Energest
*/

// #define VCC 3.3f
// #define wc_CPU 0.01535f * VCC * 1000
// #define wc_LPM 0.00959f * VCC * 1000
// #define wc_deep_LPM 0.00258f * VCC * 1000
// #define wc_LISTEN 0.02832f * VCC * 1000
// #define wc_Rx 0.03014f * VCC * 1000
// #define wc_Tx 0.03112f * VCC * 1000


static inline unsigned long to_seconds(uint64_t time)
{
  return (unsigned long)(time / ENERGEST_SECOND);
}

void logPowerUse(/*unsigned stepSize*/) {
	//static unsigned long sec = 0;
  //sec += stepSize;
	/*
		* Update all energest times. Should always be called before energest
		* times are read.
		*/
	energest_flush();
	// Store on-times
	uint64_t results[5] = {0, 0, 0, 0, 0};
	results[0] = energest_type_time(ENERGEST_TYPE_CPU);
	results[1] = energest_type_time(ENERGEST_TYPE_LPM);
	results[2] = energest_type_time(ENERGEST_TYPE_DEEP_LPM);
	results[3] = energest_type_time(ENERGEST_TYPE_TRANSMIT);
	results[4] = energest_type_time(ENERGEST_TYPE_LISTEN);

	//LOG_INFO("ENERGEST: %lu sec\n", sec);
	LOG_INFO("\tCPU:\t%"PRIu64" ticks\n", results[0]);
	LOG_INFO("\tLPM:\t%"PRIu64" ticks\n", results[1]);
	LOG_INFO("\tDeep:\t%"PRIu64" ticks\n", results[2]);
	LOG_INFO("\tTx:\t%"PRIu64" ticks\n", results[3]);
	LOG_INFO("\tRx:\t%"PRIu64" ticks\n", results[4]);
	LOG_INFO("Total:\t%"PRIu64" ticks\n", energest_get_total_time());
	LOG_INF("Energest ticks/second: %u", ENERGEST_SECOND);
};

#endif