#!/bin/bash
# Touch the file so Make will definitely pick up changes
touch my_coordinator_app.c

if [ -e /dev/ttyUSB0 ]; then
	echo "Installing root onto port 0!"
	make -j 20 my_coordinator_app.upload PORT=/dev/ttyUSB0
else
	echo "/dev/ttyUSB0 doesn't exist. Please ensure sensors are connected for flashing."
fi
