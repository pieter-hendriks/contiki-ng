import os 

d = 'data/'
for entry in os.scandir(d):
	if (entry.is_dir()):
		for e in os.scandir(entry):
			# Descend into data_## directories
			if e.is_file():
				with open(e.path, 'r') as f:
					lines = f.readlines()
				i = 0
				while i < len(lines):
					if '[' not in lines[i] or '/dev/ttyUSB' in lines[i] or 'INCOMING PACKET RECEIVED' in lines[i]:
						del lines[i]
						continue
					lines[i] = lines[i].split('] ')[1]
					i += 1
				with open(e.path, 'w') as f:
					f.writelines(lines)

