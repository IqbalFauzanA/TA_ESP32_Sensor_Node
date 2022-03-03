#!/usr/bin/python3
import serial
import time
import re
import RPi.GPIO as GPIO
import os
import csv
import shutil
from dataclasses import dataclass

# set up GPIO
GPIO.cleanup()
GPIO.setmode(GPIO.BOARD)
PIN_TO_ESP = 37  # for making data request to ESP32
GPIO.setup(PIN_TO_ESP, GPIO.OUT)
GPIO.output(PIN_TO_ESP, GPIO.LOW)

# set up serial
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=10)
ser.reset_input_buffer()

def sendRequestAndReceiveSerial(reqMessage):
    input_str = ""
    timepoint = time.time()
    timepoint1 = time.time() - 3
    while ((not input_str.startswith("Data#"))
            and (time.time() - timepoint < 60)):
        ser.write(reqMessage.encode())
        print("Receiving serial data from ESP32")
        input_str = ser.readline().decode("utf-8").strip()
        if ((input_str == "CALIB") and (time.time() - timepoint1 > 3)):
            print("ESP32 is still calibrating, please wait...")
            timepoint = time.time()
            timepoint1 = time.time()
    print("Received serial: " + input_str)
    return input_str

def newConfigString(sensors):
    configString = "newconfig:"
    for sen in sensors:
        configString += str(sen.en) + ";"
    configString += "\n"
    return configString

def newCalibString(sensors):
    calibDataString = ("newcalibdata:")
    for x in sensors:
        for y in x.parameters:
            calibDataString += str(y.value) + ","
        calibDataString += ";"
    return calibDataString

def saveChanges(newDataString, sensors):
    timepoint = time.time() - 3
    input_str = ""
    while ((not (input_str == "newdatareceived"))
            and (time.time() - timepoint < 60)):
        if (time.time() - timepoint > 3):
            dataString = newDataString(sensors)
            print("Sending new data: ")
            print(dataString)
            ser.write(dataString.encode())
            input_str = ser.readline().decode("utf-8").strip()
            timepoint = time.time()
    if (input_str == "newdatareceived"):
        print("New data sent")
        print("Value changes saved")
    else:
        print("Response waiting timeout (60 seconds)")



def configuration():
    print("Making config request to ESP32...")
    @dataclass
    class sensor:
        name: str
        en: int
    sensors = []
    input_str = sendRequestAndReceiveSerial("config\n")
    """Contoh data: Data#EC1;Tbd1;PH1;"""
    if (input_str.startswith("Data#")):
        ser.write(b"ensensorsreceived\n")
        names = re.findall("([A-z]+)[01]", input_str)
        ens = re.findall("([01]);", input_str)
        for i in range(0, len(names)):
            sensors.append(sensor(names[i], int(ens[i])))
        while ((input_str != "Y") and (input_str != "N")):
            for i, sen in enumerate(sensors):
                print(str(i+1) + ".", sen.name + ":", str(sen.en), sep = " ")
            print("Select the sensor to enable/disable (enter the number).")
            print("After finished, enter Y to keep changes,")
            input_str = input("or enter N to cancel all changes.\n")
            if (input_str.isnumeric()):
                idx = int(input_str) - 1
                sensors[idx].en = int(not sensors[idx].en )
        if (input_str == 'Y'):
            saveChanges(newConfigString, sensors)
        elif (input_str == 'N'):
            print("Value changes not saved")
            ser.write(b"cancelconfig:\n")
    else:
        print("Timeout 60 seconds, initial config data not received")

def manualCalib():
    print("Making initial calibration data request to ESP32...")
    @dataclass
    class parameter:
        name: str
        value: float
    @dataclass
    class sensor:
        sensorName: str
        parameters: list  # list of parameter
    sensors = []
    input_str = sendRequestAndReceiveSerial("manualcalib\n")
    """Contoh data: Data#EC:K Value Low (1.413 mS/cm)_1.14,K Value High (2.76 mS/cm or 12.88 mS/cm)_1.00,;
    Tbd:Opaque (2000 NTU) Voltage_2304.00,Translucent (1000 NTU) Voltage_2574.00,Transparent (0 NTU) Voltage_2772.00,;
    PH:Neutral (PH 7) Voltage_1501.00,Acid (PH 4) Voltage_2031.00,"""
    if (input_str.startswith("Data#")):
        ser.write(b"initcalibdatareceived\n")
        input_str = input_str.lstrip("Data#")
        sensors_str_arr = input_str.split(";")
        print("List of sensor: ")
        print(sensors_str_arr)
        for x in sensors_str_arr:
            temp_sensor_name = re.search("([A-z]+):", x).group(1)
            temp_parameter_names = re.findall("([A-z()./\s0-9]+)_", x)
            temp_parameter_values = re.findall("([-0-9.]+),", x)
            temp_parameters = []
            for i in range(0, len(temp_parameter_names)):
                temp_parameters.append(parameter(temp_parameter_names[i],
                                       temp_parameter_values[i]))
            sensors.append(sensor(temp_sensor_name, temp_parameters))
        while ((input_str != "Y") and (input_str != "N")):
            for i, x in enumerate(sensors):
                if i > 0:
                    print(", ", end = "")
                print(str(i+1) + ". " + x.sensorName, end = "")
            print("")
            print("Select the sensor to calibrate manually (enter the number).")
            print("After finished, enter N to cancel all changes,")
            input_str = input("or enter Y to save all changes.\n")
            if (input_str.isnumeric()):
                sensorIdx = int(input_str) - 1
            while ((input_str != "B") and (input_str != "Y") and
                   (input_str != "N")):
                print(sensors[sensorIdx].sensorName + " parameters:")
                for i, x in enumerate(sensors[sensorIdx].parameters):
                    print(str(i+1) + ". " + x.name + ": " + str(x.value))
                print("Select the parameter (enter the number).")
                print("Enter B to select another sensor.")
                print("After finished, enter N to cancel all changes,")
                input_str = input("or enter Y to save all changes.\n")
                if (input_str.isnumeric()):
                    paramIdx = int(input_str) - 1
                    input_str = input("Enter the new value of "
                                + sensors[sensorIdx].parameters[paramIdx].name + ": ")
                    sensors[sensorIdx].parameters[paramIdx].value = float(input_str)
                    print("New value of " + sensors[sensorIdx].parameters[paramIdx].name
                          + ": " + str(sensors[sensorIdx].parameters[paramIdx].value))
        if (input_str == "Y"):
            saveChanges(newCalibString, sensors)
        elif (input_str == "N"):
            print("Value changes not saved")
            ser.write(b"cancelcalib:\n")
    else:
        print("Timeout 60 seconds, initial calibration data not received")

def dataRequest():
    print("Making data request to ESP32...")
    file_name = "sensor_data.csv"
    new_file_name = "previous_sensor_data.csv"
    @dataclass
    class sensor:
        name: str
        value: float
        unit: str
    sensors = []
    input_str = sendRequestAndReceiveSerial(time.strftime("%H:%M\n", time.localtime()))
    """Contoh data: Data#Time:00:17 ;Temperature:-127.00 ;EC:-1.64 mS/cm;Tbd:2744.13 NTU;PH:15.33 ;"""
    if (input_str.startswith("Data#")):
        names = re.findall("([A-z]+):", input_str)
        values = re.findall(":([-.:0-9]+)\s", input_str)  # parsing using regex
        units = re.findall("([A-z/\s]+);", input_str)
        readTime = values[0]
        temperature = values[1]
        for i in range(2, len(values)):
            sensors.append(sensor(names[i], values[i], units[i]))
        print("Reading time: " + readTime)
        print("Temperature: " + temperature + " ^C")
        data = [readTime, temperature]
        for i, sen in enumerate(sensors):
            print(sen.name + ":", str(sen.value) + sen.unit, sep = " ")
            data.append(sen.value)
            if (sen.name == "EC"):
                names.insert(i+3, "TSS")
                TSS = round(float(sen.value)*0.123*1000 - 41.593, 2)
                print("Calculated TSS from EC: " + str(TSS) + " mg/L")
                data.append(str(TSS))
        print(names)
        print(data)
        try:
            file = open(file_name, 'a+', newline = '')
            writer = csv.writer(file)
            if os.stat(file_name).st_size == 0:
                writer.writerow(names)
            else:
                file.seek(0)
                reader = csv.reader(file)
                rows = []
                for row in reader:
                    rows.append(row)
                if (rows[0] != names):
                    file.close()
                    file_no = 0
                    while (os.path.exists(new_file_name)):
                        file_no += 1
                        new_file_name = "previous_sensor_data_" + str(file_no) + ".csv"
                    shutil.copyfile(file_name, new_file_name)
                    file = open(file_name, 'w+', newline = '')
                    writer = csv.writer(file)
                    writer.writerow(names)
            writer.writerow(data)
        finally:
            file.close()
    else:
        print("Failed to receive data (invalid format or exceeds 60 seconds timeout)...")



while (1):
    try:
        isInputValid = False
        while (not isInputValid):
            input_str = input("Type command (REQ, CALIB, or CONFIG)\n")
            time.sleep(0.2)
            GPIO.output(PIN_TO_ESP, GPIO.HIGH)  # requesting data
            if (input_str == "REQ"):
                isInputValid = True
                dataRequest()
            elif (input_str == "CALIB"):
                isInputValid = True
                manualCalib()
            elif (input_str == "CONFIG"):
                isInputValid = True
                configuration()
            else:
                print("Invalid, please retype command\n")
    finally:
        GPIO.output(PIN_TO_ESP, GPIO.LOW)  # turn off the request pin
    print("Going to sleep...")