import os 


class Measurement:
	def __init__(self, cpu, lpm, deep, tx, rx):
		self.cpu = cpu
		self.lpm = lpm
		self.deep = deep
		self.tx = tx
		self.rx = rx
	def __str__(self):
		return f"CPU: {self.cpu}\nLPM: {self.lpm}\nDeepSleep: {self.deep}\nRX: {self.rx}\nTX: {self.tx}"

class LeafMeasurement(Measurement):
	def __init__(self, seconds, ticks, cpu, lpm, deep, tx, rx):
		super().__init__(cpu, lpm, deep, tx, rx)
		self.seconds = seconds
		self.ticks = ticks
	def __str__(self):
		return f"{super().__str__()}\nseconds: {self.seconds}\nticks: {self.ticks}"



dataPrefixes = ["ticks =",
"seconds =",
"cpu =",
"lpm =",
"deep =",
"tx =",
"rx ="]
sqBrPrefix = "[INFO: HELPERS   ] "
d = 'data/'

data = {}
for entry in os.scandir(d):
	if (entry.is_dir()):
		for e in os.scandir(entry):
			# Descend into data_## directories
			if e.is_file():
				with open(e.path, 'r') as f:
					lines = f.readlines()
				i = 0
				while i < len(lines):
					if (not any([x in lines[i] for x in dataPrefixes])):
						del lines[i]
						continue
					lines[i] = lines[i][len(sqBrPrefix):-1] # Cut the prefix and the newline
					i += 1
				if "_1" in entry.name: # 
					hopSeqLen = 1
				elif "_4" in entry.name:
					hopSeqLen = 4
				
				if "_1" in e.name:
					ebPeriod = 1
				elif "_4" in e.name:
					ebPeriod = 4
				elif "_8" in e.name:
					ebPeriod = 8
				if (hopSeqLen, ebPeriod) not in data:
					data[(hopSeqLen, ebPeriod)] = []
					data[(hopSeqLen, ebPeriod)].append(LeafMeasurement(0,0,0,0,0,0,0))
					data[(hopSeqLen, ebPeriod)].append(Measurement(0,0,0,0,0))
				
				if 'leaf' in e.name:
					i = 0
					while i < len(lines):
						data[(hopSeqLen, ebPeriod)][0].ticks += int(lines[i].split(" = ")[1]) 
						data[(hopSeqLen, ebPeriod)][0].seconds += int(lines[i+1].split(" = ")[1])
						data[(hopSeqLen, ebPeriod)][0].cpu +=int(lines[i+2].split(" = ")[1])
						data[(hopSeqLen, ebPeriod)][0].lpm += int(lines[i+3].split(" = ")[1])
						data[(hopSeqLen, ebPeriod)][0].deep += int(lines[i+4].split(" = ")[1])
						data[(hopSeqLen, ebPeriod)][0].tx += int(lines[i+5].split(" = ")[1])
						data[(hopSeqLen, ebPeriod)][0].rx += int(lines[i+6].split(" = ")[1])
						i = i + 7

				elif 'root' in e.name:
					i = 0
					while i < len(lines):
						data[(hopSeqLen, ebPeriod)][1].cpu +=int(lines[i].split(" = ")[1])
						data[(hopSeqLen, ebPeriod)][1].lpm += int(lines[i+1].split(" = ")[1])
						data[(hopSeqLen, ebPeriod)][1].deep += int(lines[i+2].split(" = ")[1])
						data[(hopSeqLen, ebPeriod)][1].tx +=int(lines[i+3].split(" = ")[1])
						data[(hopSeqLen, ebPeriod)][1].rx +=int(lines[i+4].split(" = ")[1])
						i = i + 5

for k in data:
	data[k][0].seconds = int(1/2 + data[k][0].seconds / 25)
	data[k][0].ticks = int(1/2 + data[k][0].ticks / 25)
	data[k][0].cpu = int(1/2 + data[k][0].cpu / 25)
	data[k][0].lpm = int(1/2 + data[k][0].lpm / 25)
	data[k][0].deep = int(1/2 + data[k][0].deep / 25)
	data[k][0].rx = int(1/2 + data[k][0].rx / 25)
	data[k][0].tx = int(1/2 + data[k][0].tx / 25)
	data[k][1].cpu = int(1/2 + data[k][1].cpu / 25)
	data[k][1].lpm = int(1/2 + data[k][1].lpm / 25)
	data[k][1].deep = int(1/2 + data[k][1].deep / 25)
	data[k][1].rx = int(1/2 + data[k][1].rx / 25)
	data[k][1].tx = int(1/2 + data[k][1].tx / 25)

def getEnergy(k):
	return [k.cpu ,
	k.lpm ,
	k.deep ,
	k.rx ,
	k.tx ]

def getTime(k):
	return k.seconds

from matplotlib.pyplot import *



colors = {1: 'r', 4: 'g', 8: 'b'}
styles = {1: '-', 4: ':'}
fig = figure()
title("Average power states for leaf in various configs")
ylabel("Percentage of time spent in each power state")
xlabel("Power state")
for k in data:
	plot(["cpu", "lpm", "deep sleep", "RX", "TX"], getEnergy(data[k][0]), f"{colors[k[1]]}{styles[k[0]]}", label=f"Leaf HopSeq length {k[0]}, eb period {k[1]}")
legend()
#show()
savefig("leaf_powerstates.png")
close(fig)

fig = figure()
title("Average power states for root in various configs")
ylabel("Percentage of time spent in each power state")
xlabel("Power state")
for k in data:
	plot(["cpu", "lpm", "deep sleep", "RX", "TX"], getEnergy(data[k][1]), f"{colors[k[1]]}{styles[k[0]]}", label=f"Leaf HopSeq length {k[0]}, eb period {k[1]}")
legend()
#show()
savefig("root_powerstates.png")
close(fig)


fig = figure()
title = "Average connection times for all configurations"
ylabel = "Seconds"
xlabel = "Configuration"

plot([str(k) for k in data], [data[k][0].seconds for k in data], label="Average time per configuration (seconds)")
#show()
savefig("times.png")
close(fig)