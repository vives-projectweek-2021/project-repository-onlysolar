import serial

port = serial.Serial("/dev/ttyACM0", baudrate=9600, timeout=3.0)
hasWritten = False

while True:
	rcv = port.readline()	
	while rcv == b"":
		rcv = port.readline()
	rcv = rcv[:-2]
	print(rcv)
	
	with open('solar_readings.txt', 'w') as f:
		f.write(rcv.decode('UTF-8'))
