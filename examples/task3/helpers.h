#pragma once
#ifndef INC_UANTWERP_SENSORNET_HELPERS_H_
#define INC_UANTWERP_SENSORNET_HELPERS_H_
#include "os/net/linkaddr.h"
void createTschSchedule(bool coordinator, linkaddr_t* address);
void outputPreviousStatistics();
void outputPreviousStatisticsNoRemove();
void recordEnergyStatistics(int filehandle, char* suffix);
void outputTimerInformation();
void setTxPower(uint8_t index);
bool validPrintfResult(int ret, int maxSize);

void writeFileUint64(int filehandle, char* prefix, char* suffix, uint64_t* dataptr, uint8_t len, char* error_indicator);
void writeFileUint8(int filehandle, char* prefix, char* suffix, uint8_t* dataptr, uint8_t len, char* error_indicator);
#endif