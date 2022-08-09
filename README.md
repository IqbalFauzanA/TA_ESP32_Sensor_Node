
# WASTEWATER QUALITY MONITORING SYSTEM - SENSOR NODES

This wastewater monitoring system consists of two main types of 
subsystems: Sensor Nodes and Sink Node. The Sensor Nodes, with 
ESP32 as processor, obtain the sensor data from the wastewater, 
display it, and send it to the Sink Node. This page will focus on 
explaining the Sensor Nodes. To see the explanation of Sink 
Node, please click this 
[link](https://github.com/IqbalFauzanA/TA_Raspi_Integrated_with_ESP32).

This system has two Sensor Nodes. Sensor Node 1 has four sensor
slots: pH/NH3-N (these two sensors can use the same slot 
alternately), electrical conductivity or EC (converted to Total 
Suspended Solids or TSS), turbidity (converted to TSS), and 
temperature (as a corrector). Sensor Node 1 uses Zigbee XBee
S2C module (set to transparent mode for UART communication)
to communicate wirelessly with Sink Node. Sensor Node 
2 has two sensor slots: pH/NH3-N and temperature. Sensor Node 2 
uses cables to communicate with Sink Node.

The Sensor Nodes have a feature that lets users calibrate
sensors while the system is still running.


## Hardware

### Schematics

The pictures below are the schematic and board for Sensor Node 1.
![Sensor Node 1](https://i.imgur.com/2B6KY8h.png)
![Sensor Node 1 Board](https://i.imgur.com/8VcT3iy.png)

The pictures below are the schematic and board for Sensor Node 2.
![Sensor Node 2](https://i.imgur.com/u23lxd9.png)
![Sensor Node 2 Board](https://i.imgur.com/GUvEcdG.png)

Please note that both schematics need to be rotated 180 degrees 
to make the OLED display readable.

### Product Links of Components

- [ESP32](https://tokopedia.link/X6qnCWEeppb)
- [pH Sensor Probe](https://dfrobot.com/product-2069.html)
- [pH/NH3-N Sensor Board](https://tokopedia.link/s8CDlPYS6qb)
- [EC Sensor Probe](https://picclick.com/E201WM-Conductivity-COND-EC-electrode-Conductivity-sensor-probe-131759489750.html)
- [EC Sensor Board](https://dfrobot.com/product-1123.html)
- [ADC (ADS1115) for EC Sensor](https://tokopedia.link/3XWeaoU4Nrb)
- [Turbidity Sensor Probe & Board](https://wiki.dfrobot.com/Turbidity_sensor_SKU__SEN0189)
- [NH3-N Sensor Probe](https://id.aliexpress.com/item/32846294005.html?)
- [Temperature Sensor Probe](https://id.aliexpress.com/item/32846294005.html?)
- [OLED Mini Display](https://tokopedia.link/HVQTn6CW4qb)
- [ZigBee XBee S2C](https://tokopedia.link/g438vpVY4qb)
- [Power Supply Module](https://tokopedia.link/DWuWflu14qb)
- [Adaptor for Power Supply Module](https://tokopedia.link/or9EsNO14qb)
## Firmware Preparation

1. Download this repository and put all the files into a folder named "TA_ESP32_Sensors_Integrated_with_Raspi".
2. Open the file "TA_ESP32_Sensors_Integrated_with_Raspi.ino" using Arduino IDE. 
3. Install these libraries through the library manager.


| Library Name          | Author          | Library Usage      |
| :-------------------- | :-------------: | :----------------: |
| `DallasTemperature.h` | Miles Burton    | Temperature sensor |
| `OneWire.h`           | Paul Stoffregen | Temperature sensor |
| `Adafruit_GFX.h`      | Adafruit        | OLED Display       |
| `Adafruit_SH110X.h`   | Adafruit        | OLED Display       |


4. Upload to ESP32.

## Calibrating Sensors

1. Insert the temperature sensor probe and the one you want to calibrate into the container of one of the calibration solutions.
2. From sleep mode, to wake up the Sensor Node, press the CAL button until the sensor selection menu appears on the OLED display.
3. Press the MODE button to switch sensors. After the words "[sensor name] Calibration" correspond to the sensor you want to calibrate, press the CAL button to enter the calibration solution selection menu.
4. Press the MODE button to switch the calibration solution. After the writing on the OLED display matches the solution being calibrated, press the CAL button to enter calibration mode.
5. Wait for the reading to stabilize. Press the CAL button to temporarily store the calibration value. Press the MODE button to exit calibration mode and return to the calibration solution selection menu.
6. Transfer the sensor probe to another calibration solution. Repeat steps 4 and 5 for this calibration solution. Do this for all available calibration solutions for the sensor.
7. After the calibration is complete for all solutions, on the solution selection menu, press the MODE button until the display shows "Save & Exit". Press the CAL button to return to the main menu and store the calibration values ​​in the EEPROM memory so that the calibration values ​​can still be accessed even after the system is turned off and on again. You can also select "Cancel & Exit" to cancel values ​​that have not been stored in the EEPROM memory.
8. From the main menu, other sensors can be selected for calibration. To put the system into deep sleep mode, select “Exit” on the main menu.
9. To check the calibration value, restart the calibration mode and pay attention to the value shown on the display.
\
After calibration, keep the Sensor Node powered and put all the sensors into the liquid which will be observed. The Sensor Node will perform data reading after receiving a signal from the Sink Node.

## Continuation

This page is the first part of the project explanation. Click this
[link](https://github.com/IqbalFauzanA/TA_Raspi_Integrated_with_ESP32)
to see the second part.

