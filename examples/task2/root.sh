#!/bin/bash
# Touch the file so Make will definitely pick up changes
touch root_app.c
# Also touch tsch files. We change behaviour with command-line defines, so these are important
touch ../../os/net/mac/tsch/tsch.c
touch ../../os/net/mac/tsch/tsch.h
touch ../../os/net/mac/tsch/tsch-conf.h

if [ -e /dev/ttyUSB0 ]; then
	echo "Installing root onto port 0!"
	make -j 20 root_app.upload PORT=/dev/ttyUSB0
else
	echo "/dev/ttyUSB0 doesn't exist. Please ensure sensors are connected for flashing."
fi
