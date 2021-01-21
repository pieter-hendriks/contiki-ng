This file describes how to use the task 3 implementation.

Helper scripts are provided for easy use. In the examples/task3 directory, leaf.sh builds the leaf application and uploads it to /dev/ttyUSB1 (if present - if only 0 is present, a warning is issued and it is uploaded to port 0). Similarly, root.sh does the same for the coordinator application and uploads to port 0 (never warns).

A login script is also available. Login script takes a single parameter: index of the port to connect to. Accepts only 0 or 1, as we'd never had more than 2 connected. 

After installation, the program starts running on the nodes automatically.
Output is written to a file on the node as it goes (debugging output to serial port). When the device is rebooted, the file it wrote during the last program run is read and output through the serial port. Then, the file is removed so the program can start writing again. 

Data is available in plotting/data/, plotting script in plotting/.

