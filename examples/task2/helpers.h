#pragma once
#ifndef INC_UANTWERPEN_SENSORNETS_HELPERS_H_
#define INC_UANTWERPEN_SENSORNETS_HELPERS_H_
#include "conf_my_app.h"
#include "os/net/packetbuf.h"
#include "sys/energest.h"
#include "os/storage/cfs/cfs-coffee.h"
#include "sys/log.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "os/net/routing/routing.h"
#include <stdio.h>
#include <stdlib.h> 

void removeOldFiles();
void resetInterfaces();
void logToFile(int filehandle, char* prefix, uint64_t value);
void logEnergy(int filehandle);
void outputFileContents(int filehandle);
void outputEnergyFiles();
void outputTimeFiles();
void resetEnergy();
void doInitialSetup();


#endif