/*
 * By M. Iqbal F. A.
 * Modified from:
 * file DFRobot_ESP_EC.h * @ https://github.com/GreenPonik/DFRobot_ESP_EC_BY_GREENPONIK
 * file DFRobot_ESP_PH.h * @ https://github.com/GreenPonik/DFRobot_ESP_PH_BY_GREENPONIK
 *
 * Arduino library for Gravity: Analog Turbidity Sensor
 * Compatible for ESP32
 */

#ifndef _ESP_TURBIDITY_H_
#define _ESP_TURBIDITY_H_

#include "ESP_Sensor.h"

#define TRANSPARENT_VALUE 0.0
#define TRANSLUCENT_VALUE 304.5
#define OPAQUE_VALUE 511.5

class ESP_Turbidity : public ESP_Sensor
{
public:
    ESP_Turbidity();
    ~ESP_Turbidity();
    bool isTbdOutOfRange();

private:
    float _transparentVolt;
    float _translucentVolt;
    float _opaqueVolt;
    float _vPeak;

    float calculateValueFromVolt();
    float compensateVoltWithTemperature();
};

#endif
