#ifndef _ESP_SENSOR_H_
#define _ESP_SENSOR_H_

#include "Arduino.h"
#include "EEPROM.h"
#include <DallasTemperature.h>
#include <OneWire.h>
#include "debounceButton.h"
#include <math.h>
#include "Adafruit_ADS1015.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define CALCULATE_PERIOD 5000U

class ESP_Sensor
{
public:

    ESP_Sensor();
    ~ESP_Sensor();
    
    void calibration(byte* state);
    float getValue();
    void acquireValueFromVolt();
    float getTemperature();
    void begin();
    void saveNewConfig();
    void saveNewCalib();
    void displayTwoLines(String firstLine, String secondLine);

    String _sensorName;
    String _sensorUnit;
    int _eepromCalibParamCount; //the amount of value in eeprom array for each sensor

    virtual bool isTbdOutOfRange();

    bool _enableSensor;

    struct eepromCalibParam
    {
        String name;
        float paramValue;
        float *value;
        float lowerBound;
        float upperBound;
    }_eepromCalibParamArray[3];
    
protected:

    float _value;
    float _voltage;
    float _temperature;
    int _eepromStartAddress;
    int _eepromAddress;
    int _sensorPin;

    void calibDisplay();

    virtual void tempCompVolt();
    virtual void captureCalibVolt(bool* calibrationFinish); //to facilitate EC difference
    virtual void saveCalibVoltAndExit(bool* calibrationFinish);
    virtual void acquireVolt(); //to facilitate EC difference
    virtual void calibStartMessage() = 0;
    virtual float calculateValueFromVolt() = 0;
};

#endif
