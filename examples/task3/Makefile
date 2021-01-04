MAKE_MAC = MAKE_MAC_TSCH
MAKE_NET = MAKE_NET_NULLNET
MAKE_ROUTING = MAKE_ROUTING_NULLROUTING

CFLAGS += -Xpreprocessor -DENERGEST_CONF_ON=1
CFLAGS += -Xpreprocessor -DTSCH_SCHEDULE_CONF_WITH_6TISCH_MINIMAL=0 # Set to disable minimal start schedule
#CFLAGS += -Xpreprocessor -DTSCH_CONF_CHECK_TIME_AT_ASSOCIATION=1
#CFLAGS += -Xpreprocessor -DBUILD_WITH_SHELL=1
#CFLAGS += -Xpreprocessor -DTSCH_LOG_CONF_PER_SLOT=1
#modules += os/services/shell
#modules += os/services/simple_energest
#CFLAGS += -Xpreprocessor -DTSCH_CONF_JOIN_HOPPING_SEQUENCE='(uint8_t[]){26}'

CONTIKI_PROJECT = my_leaf_app my_coordinator_app
all: $(CONTIKI_PROJECT)

CONTIKI=../..
include $(CONTIKI)/Makefile.include
