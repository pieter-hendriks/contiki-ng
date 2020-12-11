from matplotlib.pyplot import *
import re

if __name__ == "__main__":
	startMarker = "[INFO: MyApp     ] Received first packet - assuming network convergence, now resetting power info."
	packetMarker = "[INFO: MyApp     ] <--- Received"
	filenames = ['out0.txt', 'out1.txt']
	powerOrder = ['CPU', 'LPM', 'DEEPLPM','TX','RX','CPU', 'LPM', 'Deep','Tx','Rx','Total']
	output_suffix = "0"
	for filename in filenames:
		index = None	
		packetsArrived = []
		powerOnTime = {}
		latencies = []
		with open(filename, 'r') as f:
			started = False
			nextPacket = True
			currentRegex = 0
			allLines = f.readlines()
			for lineIndex in range(len(allLines)):
				line = allLines[lineIndex][:-1] # Take line, cut off the new line at the end
				if not started:
					# Skip all lines preceding the first-packet-received
					# As this marks the start of our network setup.
					if line[:len(startMarker)] == startMarker:
						started = True
					continue
				if nextPacket:
					if line[:len(packetMarker)] == packetMarker:
						nextPacket = False
						index = -1
						result = re.match(re.escape(packetMarker) + "\s(\d+)", line)
						if (int(result[1]) > 120):
							break
						packetsArrived.append(int(result[1]))
						# \w's in the below expression are slightly more lenient than we'd like (should be hexadecimal only), but close enough
						result = re.match(re.escape(packetMarker) + "\s\d+\sfrom\s\w+\.\w+\.\w+\.\w+\sat\stime\s\d+,\ssent\sat\stime\s\d+\s\(latency\sis\s(\d+)", line)
						latencies.append(int(result[1]))
					continue

				# From here is a repeating pattern, we can select pretty simply: 
				if index == -1:
					# Skip one intermediate line
					index = 0
					continue
				result=re.search(powerOrder[index] + ":\s*(\d+)\sticks", line)


				powerOnTime[powerOrder[index] + ("SEND" if index <= 4 else "RECV")] = int(result[1])
				index += 1
				if index == 11:
					nextPacket = True
					continue
			# Packet latencies plot
			plot(packetsArrived, latencies, 'ro-', label='Latency per packet')
			ylim(4, 20)
			title("The recorded latencies for each packet")
			xlabel("Packet index")
			ylabel("Recorded latency (ASN)")
			legend()
			savefig("PacketLatencies" + output_suffix)
			clf()

			# Bar chart with power states
			senderData = [powerOnTime[powerOrder[x]+"SEND"] for x in range(5)]
			recvData = [powerOnTime[powerOrder[x]+"RECV"] for x in range(5, 10)]
			senderData.append(sum(senderData[:3])) # Sum of the CPU states is total ticks count
			recvData.append(powerOnTime[powerOrder[10]+"RECV"])
			senderData = [x/32768 for x in senderData]
			recvData = [x/32768 for x in recvData]
			barLabels = ['High CPU', 'LPM CPU', 'DLPM CPU', 'TX', 'RX', 'Total']
			ylabel("Seconds")
			xlabel("Power state")
			sendBar = bar([x * 2 for x in range(len(senderData))], senderData, color='r')
			recvBar = bar([(x+0.4) * 2 for x in range(len(recvData))], recvData, color='b')
			assert len(senderData) == len(recvData)
			xticks(ticks=[0.4 + 2 * x for x in range(len(senderData))], labels=barLabels)
			legend((sendBar, recvBar), ("Sender (leaf)", "Receiver (root)"))
			savefig("PowerStates" + output_suffix)
			clf()

			print(f"PACKET ARRIVAL RATE: {len(packetsArrived) / 120}")
			print(f"AVERAGE LATENCY: {sum(latencies)/len(latencies)}")
			powerStateValues = [0.01535 * 3300, 0.00959 * 3300, 0.00258 * 3300, 0.03014 * 3300, 0.03112 * 3300, 0]

			leafConsume = sum([x * y for x,y in zip(powerStateValues, senderData)]) / 1000
			rootConsume = sum([x * y for x,y in zip(powerStateValues, recvData)]) / 1000
			print(f"LEAF ENERGY CONSUMPTION: {leafConsume} J")
			print(f"ROOT ENERGY CONSUMPTION: {rootConsume} J")

			print(f"Since each packet carried a data payload of 52 bytes, that's\n\tLEAF: {leafConsume / (52*8/1024)} J/kbit\n\tROOT: {rootConsume / (52*8/1024)} J/kbit")

			#define VCC 3.3f
			#define wc_CPU 0.01535f * VCC * 1000
			#define wc_LPM 0.00959f * VCC * 1000
			#define wc_deep_LPM 0.00258f * VCC * 1000
			#define wc_LISTEN 0.02832f * VCC * 1000
			#define wc_Rx 0.03014f * VCC * 1000
			#define wc_Tx 0.03112f * VCC * 1000
			output_suffix = "1"
		
			