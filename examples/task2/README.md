This file describes how to use the task 2 implementation.

Helper scripts are provided for easy use. In the examples/task2/ directory, leaf.sh builds the leaf application and uploads it to /dev/ttyUSB1 (if present - if only 0 is present, a warning is issued and it is uploaded to port 0). Similarly, root.sh does the same for the root application and uploads to port 0 (never warns).

A login script is also available. Login script takes a single parameter: index of the port to connect to. Accepts only 0 or 1, as we'd never had more than 2 connected. 

After installation, the program starts running on the nodes automatically.

Output is written to the serial port. Connection to the nodes must be maintained to read the output. 


Plotting scripts are available under plotting/
Data is available under plotting/data/

