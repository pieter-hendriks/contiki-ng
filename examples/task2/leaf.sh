#!/bin/bash
# Touch the file so Make will definitely re-make it.
# Probably not as necessary now that we've removed the compile-time define method of distinguishing
# But can't hurt, either.
touch leaf_app.c
# Also touch tsch files. We change behaviour with command-line defines, so these are important
touch ../../os/net/mac/tsch/tsch.c
touch ../../os/net/mac/tsch/tsch.h
touch ../../os/net/mac/tsch/tsch-conf.h
if [ -e /dev/ttyUSB1 ]; then
	echo "Installing leaf onto port 1!"
	make -j 20 leaf_app.upload PORT=/dev/ttyUSB1
elif [ -e /dev/ttyUSB0 ]; then
	echo "Only a single sensor is connected. Installing leaf onto port 0!"
	make -j 20 leaf_app.upload PORT=/dev/ttyUSB0
else
	echo "Neither /dev/ttyUSB0 nor /dev/ttyUSB1 exist. Please ensure sensors are connected for flashing."
fi
