#include "conf_my_app.h"
#include "helpers.h"
#define LOG_LEVEL LOG_LEVEL_INFO
#define LOG_MODULE "SENSORNETS"
void outputFileContents(int filehandle) {
	char* buffer = calloc(128, 1);
	int read = cfs_read(filehandle, buffer, 128);
	LOG_INFO("Reading filehandle %i. Output below.\n--------------------------------------------\n", filehandle);
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
void logEnergy(int filehandle) {
	energest_flush();
	logToFile(filehandle, CPUPREFIX, energest_type_time(ENERGEST_TYPE_CPU));
	logToFile(filehandle, LPMPREFIX, energest_type_time(ENERGEST_TYPE_LPM));
	logToFile(filehandle, DLPMPREFIX, energest_type_time(ENERGEST_TYPE_DEEP_LPM));
	logToFile(filehandle, TXPREFIX, energest_type_time(ENERGEST_TYPE_TRANSMIT));
	logToFile(filehandle, RXPREFIX, energest_type_time(ENERGEST_TYPE_LISTEN));
}

void doInitialSetup() {
	removeOldFiles();
	resetInterfaces();
	resetEnergy();
}

void logToFile(int filehandle, char* prefix, uint64_t value) 
{
	char buffer[128] = {0};
	int len = snprintf(buffer, 128, "%s%"PRIu64"\n", prefix, value);
	LOG_INFO("Writing %s to filehandle %i (length=%i)\n", buffer, filehandle, len);
	int ret = cfs_write(filehandle, buffer, len);
	if (ret < 0) {
		LOG_ERR("File write failed! SNPRINTF: %i, CFS_WRITE: %i\n", len, ret);
	}
	else {
		LOG_INFO("File write returned %i\n", ret);
		filehandle = -1;
	}
}

void resetInterfaces() {
	NETSTACK_RADIO.init();
	NETSTACK_MAC.init();
	NETSTACK_NETWORK.init();
}

void removeOldFiles() {
	int ret = cfs_remove(MYFILENAME);
	if (ret != 0) {
		LOG_ERR("Failed to remove file. Reason unknown; error code = %i", ret);
	}
}
