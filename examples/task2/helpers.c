#include "conf_my_app.h"
#include "helpers.h"
#define LOG_LEVEL LOG_LEVEL_INFO
#define LOG_MODULE "SENSORNETS"
static int filehandle = -1;
void outputFileContents(char* filename) {
	filehandle = cfs_open(filename, CFS_READ);
	char* buffer = calloc(128, 1);
	int read = cfs_read(filehandle, buffer, 128);
	LOG_INFO("Reading ");
	LOG_INFO_(filename);
	LOG_INFO_(". Output below.\n--------------------------------------------\n");
	while (read == 128) {
		LOG_INFO_(&(buffer[0]));
		read = cfs_read(filehandle, buffer, 128);
	}
	LOG_INFO_(buffer);
	LOG_INFO_("\n--------------------------------------------\n");
	free(buffer);
	cfs_close(filehandle);
	filehandle = -1;
}
void resetEnergy() {
	energest_flush();
	energest_init();
}
void logEnergy() {
	energest_flush();
	logToFile(MYCPUFILENAME, energest_type_time(ENERGEST_TYPE_CPU));
	logToFile(MYLPMFILENAME, energest_type_time(ENERGEST_TYPE_LPM));
	logToFile(MYDEEPSLEEPFILENAME, energest_type_time(ENERGEST_TYPE_DEEP_LPM));
	logToFile(MYTXFILENAME, energest_type_time(ENERGEST_TYPE_TRANSMIT));
	logToFile(MYRXFILENAME, energest_type_time(ENERGEST_TYPE_LISTEN));
}

void doInitialSetup() {
	removeOldFiles();
	resetInterfaces();
	resetEnergy();
}

void logToFile(char* filename, uint64_t value) 
{
	filehandle = cfs_open(filename, CFS_WRITE | CFS_APPEND);
	char buffer[128] = {0};
	int len = snprintf(buffer, 128, "%"PRIu64", ", value);
	LOG_INFO("Writing ");
	LOG_INFO_(buffer);
	LOG_INFO_(" to ");
	LOG_INFO_(filename);
	LOG_INFO_(" (length=%i)", len);
	LOG_INFO_("\n");
	cfs_write(filehandle, buffer, len);
	cfs_close(filehandle);
	filehandle = -1;
}

void resetInterfaces() {
	NETSTACK_RADIO.off();
	NETSTACK_MAC.off();
	NETSTACK_RADIO.init();
	NETSTACK_RADIO.on();
	NETSTACK_MAC.init();
	NETSTACK_MAC.on();
	NETSTACK_NETWORK.init();
}

void removeOldFiles() {
	cfs_remove(MYCPUFILENAME);
	cfs_remove(MYLPMFILENAME);
	cfs_remove(MYDEEPSLEEPFILENAME);
	cfs_remove(MYTXFILENAME);
	cfs_remove(MYRXFILENAME);
	cfs_remove(MYTICKSFILENAME);
	cfs_remove(MYSECONDSFILENAME);
}

void outputAllFiles() {
	LOG_INFO("Outputting recorded energy uses!\n");
	outputFileContents(MYCPUFILENAME);
	outputFileContents(MYLPMFILENAME);
	outputFileContents(MYDEEPSLEEPFILENAME);
	outputFileContents(MYTXFILENAME);
	outputFileContents(MYRXFILENAME);
	LOG_INFO("Outputting recorded times!\n");
	outputFileContents(MYTICKSFILENAME);
	outputFileContents(MYSECONDSFILENAME);
}
