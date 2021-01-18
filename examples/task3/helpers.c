#include "helpers.h"
#include "conf_my_app.h"
#include "os/net/mac/tsch/tsch.h"
#include "os/storage/cfs/cfs.h"
#include "os/sys/energest.h"
#define LOG_LEVEL LOG_LEVEL_INFO
#define LOG_MODULE "HELPER"

void createTschSchedule(bool coordinator, linkaddr_t* address) {
	uint8_t options = 0;
	if (coordinator) {
		options ^= LINK_OPTION_RX;
	} else {
		options ^= LINK_OPTION_TX;
	}
	// First, the slotframe should be added to tsch 
	struct tsch_slotframe *sf = tsch_schedule_add_slotframe(APP_SLOTFRAME_HANDLE, TSCH_SCHEDULE_DEFAULT_LENGTH);
	// Create a TX dedicated cell in (1, 1) 
	tsch_schedule_add_link(sf, options, LINK_TYPE_NORMAL, address, 1, 1, 1);
	// Create advertisement cell in (0, 0)
	tsch_schedule_add_link(sf, LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED | LINK_OPTION_TIME_KEEPING, LINK_TYPE_ADVERTISING_ONLY, &tsch_broadcast_address, 0, 0, 1);
}
void outputPreviousStatisticsNoRemove() {
	// allocate buffer
	char readBuffer[256] = {0};
	int filehandle = cfs_open(MYFILENAME, CFS_READ);
	LOG_INFO("Outputting file read:\n");
	// Iterate while we keep reading 256 bytes, outputs everything until EOF
	int ret = cfs_read(filehandle, readBuffer, 256);
	LOG_INFO_(readBuffer);
	while (ret == 256) {
		memset(readBuffer, 0, 256);
		ret = cfs_read(filehandle, readBuffer, 256);
		LOG_INFO_(readBuffer);
	}
	LOG_INFO_("\n----------------------------\nFile read output above.\n");
	cfs_close(filehandle);
}

void outputPreviousStatistics() {
	outputPreviousStatisticsNoRemove();
	cfs_remove(MYFILENAME);
}

void recordEnergyStatistics(int filehandle, char* suffix) {
	// Update energest values.
	energest_flush();
	#define RECORDENERGY_BUFFERSIZE 1024
	char buffer[RECORDENERGY_BUFFERSIZE] = {0};
	int ret = snprintf(buffer, RECORDENERGY_BUFFERSIZE, "cpu_time%s = %"PRIu64"\ncpu_sleep_time%s = %"PRIu64"\ncpu_deepsleep_time%s = %"PRIu64"\nradio_rx_time%s = %"PRIu64"\nradio_tx_time%s = %"PRIu64"\n", 
															suffix, energest_type_time(ENERGEST_TYPE_CPU), 
															suffix, energest_type_time(ENERGEST_TYPE_LPM), 
															suffix,	energest_type_time(ENERGEST_TYPE_DEEP_LPM), 
															suffix,	energest_type_time(ENERGEST_TYPE_TRANSMIT),
															suffix, energest_type_time(ENERGEST_TYPE_LISTEN)
										);
	if (ret < 0 || ret > RECORDENERGY_BUFFERSIZE) {
		LOG_ERR("Failed to write power states. (Not enough buffer space in record energy statistics, presumably)");
	} else {
		cfs_write(filehandle, buffer, ret);
	}
	#undef RECORDENERGY_BUFFERSIZE
}

void outputTimerInformation() {
	LOG_INFO("\nTime per slot:\n\t(us): %u\n\t(ticks): %lu\nTicks per second: %u\n", tsch_timing_us[tsch_ts_timeslot_length], tsch_timing[tsch_ts_timeslot_length], RTIMER_ARCH_SECOND);
}


void setTxPowerHigh() {
	radio_result_t res = NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, APP_TX_POWER_HIGH);
	if (res != RADIO_RESULT_OK) {
		LOG_ERR("Encountered error while setting radio tx power high (%u)", res);
	}
	// Reset the radio stack
	NETSTACK_RADIO.init();
}
void setTxPowerDefault() {
	radio_result_t res = NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, APP_TX_POWER_DEFAULT);
	if (res != RADIO_RESULT_OK) {
		LOG_ERR("Encountered error while setting radio tx power default (%u)", res);
	}
	// Reset the radio stack
	NETSTACK_RADIO.init();
}
void setTxPowerLow() {
	radio_result_t res = NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, APP_TX_POWER_LOW);
	if (res != RADIO_RESULT_OK) {
		LOG_ERR("Encountered error while setting radio tx power low (%u)", res);
	}
	// Reset the radio stack
	NETSTACK_RADIO.init();
}

void setTxPower(uint8_t index) {
	switch(index) {
		case 0: 
			setTxPowerDefault();
			break;
		case 1:
			setTxPowerHigh();
			break;
		case 2:
			setTxPowerLow();
			break;
		default:
			LOG_ERR("IMPLEMENTATION ERROR. TX POWER INDEX > 2");
	}
}


bool validPrintfResult(int ret, int maxSize) 
{
	return ret >= 0 && ret <= maxSize;
}

void writeFileUint64(int filehandle, char* prefix, char* suffix, uint64_t* dataptr, uint8_t len, char* error_indicator) {
	#define MYFN_BUFFERSIZE 64
	char buffer[MYFN_BUFFERSIZE] = {0};
	char* bufferptr = buffer;
	unsigned size = MYFN_BUFFERSIZE;
	// Write prefix
	int ret = snprintf(bufferptr, size, prefix);
	if (!validPrintfResult(ret, size)) {
		LOG_ERR(error_indicator);
		return;
	}
	cfs_write(filehandle, buffer, ret);
	memset(bufferptr, 0, MYFN_BUFFERSIZE);
	// Write one element at a time
	for (uint8_t index = 0; index < len; ++index) {
		ret = snprintf(bufferptr, size, "%"PRIu64, dataptr[index]);
		if (!validPrintfResult(ret, size)) {
			LOG_ERR(error_indicator);
			return;
		}
		if (index != len - 1) {
			ret += snprintf(bufferptr + ret, size - ret, ", ");
			if (!validPrintfResult(ret, size)) {
				LOG_ERR(error_indicator);
				return;
			}
		}
		cfs_write(filehandle, buffer, ret);
		memset(bufferptr, 0, MYFN_BUFFERSIZE);
	}
	ret = snprintf(bufferptr, size, suffix);
	if (!validPrintfResult(ret, size)) {
		LOG_ERR(error_indicator);
		return;
	}
	cfs_write(filehandle, buffer, ret);
}


void writeFileUint8(int filehandle, char* prefix, char* suffix, uint8_t* dataptr, uint8_t len, char* error_indicator) {
	#define MYFN_BUFFERSIZE 64
	char buffer[MYFN_BUFFERSIZE] = {0};
	char* bufferptr = buffer;
	unsigned size = MYFN_BUFFERSIZE;
	// Write prefix
	int ret = snprintf(bufferptr, size, prefix);
	if (!validPrintfResult(ret, size)) {
		LOG_ERR(error_indicator);
		return;
	}
	cfs_write(filehandle, buffer, ret);
	memset(bufferptr, 0, MYFN_BUFFERSIZE);
	// Write one element at a time
	for (uint8_t index = 0; index < len; ++index) {
		ret = snprintf(bufferptr, size, "%u", dataptr[index]);
		if (!validPrintfResult(ret, size)) {
			LOG_ERR(error_indicator);
			return;
		}
		if (index != len - 1) {
			ret += snprintf(bufferptr + ret, size - ret, ", ");
			if (!validPrintfResult(ret, size)) {
				LOG_ERR(error_indicator);
				return;
			}
		}
		cfs_write(filehandle, buffer, ret);
		memset(bufferptr, 0, MYFN_BUFFERSIZE);
	}
	ret = snprintf(bufferptr, size, suffix);
	if (!validPrintfResult(ret, size)) {
		LOG_ERR(error_indicator);
		return;
	}
	cfs_write(filehandle, buffer, ret);
}

