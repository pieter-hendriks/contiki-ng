MAKE_MAC = MAKE_MAC_TSCH
MAKE_NET = MAKE_NET_NULLNET
MAKE_ROUTING = MAKE_ROUTING_NULLROUTING

COORDINATOR ?= 0 # Set coordinator to zero if it hasn't been defined.
CFLAGS += -Xpreprocessor -DMYAPP_AS_COORDINATOR=$(COORDINATOR)
CFLAGS += -Xpreprocessor -DENERGEST_CONF_ON=1
#CFLAGS += -Xpreprocessor -DTSCH_CONF_CHECK_TIME_AT_ASSOCIATION=1
#CFLAGS += -Xpreprocessor -DBUILD_WITH_SHELL=1
#CFLAGS += -Xpreprocessor -DTSCH_LOG_CONF_PER_SLOT=1
#modules += os/services/shell
#modules += os/services/simple_energest
#CFLAGS += -Xpreprocessor -DTSCH_SCHEDULE_CONF_WITH_6TISCH_MINIMAL=1
#CFLAGS += -Xpreprocessor -DTSCH_CONF_JOIN_HOPPING_SEQUENCE='(uint8_t[]){26}'

CONTIKI_PROJECT = poc my_app
all: $(CONTIKI_PROJECT)

CONTIKI=../..
include $(CONTIKI)/Makefile.include
