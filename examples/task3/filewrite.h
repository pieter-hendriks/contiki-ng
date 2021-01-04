#ifndef INC_UANTW_FILEWRITE_H_
#define INC_UANTW_FILEWRITE_H_

bool validPrintfResult(int ret, int maxSize) 
{
	return ret >= 0 && ret <= maxSize;
}
void writeFile(char* prefix, char* suffix, void* dataptr, char* dataFormatString, uint8_t datasize, char* error_indicator)
{
	#define INC_UANTW_FILEWRITE_H_MYLEAFAPP_BUFFERSIZE 1024
	#define INC_UANTW_FILEWRITE_H_MYLEAFAPP_SEPARATOR ", "

	uint8_t* data = (uint8_t*)dataptr; // So we can increment/decrement per byte.
	char buffer[INC_UANTW_FILEWRITE_H_MYLEAFAPP_BUFFERSIZE];
	char* bufferptr = (char*)buffer;
	unsigned size = INC_UANTW_FILEWRITE_H_MYLEAFAPP_BUFFERSIZE;
	int ret = snprintf(bufferptr, size, prefix);
	if (!validPrintfResult(ret, size)) {
		LOG_ERR(error_indicator);
		return;
	}
	bufferptr += ret; size -= ret;
	for (uint8_t i = 0; i < 59; ++i) {
		ret = snprintf(bufferptr, size, dataFormatString, (char*)(data));
		if (!validPrintfResult(ret, size)) {
			LOG_ERR(error_indicator);
			return;
		}
		bufferptr += ret; size -= ret;
		data += datasize;
		ret = snprintf(bufferptr, size, INC_UANTW_FILEWRITE_H_MYLEAFAPP_SEPARATOR);
		if (!validPrintfResult(ret, size)) {
			LOG_ERR(error_indicator);
			return;
		}
		bufferptr += ret; size -= ret;
	}
	ret = snprintf(bufferptr, size, dataFormatString, (char*)(data));
	if (!validPrintfResult(ret, size)) {
		LOG_ERR(error_indicator);
		return;
	}
	bufferptr += ret; size -= ret;
	data += datasize;
	ret = snprintf(bufferptr, size, suffix);
	if (!validPrintfResult(ret, size)) {
		LOG_ERR(error_indicator);
		return;
	}
	bufferptr += ret; size -= ret;

	int filehandle = cfs_open(MYFILENAME, CFS_WRITE | CFS_APPEND);
	ret = cfs_write(filehandle, buffer, INC_UANTW_FILEWRITE_H_MYLEAFAPP_BUFFERSIZE - size);
	cfs_close(filehandle);
	#undef INC_UANTW_FILEWRITE_H_MYLEAFAPP_SEPARATOR
	#undef INC_UANTW_FILEWRITE_H_MYLEAFAPP_BUFFERSIZE
}


#endif