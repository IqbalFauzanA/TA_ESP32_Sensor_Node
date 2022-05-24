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

#define TBDVALUEADDR 20
#define TRANSPARENT_HIGH_VOLTAGE 3300.0
#define TRANSPARENT_LOW_VOLTAGE 2610.0
#define TRANSLUCENT_HIGH_VOLTAGE 2600.0
#define TRANSLUCENT_LOW_VOLTAGE 2455.0
#define OPAQUE_HIGH_VOLTAGE 2449.0
#define OPAQUE_LOW_VOLTAGE 2138.0
#define TRANSPARENT_VALUE 0.0
#define TRANSLUCENT_VALUE 1000.0
#define OPAQUE_VALUE 2000.0
#define TBD_SENSOR 32 //turbidity sensor pin

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
        float compensateVoltWithTemperature();
        void calibStartMessage();
};

#endif
