#! /usr/bin/python3
import serial
import time
import psutil
import sys

debugSerialOut = 0
debugSerialIn = 0
debugUsage = 0
sendOnlyTestData = 0

arduino = serial.Serial()
arduino.baudrate = 115200
arduino.port = sys.argv[1]

coreUsage = psutil.cpu_percent(percpu=True)

def updateArduino(dataToWrite):
	if sendOnlyTestData: dataToWrite = '100.00%'
	if debugSerialOut: print("out:", dataToWrite)
	bytesSent = arduino.write(dataToWrite.encode('utf-8'))
	return bytesSent

arduino.open()
while (1):
	requestFromArduino = arduino.read()
	if requestFromArduino:
		requestedCore = int.from_bytes(requestFromArduino, "big", signed=False)
		if(requestedCore == 0):
			coreUsage = psutil.cpu_percent(percpu=True)
			if debugUsage: print("Load Updated:", coreUsage)
		if debugSerialIn: print("req:", requestedCore)
		# usage = coreUsage[requestedCore]
		updateArduino(str(coreUsage[requestedCore]) + "%")
arduino.close()