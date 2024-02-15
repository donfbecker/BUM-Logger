import serial
import datetime
import sys

ser = serial.Serial(sys.argv[1], 9600)
cmd = "settime " + datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
print(cmd)
ser.write(cmd.encode())
print(ser.readline().decode())
ser.close()
