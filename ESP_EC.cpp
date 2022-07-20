/*
 * By M. Iqbal F. A.
 * Modified from:
 * file DFRobot_ESP_EC.cpp * @ https://github.com/GreenPonik/DFRobot_ESP_EC_BY_GREENPONIK
 *
 * Arduino library for Gravity: Analog EC Sensor / Meter Kit V2, SKU: DFR0300
 * Compatible for ESP32
 */

#include "ESP_EC.h"

Adafruit_ADS1115 ads;
extern OneWire oneWire;              // Setup a oneWire instance to communicate with any OneWire devices
extern DallasTemperature tempSensor; // Pass our oneWire reference to Dallas Temperature sensor

ESP_EC::ESP_EC()
{
    _resetCalibratedValueToDefault = 0;

    ads.setGain(GAIN_ONE);
    ads.begin();

    _eepromStartAddress = 10; // the start address of the EC calb. param. stored in the EEPROM

    // default values
    _lowCondVolt = 300;
    _highCondVolt = 2400;
    _calibParamArray[0] = {EC_LOW_VALUE, &_lowCondVolt};
    _calibParamArray[1] = {EC_HIGH_VALUE, &_highCondVolt};

    _sensorName = "EC";
    _calibParamCount = 2;
    _sensorUnit = "mS/cm";
}

ESP_EC::~ESP_EC()
{
}

// compensate raw EC with calibration value and temperature
float ESP_EC::calculateValueFromVolt()
{
    float kValueLow = RES2 * ECREF * EC_LOW_VALUE / 1000.0 / _lowCondVolt;
    float kValueHigh = RES2 * ECREF * EC_HIGH_VALUE / 1000.0 / _highCondVolt;
    float value, valueTemp;
    float _rawEC = 1000 * _voltage / RES2 / ECREF;
    valueTemp = _rawEC * kValueLow; // use default K value (kvalueLow)
    // automatic shift process
    // First Range:(0,2.5); Second Range:(2.5,20)
    // if > 2.5, kvalue high, else low (stays default)
    if (valueTemp > 2.5)
    {
        value = _rawEC * kValueHigh;
    }
    else
    {
        value = _rawEC * kValueLow;
    }
    return value;
}

float ESP_EC::compensateVoltWithTemperature()
{
    float voltage;
    tempSensor.requestTemperatures();
    _temperature = tempSensor.getTempCByIndex(0);                // store last temperature value
    voltage = _voltage / (1.0 + 0.0185 * (_temperature - 25.0)); // temperature compensation
    return voltage;
}

void ESP_EC::readAndAverageVolt()
{
    float adsvoltage;
    float voltage = 0;
    int n = 100;
    for (int i = 0; i < n; i++)
    {
        adsvoltage = ads.readADC_SingleEnded(0) / 10;
        voltage += 0.0000022091 * pow(adsvoltage, 3.0) - 0.00243269 * pow(adsvoltage, 2.0) + 1.74097 * adsvoltage - 8.11739;
    }
    _voltage = voltage / n;
}