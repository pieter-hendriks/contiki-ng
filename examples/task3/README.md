This file describes how to use the task 3 implementation.

Helper scripts are provided for easy use. In the examples/task3 directory, giving the command ./upload x will build the application and upload it to /dev/ttyUSBx. As of right now, the script only supports the values 0 and 1.

When x is zero, the application is built for the root node. When x is one, the application is built for the leaf node.

Then, ./login 0 connects to the root node and will output all required information.

Some additional output is available through ./login 1 (assuming both are still connected to the computer), but this information was not used to create our analyses. 

Plotting script is ./plotting/plot.py and was fed the files ('out0.txt' and 'out1.txt' as included in data_raw.zip). 