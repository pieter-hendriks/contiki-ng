import data.data_1_1 as d11
import data.data_1_3 as d13
import data.data_1_7 as d17
import data.data_3_1 as d31
import data.data_3_3 as d33
import data.data_3_7 as d37
import data.data_5_1 as d51
import data.data_5_3 as d53
import data.data_5_7 as d57
import data.data_10_1 as d91
import data.data_10_3 as d93
import data.data_10_7 as d97



dataSets = [d11, d13, d17, d31, d33, d37, d51, d53, d57, d91, d93, d97]
dataSets1m = [d11, d13, d17]
dataSets3m = [d31, d33, d37]
dataSets5m = [d51, d53, d57]
dataSets10m = [d91, d93, d97]
dataSets1p = [d11, d31, d51, d91]
dataSets3p = [d13, d33, d53, d93]
dataSets7p = [d17, d37, d57, d97]

powerSets = [dataSets1p, dataSets3p, dataSets7p]
distanceSets = [dataSets1m, dataSets3m, dataSets5m, dataSets10m]
distances = ["1m", "3m", "5m", "10m"]
powers = ["-1dBm", "3dBm", "7dBm"]
distancePowers=['1m, -1dBm', '1m, 3dBm', '1m, 7dBm', '3m, -1dBm', '3m, 3dBm', '3m, 7dBm', '5m, -1dBm', '5m, 3dBm', '5m, 7dBm', '10m, -1dBm', '10m, 3dBm', '10m, 7dBm']
powerColors = ['ro-', 'go-', 'bo-']
distanceColors = ['ro-', 'go-', 'bo-', 'mo-']
from matplotlib.pyplot import *
output_suffix = ".png"


# Plot average transmission attempts per packet
# Show per powerlevel
# So for distances 1, 3, 5, 10 

figure()
title("Transmission reliability per power level")
xlabel("distance (meters)")
ylabel("Average transmission attempts required")
for i in range(len(powerSets)):
  s = powerSets[i]
  plot(distances, [sum(x.numtx) / (1.0 * len(x.numtx)) for x in s], distanceColors[i], label=f"Average # transmissions needed per packet at power level {powers[i]}")
legend()
ylim(1, 1.2)
savefig(f"transmission_reliability{output_suffix}")
clf()

# Plot average packet latencies, bar graph
figure(figsize=[8,8])
diffSets = [[x - y for x, y in zip(dataset.rxTimes, dataset.scheduleTimes)] for dataset in dataSets]
latencies = [sum(x) / (1.0 * len(x)) for x in diffSets]
latencies = [1000 * x / d11.ticksPerSecond for x in latencies]
bar(distancePowers, latencies, tick_label=[x.replace(', ', '\n') for x in distancePowers])
xlabel("Test setup")
ylabel("Latency (ms)")
title("Average packet latencies per distance/power combination")
savefig(f"average_latencies{output_suffix}")
clf()

def total(p):
  return p[1] + p[2] + p[3]

# Plot time spent in each power state for 1m distance
figure()
coor_powers = [0, 0, 0, 0, 0]
leaf_powers = [0, 0, 0, 0, 0]
for s in dataSets1m:
  coor_powers[0] += s.cpu_time_coordinator
  coor_powers[1] += s.cpu_sleep_time_coordinator
  coor_powers[2] += s.cpu_deepsleep_time_coordinator
  coor_powers[3] += s.radio_tx_time_coordinator
  coor_powers[4] += s.radio_rx_time_coordinator
  
  leaf_powers[0] += s.cpu_time_leaf
  leaf_powers[1] += s.cpu_sleep_time_leaf
  leaf_powers[2] += s.cpu_deepsleep_time_leaf
  leaf_powers[3] += s.radio_tx_time_leaf
  leaf_powers[4] += s.radio_rx_time_leaf

leaf_powers = [x / total(leaf_powers) for x in leaf_powers]
coor_powers = [x / total(coor_powers) for x in coor_powers]
plot(['CPU Active', 'CPU Sleep', 'CPU Deep Sleep', 'Radio Transmit', 'Radio Listen'], coor_powers, 'ro-', label='Root')
plot(['CPU Active', 'CPU Sleep', 'CPU Deep Sleep', 'Radio Transmit', 'Radio Listen'], leaf_powers, 'bo-', label='Leaf')
legend()
savefig(f"powerstates_1m{output_suffix}")
clf()


# Plot time spent in each power state for 10m distance
figure()
coor_powers = [0, 0, 0, 0, 0]
leaf_powers = [0, 0, 0, 0, 0]
for s in dataSets10m:
  coor_powers[0] += s.cpu_time_coordinator
  coor_powers[1] += s.cpu_sleep_time_coordinator
  coor_powers[2] += s.cpu_deepsleep_time_coordinator
  coor_powers[3] += s.radio_tx_time_coordinator
  coor_powers[4] += s.radio_rx_time_coordinator
  
  leaf_powers[0] += s.cpu_time_leaf
  leaf_powers[1] += s.cpu_sleep_time_leaf
  leaf_powers[2] += s.cpu_deepsleep_time_leaf
  leaf_powers[3] += s.radio_tx_time_leaf
  leaf_powers[4] += s.radio_rx_time_leaf

leaf_powers = [x / total(leaf_powers) for x in leaf_powers]
coor_powers = [x / total(coor_powers) for x in coor_powers]
plot(['CPU Active', 'CPU Sleep', 'CPU Deep Sleep', 'Radio Transmit', 'Radio Listen'], coor_powers, 'ro-', label='Root')
plot(['CPU Active', 'CPU Sleep', 'CPU Deep Sleep', 'Radio Transmit', 'Radio Listen'], leaf_powers, 'bo-', label='Leaf')
legend()
savefig(f"powerstates_10m{output_suffix}")
clf()

# Plot 



# slotTime 
# slotTicks 
# ticksPerSecond 
# numtx
# scheduleTimes
# cpu_time_leaf 
# cpu_sleep_time_leaf 
# cpu_deepsleep_time_leaf
# radio_rx_time_leaf 
# radio_tx_time_leaf 
# rtimerTicksPerSecond 
# rxTimes 
# cpu_time_coordinator 
# cpu_sleep_time_coordinator
# cpu_deepsleep_time_coordinator
# radio_rx_time_coordinator
# radio_tx_time_coordinator
