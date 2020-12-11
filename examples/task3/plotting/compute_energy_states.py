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

power_leaf = [0, 0, 0, 0, 0]
power_root = [0, 0, 0, 0, 0]
for s in dataSets:
    power_leaf[0] += s.cpu_time_leaf
    power_leaf[1] += s.cpu_sleep_time_leaf
    power_leaf[2] += s.cpu_deepsleep_time_leaf
    power_leaf[3] += s.radio_rx_time_leaf
    power_leaf[4] += s.radio_tx_time_leaf

    power_root[0] += s.cpu_time_coordinator
    power_root[1] += s.cpu_sleep_time_coordinator
    power_root[2] += s.cpu_deepsleep_time_coordinator
    power_root[3] += s.radio_rx_time_coordinator
    power_root[4] += s.radio_tx_time_coordinator

power_leaf = [x / d11.rtimerTicksPerSecond for x in power_leaf]
power_root = [x / d11.rtimerTicksPerSecond for x in power_root]

sentKiloBits = 12 * 60 * 64 * 8 / 1024.0

computePower = lambda pl: 0.01535 * pl[0] + 0.00959 * pl[1] + 0.00258 * pl[2] + 0.02832 * pl[3] + 0.03112 * pl[4]
leafPower = computePower(power_leaf)
rootPower = computePower(power_root)

leafPowerKbit = leafPower / sentKiloBits
rootPowerKbit = rootPower / sentKiloBits

print(f"sent size = {sentKiloBits}")
# Each power state is recorded in ticks, where tick = 1 / 32768 of a second

print(f"leafPower = {leafPower}")
print(f"rootPower = {rootPower}")
print(f"leafPowerKbit = {leafPowerKbit}")
print(f"leafPowerKbit = {rootPowerKbit}")