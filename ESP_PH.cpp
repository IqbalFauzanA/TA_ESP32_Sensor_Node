/*
 * By M. Iqbal F. A.
 * Modified from:
 * file DFRobot_ESP_PH.cpp * @ https://github.com/GreenPonik/DFRobot_ESP_PH_BY_GREENPONIK
 *
 * Arduino library for Gravity: Analog pH Sensor / Meter Kit V2, SKU: SEN0161-V2
 * Compatible for ESP32
 */

#include "ESP_PH.h"

extern OneWire oneWire;              // Setup a oneWire instance to communicate with any OneWire devices
extern DallasTemperature tempSensor; // Pass our oneWire reference to Dallas Temperature sensor

ESP_PH::ESP_PH()
{
    _resetCalibratedValueToDefault = 0;

    _eepromStartAddress = 0; // the start address of the pH calibration parameters stored in the EEPROM
    _acidVolt = 1215.0;    // buffer solution 4.01 at 25C
    _neutralVolt = 1600.0; // buffer solution 6.86 at 25C

    _calibParamArray[0] = {NEUTRAL_VALUE, &_neutralVolt};
    _calibParamArray[1] = {ACID_VALUE, &_acidVolt};

    _sensorName = "PH";
    _calibParamCount = 2;
    _sensorUnit = "";
    _sensorPin = 35;
}

ESP_PH::~ESP_PH()
{
}

float ESP_PH::calculateValueFromVolt()
{
    float slope = (NEUTRAL_VALUE - ACID_VALUE) / (_neutralVolt - _acidVolt); // two point: (_neutralVoltage,7.0),(_acidVoltage,4.0)
    float intercept = NEUTRAL_VALUE - slope * _neutralVolt;
    float value = slope * _voltage + intercept; // y = k*x + b
    return value;
}

float ESP_PH::compensateVoltWithTemperature()
{
    float voltage;
    tempSensor.requestTemperatures();
    _temperature = tempSensor.getTempCByIndex(0);
    voltage = 1500 + (_voltage - 1500) * (298.15 / (_temperature + 273.15));
    return voltage;
}
