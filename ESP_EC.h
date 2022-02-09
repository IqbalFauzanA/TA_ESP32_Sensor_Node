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

#include "ESP_Sensor.h"

class ESP_EC: public ESP_Sensor
{
public:
    ESP_EC();
    ~ESP_EC();

private:
    float  _kvalueLow;
    float  _kvalueHigh;

    float compensateRaw();
    void calibStartMessage();
    void acqCalibValue(bool* calibrationFinish);
    void voltAcq();
};

#endif
