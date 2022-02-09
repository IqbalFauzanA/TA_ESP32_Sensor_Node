#!/usr/bin/python3
import serial
import time
import re
import RPi.GPIO as GPIO
import os
import csv

#set up GPIO
GPIO.cleanup()
GPIO.setmode(GPIO.BOARD)
PIN_TO_ESP = 37 #for making data request to ESP32
GPIO.setup(PIN_TO_ESP, GPIO.OUT)
GPIO.output(PIN_TO_ESP, GPIO.LOW)
file_name = "sensor_data.csv"
first_row = ['Time', 'EC', 'TSS', 'Turbidity', 'pH', 'Temperature']

#set up serial
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=30)

while (1):
    while (input("Type START to start making request\n") != "START"):
        pass
    input_str = ""
    GPIO.output(PIN_TO_ESP, GPIO.HIGH) #requesting data
    while (not input_str.startswith("Time: ")):
        print("Making data request to ESP32...")
        print("Waiting for first response from ESP32...")
        timepoint = 0
        while (input_str != "reqtime"): #waiting for time request
            input_str = ser.readline().decode("utf-8").strip()
            if ((input_str == "CALIB") and (time.time() - timepoint > 3)):
                print("ESP32 is still calibrating, please wait...")
                timepoint = time.time()
            elif (input_str == "reqtime"):
                print("Time request received")
        time_string = time.strftime("%H:%M\n", time.localtime())
        ser.write(time_string.encode()) #send time
        print("Current time " + time_string + " sent to ESP32")
        print("Receiving serial data from ESP32")
        input_str = ser.readline().decode("utf-8").strip()
        print("Received serial: " + input_str)
        if (input_str.startswith("Time: ")):
            #x = re.findall("[a-zA-Z]+: ([\-0-9]+.[0-9]+)", input_str) #parsing using regex
            readTime = re.search("Time: ([\-0-9]+.[0-9]+)", input_str).group(1)
            ec = re.search("EC: ([\-0-9]+.[0-9]+)", input_str).group(1)
            tbd = re.search("Turbiditas: ([\-0-9]+.[0-9]+)", input_str).group(1)
            ph = re.search("pH: ([\-0-9]+.[0-9]+)", input_str).group(1)
            temperature = re.search("Temperature: ([\-0-9]+.[0-9]+)", input_str).group(1)
            print("Reading time: " + readTime)
            print("EC: " + ec + " mS/cm")
            TSS = round(float(ec)*0.123*1000 - 41.593 , 2)
            print("Calculated TSS from EC: " + str(TSS) + " mg/L")
            print("Turbidity: " + tbd + " NTU")
            print("pH: " + ph)
            print("Temperature: " + temperature + " ^C")
            row = [readTime, ec, str(TSS), tbd, ph, temperature]
            with open(file_name, 'a+') as csvfile:
                csvwriter = csv.writer(csvfile)
                if os.stat(file_name).st_size == 0:
                    csvwriter.writerow(first_row)
                csvwriter.writerow(row)
        else:
            print("Invalid data format. Remaking request...")
    GPIO.output(PIN_TO_ESP, GPIO.LOW) #turn off the request pin
    print("Going to sleep...")
    time.sleep(10)

