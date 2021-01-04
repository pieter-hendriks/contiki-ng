#!/bin/bash
# Touch the file so Make will definitely re-make it.
# Probably not as necessary now that we've removed the compile-time define method of distinguishing
# But can't hurt, either.
touch my_leaf_app.c 
if [ -e /dev/ttyUSB1 ]; then
	echo "Installing leaf onto port 1!"
	make -j 10 my_leaf_app.upload PORT=/dev/ttyUSB1
elif [ -e /dev/ttyUSB0 ]; then
	echo "Only a single sensor is connected. Installing leaf onto port 0!"
	make -j 10 my_leaf_app.upload PORT=/dev/ttyUSB0
else
	echo "Neither /dev/ttyUSB0 nor /dev/ttyUSB1 exist. Please ensure sensors are connected for flashing."
fi
