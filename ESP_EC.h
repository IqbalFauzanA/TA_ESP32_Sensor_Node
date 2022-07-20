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

#define EC_LOW_VALUE 1.413
#define EC_HIGH_VALUE 12.88
#define RES2 900.0
#define ECREF 200.0

class ESP_EC : public ESP_Sensor
{
public:
    ESP_EC();
    ~ESP_EC();

private:
    float _lowCondVolt;
    float _highCondVolt;

    float calculateValueFromVolt();
    float compensateVoltWithTemperature();
    void captureCalibVolt(bool *calibrationFinish);
    void readAndAverageVolt();
};

#endif
