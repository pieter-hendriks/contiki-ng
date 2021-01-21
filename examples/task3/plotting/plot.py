
from data import data1m, data10m, data100m, data50m 
from matplotlib.pyplot import *
from statistics import mean

ioff()

distances = ["1m", "10m", "50m", "100m"]
powers = {"low_": "-7 dBm", "def_": "0 dBm", "high_": "7 dBm"}
powerlist = ["-7 dBm", "0 dBm", "7 dBm"]
prefixes = ["low_", "def_", "high_"]

modules = [data1m, data10m, data50m, data100m]

# Retransmissions Figure
fig = figure()
title("Transmission reliability per power level")
xlabel("Distance (meters)")
ylabel("Amount of unsuccessful transmissions over 60 packets")
low_data = []
high_data = []
def_data = []
for module in modules:
	for prefix in prefixes:
		numtx = [x - 1 for x in getattr(module, f"{prefix}numtx")]
		numtx = [x if x != -1 else 8 for x in numtx]
		eval(f"{prefix}data.append(sum(numtx))")
plot(distances, high_data, 'ro-', label="TX power 7 dBm")
plot(distances, def_data, 'go-', label="TX power 0 dBm")
plot(distances, low_data, 'bo-', label="TX power -7 dBm")
legend()
#show()
savefig("retransmissions.png")
close(fig)

# Latencies figure
fig = figure()
title("Latencies per power level")
xlabel("Distance (meters)")
ylabel("Packet latency (ms)")
low_data = []
high_data = []
def_data = []
data = [low_data, def_data, high_data]
drop = []
for module in modules:
	for j in range(len(prefixes)):
		prefix = prefixes[j]
		schedule = getattr(module, f"{prefix}scheduleTimes")
		send = getattr(module, f"{prefix}sendTimes")
		i = 0
		while i < len(send):
			if (send[i]) == 0:
				del schedule[i]
				del send[i]
				continue
			i += 1
		data[j].append(mean([x - y for x, y in zip(send, schedule)]) / 128 * 1000)
		drop.append(60 - len(schedule))
		
plot(distances, high_data, 'ro-', label="TX power 7 dBm")
plot(distances, def_data, 'go-', label="TX power 0 dBm")
plot(distances, low_data, 'bo-', label="TX power -7 dBm")
legend()
#show()
savefig("latencies.png")
close(fig)



# Power states: 
# Leaf and root on seperate graphs
# All distances + TX powers on one graph
fig = figure()
colors = ['r', 'g', 'b', 'm']
styles = ['-', ':', '-.']
for k in range(len(modules)):
	module = modules[k]
	for j in range(len(prefixes)):
		prefix = prefixes[j]
		states = [
			getattr(module, f"{prefix}cpu_time_leaf"),
			getattr(module, f"{prefix}cpu_sleep_time_leaf"),
			getattr(module, f"{prefix}cpu_deepsleep_time_leaf"),
			getattr(module, f"{prefix}radio_rx_time_leaf"),
			getattr(module, f"{prefix}radio_tx_time_leaf")
		]
		states = [x / (states[0] + states[1] + states[2]) for x in states]
		plot(['CPU Active', 'CPU Sleep', 'CPU Deep Sleep', 'Radio TX', 'Radio RX'], states, f'{colors[k]}{styles[j]}', label=f"{distances[k]}, {powerlist[j]}")
		
xlabel("Device power state")
ylabel("Time spent (percentage of total)")
legend()
savefig("leaf_allstates.png")
#show()
close (fig)

fig = figure()
colors = ['r', 'g', 'b', 'm']
styles = ['-', ':', '-.']
for k in range(len(modules)):
	module = modules[k]
	for j in range(len(prefixes)):
		prefix = prefixes[j]
		states = [
			getattr(module, f"{prefix}cpu_time_leaf"),
			getattr(module, f"{prefix}cpu_sleep_time_leaf"),
			getattr(module, f"{prefix}cpu_deepsleep_time_leaf"),
			getattr(module, f"{prefix}radio_rx_time_leaf"),
			getattr(module, f"{prefix}radio_tx_time_leaf")
		]
		states = [x / (states[0] + states[1] + states[2]) for x in states]
		states = states[3:]
		plot(['Radio TX', 'Radio RX'], states, f'{colors[k]}{styles[j]}', label=f"{distances[k]}, {powerlist[j]}")
xlabel("Device power state")
ylabel("Time spent (percentage of total)")
legend()
savefig("leaf_rx_tx.png")
#show()
close (fig)

fig = figure()
colors = ['r', 'g', 'b', 'm']
styles = ['-', ':', '-.']
for k in range(len(modules)):
	module = modules[k]
	for j in range(len(prefixes)):
		prefix = prefixes[j]
		states = [
			getattr(module, f"{prefix}cpu_time_leaf"),
			getattr(module, f"{prefix}cpu_sleep_time_leaf"),
			getattr(module, f"{prefix}cpu_deepsleep_time_leaf"),
			getattr(module, f"{prefix}radio_rx_time_leaf"),
			getattr(module, f"{prefix}radio_tx_time_leaf")
		]
		states = [x / (states[0] + states[1] + states[2]) for x in states]
		plot(['CPU Active', 'CPU Sleep', 'CPU Deep Sleep', 'Radio TX', 'Radio RX'], states, f'{colors[k]}{styles[j]}', label=f"{distances[k]}, {powerlist[j]}")
		
xlabel("Device power state")
ylabel("Time spent (percentage of total)")
legend()
savefig("root_allstates.png")
#show()
close(fig)

fig = figure()
colors = ['r', 'g', 'b', 'm']
styles = ['-', ':', '-.']
for k in range(len(modules)):
	module = modules[k]
	for j in range(len(prefixes)):
		prefix = prefixes[j]
		states = [
			getattr(module, f"{prefix}cpu_time_coordinator"),
			getattr(module, f"{prefix}cpu_sleep_time_coordinator"),
			getattr(module, f"{prefix}cpu_deepsleep_time_coordinator"),
			getattr(module, f"{prefix}radio_rx_time_coordinator"),
			getattr(module, f"{prefix}radio_tx_time_coordinator")
		]
		states = [x / (states[0] + states[1] + states[2]) for x in states]
		states = states[3:]
		plot(['Radio TX', 'Radio RX'], states, f'{colors[k]}{styles[j]}', label=f"{distances[k]}, {powerlist[j]}")
xlabel("Device power state")
ylabel("Time spent (percentage of total)")
legend()
savefig("root_rx_tx.png")
#show()
close(fig)
