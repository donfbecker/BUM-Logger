import serial
import datetime
import sys

ser = serial.Serial(sys.argv[1], 9600)
cmd = "dumplog"
ser.write(cmd.encode())

line = "";
while True:
	line = ser.readline().decode().strip();
	if line == ".":
		break;

	print(line);

ser.close()
