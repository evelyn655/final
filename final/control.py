import serial
import time

# XBee setting
serdev = '/dev/ttyUSB0'
s = serial.Serial(serdev, 9600)

print("Sending /RPC_Line_Following/run\n")
s.write("/RPC_Line_Following/run\n".encode())
time.sleep(25)
print("Sending /RPC_Parking/run\n")
s.write("/RPC_Parking/run\n".encode())
time.sleep(10)
print("/RPC_AprilTag/run\n")
s.write("/RPC_AprilTag/run\n".encode())

# s.write("/RPC_Parking/run 10 5 U \n".encode())
# time.sleep(1)


s.close()