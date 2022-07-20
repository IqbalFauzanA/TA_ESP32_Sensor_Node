#include "ESP_NH3N.h"

extern OneWire oneWire;              // Setup a oneWire instance to communicate with any OneWire devices
extern DallasTemperature tempSensor; // Pass our oneWire reference to Dallas Temperature sensor

ESP_NH3N::ESP_NH3N()
{
    _resetCalibratedValueToDefault = 0;

    // default values
    _eepromStartAddress = 34;
    _strongBaseVolt = 2600.0;
    _weakBaseVolt = 2400.0;

    _calibParamArray[0] = {WEAK_BASE_VALUE, &_weakBaseVolt};
    _calibParamArray[1] = {STRONG_BASE_VALUE, &_strongBaseVolt};

    _sensorName = "NH3N";
    _calibParamCount = 2;
    _sensorUnit = "mg/L";
    _sensorPin = 35;
}

ESP_NH3N::~ESP_NH3N()
{
}

float ESP_NH3N::calculateValueFromVolt()
{
    float slope = (WEAK_BASE_VALUE - STRONG_BASE_VALUE) / (_weakBaseVolt - _strongBaseVolt); // two point: (_weakBaseVoltage,7.0),(_strongBaseVoltage,4.0)
    float intercept = WEAK_BASE_VALUE - slope * _weakBaseVolt;
    float value = slope * _voltage + intercept; // y = k*x + b
    return value;
}