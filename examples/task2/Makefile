# Use TSCH MAC layer
MAKE_MAC = MAKE_MAC_TSCH
# Nullnet (dummy) network layer
MAKE_NET = MAKE_NET_NULLNET
# Nullrouting (dummy) routing functionality 
MAKE_ROUTING = MAKE_ROUTING_NULLROUTING

# Set energest ON
CFLAGS += -Xpreprocessor -DENERGEST_CONF_ON=1
# Set no minimal 6TSCH schedule - we configure our own
CFLAGS += -Xpreprocessor -DTSCH_SCHEDULE_CONF_WITH_6TISCH_MINIMAL=0 

CONTIKI_PROJECT = task2_root task2_leaf
all: $(CONTIKI_PROJECT)



CONTIKI=../..
include $(CONTIKI)/Makefile.include
