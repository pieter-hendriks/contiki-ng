# Use TSCH MAC layer
MAKE_MAC = MAKE_MAC_TSCH
# Nullnet (dummy) network layer
MAKE_NET = MAKE_NET_NULLNET
# Nullrouting (dummy) routing functionality 
MAKE_ROUTING = MAKE_ROUTING_NULLROUTING


# Define the joining sequences
CFLAGS += -Xpreprocessor -DTSCH_CONF_DEFAULT_HOPPING_SEQUENCE="(uint8_t[]){ 20 }" # TSCH_HOPPING_SEQUENCE_1_1
#CFLAGS += -Xpreprocessor -DTSCH_CONF_DEFAULT_HOPPING_SEQUENCE="(uint8_t[]){ 15, 25, 26, 20 }" # TSCH_HOPPING_SEQUENCE_4_4
#CFLAGS += -Xpreprocessor -DTSCH_CONF_DEFAULT_HOPPING_SEQUENCE="(uint8_t[]){ 16, 17, 23, 18, 26, 15, 25, 22, 19, 11, 12, 13, 24, 14, 20, 21 }" # TSCH_HOPPING_SEQUENCE_16_16



PROJECT_SOURCEFILES += helpers.c

# Set energest ON
CFLAGS += -Xpreprocessor -DENERGEST_CONF_ON=1

CONTIKI_PROJECT = task2_root task2_leaf
all: $(CONTIKI_PROJECT) 




CONTIKI=../..
include $(CONTIKI)/Makefile.include
