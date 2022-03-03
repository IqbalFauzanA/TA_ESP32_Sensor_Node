#ifndef _ESP_SENSOR_H_
#define _ESP_SENSOR_H_

#include "Arduino.h"
#include "EEPROM.h"
#include <DallasTemperature.h>
#include <OneWire.h>
#include <LiquidCrystal_I2C.h>
#include "debounceButton.h"
#include <math.h>
#include "Adafruit_ADS1015.h"

#define CALCULATE_PERIOD 5000U

class ESP_Sensor
{
public:

    ESP_Sensor();
    ~ESP_Sensor();
    
    void calState(byte* state);
    float getValue();
    void calculateValue();
    float getTemperature();
    void begin();
    void saveNewConfig();
    void saveNewCalib();
    void lcdDisplay(String firstLine, String secondLine);
    void lcdFirstLine(String line);
    void lcdSecondLine(String line);

    String _paramName;
    String _unit;
    int _eepromN; //the amount of value in eeprom array for each sensor

    virtual bool isTbdOutOfRange();

    bool _enableSensor;

    struct eeprom
    {
        String name;
        float paramValue;
        float *value;
        float lowerBound;
        float upperBound;
    }_calibSolutionArr[3];
    
protected:

    float _value;
    float _voltage;
    float _temperature;
    byte _calMode;
    bool _isCalib;
    int _eepromStartAddress;
    int _eepromAddress;
    int m; //repetition to acquire temperature value
    int n; //repetition to acquire voltage value 
    int _sensorPin;

    void buttonParse();
    void lcdCal();
    void calibration();

    virtual void tempCompVolt();
    virtual void acqCalibValue(bool* calibrationFinish); //to facilitate EC difference
    virtual void voltAcq(); //to facilitate EC difference
    virtual void calibStartMessage() = 0;
    virtual float compensateRaw() = 0;
};

#endif
