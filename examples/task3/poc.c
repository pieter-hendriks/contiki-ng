#include "contiki.h"
#include "os/storage/cfs/cfs-coffee.h"
#include "sys/log.h"
#define LOG_MODULE "MyApp"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(poc, "Proof of concept");

PROCESS_THREAD(poc, ev, data)
{
  static int filehandle = 0;
  PROCESS_BEGIN();
  filehandle = cfs_open("MYTESTFILEPIETERHENDRIKS", CFS_READ | CFS_WRITE);
  char buffer[1024];
  int ret;
  ret = cfs_read(filehandle, buffer, 1024);
  if (ret != 0 && ret != -1) 
  {
    cfs_close(filehandle);
    cfs_remove("MYTESTFILEPIETERHENDRIKS");
    filehandle = cfs_open("MYTESTFILEPIETERHENDRIKS", CFS_READ | CFS_WRITE);
  }
  if (ret != -1) 
  {
    buffer[ret] = '\0';
    LOG_INFO("Ret = %i\n", ret);
    LOG_INFO("Read string: %s\n", buffer);

    for (int i = 0; i < 15; ++i) 
    {
      buffer[i] = 'a';
    }

    ret = cfs_write(filehandle, buffer, 15);
    LOG_INFO("WRet = %i\n", ret);

    cfs_close(filehandle);

  }
  
  PROCESS_END();
}
AUTOSTART_PROCESSES(&poc);