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

    _eepromStartAddress = KVALUEADDR;

    // default values
    _kvalueLow = 1.0;
    _kvalueHigh = 1.0;
    _eepromCalibParamArray[0] = {"K Value Low (1.413 mS/cm)", EC_LOW_VALUE, &_kvalueLow, EC_1413_LOW_VOLTAGE, EC_1413_HIGH_VOLTAGE};
    _eepromCalibParamArray[1] = {"K Value High (2.76 mS/cm or 12.88 mS/cm)", EC_HIGH_VALUE_1, &_kvalueHigh, EC_276_LOW_VOLTAGE, EC_276_HIGH_VOLTAGE};
    _eepromCalibParamArray[2] = {"K Value High (2.76 mS/cm or 12.88 mS/cm)", EC_HIGH_VALUE_2, &_kvalueHigh, EC_1288_LOW_VOLTAGE, EC_1288_HIGH_VOLTAGE};

    _sensorName = "EC";
    _eepromCalibParamCount = 2;
    _sensorUnit = "mS/cm";
}

ESP_EC::~ESP_EC()
{
}

// compensate raw EC with calibration value and temperature
float ESP_EC::calculateValueFromVolt()
{
    float kvalue = _kvalueLow; // set default K value: K = kvalueLow
    float value, valueTemp;
    float _rawEC = 0;
    _rawEC = 1000 * _voltage / RES2 / ECREF;
    valueTemp = _rawEC * kvalue;
    // automatic shift process
    // First Range:(0,2.5); Second Range:(2.5,20)
    // if > 2.5, kvalue high, else low (stays default)
    if (valueTemp > 2.5)
    {
        kvalue = _kvalueHigh;
    }
    value = _rawEC * kvalue; // calculate the EC value after automatic shift
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

void ESP_EC::calibStartMessage()
{
    Serial.println();
    Serial.println(F(">>>Enter EC Calibration Mode<<<"));
    Serial.println(F(">>>Please put the probe into the 1413us/cm, 2.76ms/cm, or 12.88ms/cm standard buffer solution<<<"));
    Serial.println(F(">>>Only need two point for calibration, one low (1413us/com) and one high (2.76ms/cm or 12.88ms/cm)<<<"));
    Serial.println();
}

float ESP_EC::calculateCalibValue(float solutionValue, float voltage)
{
    return RES2 * ECREF * solutionValue / 1000.0 / voltage;
}