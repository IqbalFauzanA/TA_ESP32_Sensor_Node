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

class ESP_Turbidity: public ESP_Sensor
{
public:
    ESP_Turbidity();
    ~ESP_Turbidity();
    bool isTbdOutOfRange();

private:
    float _transparentVoltage;
    float _translucentVoltage;
    float _opaqueVoltage;
    float _vPeak;
    
    float calculateValueFromVolt();
    void calibStartMessage();
    float compensateVoltWithTemperature();
};

#endif
