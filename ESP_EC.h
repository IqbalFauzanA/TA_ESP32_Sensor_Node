/*
 * By M. Iqbal F. A.
 * Modified from:
 * file DFRobot_ESP_EC.h * @ https://github.com/GreenPonik/DFRobot_ESP_EC_BY_GREENPONIK
 *
 * Arduino library for Gravity: Analog EC Sensor / Meter Kit V2, SKU: DFR0300
 * Compatible for ESP32
 */

#ifndef _ESP_EC_H_
#define _ESP_EC_H_

#include "Adafruit_ADS1015.h"
#include "ESP_Sensor.h"

#define KVALUEADDR 10               // the start address of the K value stored in the EEPROM
#define EC_1413_LOW_VOLTAGE 0.0   // 0.7
#define EC_1413_HIGH_VOLTAGE 350.0  // 1.8
#define EC_276_LOW_VOLTAGE 350.0    // 1.95
#define EC_276_HIGH_VOLTAGE 600.0   // 3.2
#define EC_1288_LOW_VOLTAGE 600.0   // 8
#define EC_1288_HIGH_VOLTAGE 3300.0 // 16.8
#define EC_LOW_VALUE 1.413
#define EC_HIGH_VALUE_1 2.76
#define EC_HIGH_VALUE_2 12.88
#define RES2 900.0
#define ECREF 200.0

class ESP_EC : public ESP_Sensor
{
public:
    ESP_EC();
    ~ESP_EC();

private:
    float _kvalueLow;
    float _kvalueHigh;

    float calculateValueFromVolt();
    float compensateVoltWithTemperature();
    void calibStartMessage();
    void captureCalibVolt(bool *calibrationFinish);
    void readAndAverageVolt();
    float calculateCalibValue(float solutionValue, float voltage);
    bool isCalibrationTemporaryValueValid(float eepromTemporaryValue);
};

#endif
