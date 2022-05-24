#include "ESP_NH3N.h"

extern OneWire oneWire;// Setup a oneWire instance to communicate with any OneWire devices
extern DallasTemperature tempSensor;// Pass our oneWire reference to Dallas Temperature sensor

ESP_NH3N::ESP_NH3N() {
    _resetCalibratedValueToDefault = 0;

    //default values
    if (_sensorNodeNumber == 0) {
        //for sensor node 0
        WEAK_BASE_LOW_VOLTAGE = 347.6;
        WEAK_BASE_HIGH_VOLTAGE = 392.5;
        STRONG_BASE_LOW_VOLTAGE = 302.5;
        STRONG_BASE_HIGH_VOLTAGE = 347.4;
        _eepromStartAddress = NH3NVALUEADDR;
        _strongBaseVoltage = 325.0;   //strong base default votlage
        _weakBaseVoltage = 370.0; //weak base default voltage
    } else if (_sensorNodeNumber == 1) {
        //for sensor node 1
        WEAK_BASE_HIGH_VOLTAGE = 2499.0;
        WEAK_BASE_LOW_VOLTAGE = 2300.0;
        STRONG_BASE_HIGH_VOLTAGE = 2700.0;
        STRONG_BASE_LOW_VOLTAGE = 2501.0;
        _eepromStartAddress = 90;
        _strongBaseVoltage = 2600.0; 
        _weakBaseVoltage = 2400.0; 
    }
    _eepromCalibParamArray[0] = {"Weak Base Voltage", WEAK_BASE_VALUE, &_weakBaseVoltage, WEAK_BASE_LOW_VOLTAGE, WEAK_BASE_HIGH_VOLTAGE};
    _eepromCalibParamArray[1] = {"Strong Base Voltage", STRONG_BASE_VALUE, &_strongBaseVoltage, STRONG_BASE_LOW_VOLTAGE, STRONG_BASE_HIGH_VOLTAGE};
    _eepromCalibParamArray[2] = {"", 0, 0, 0, 0};
    
    _sensorName = "NH3N";
    _eepromCalibParamCount = 2;
    _sensorUnit = "mg/L";
    _sensorPin = NH3N_SENSOR;
}

ESP_NH3N::~ESP_NH3N() {
}

float ESP_NH3N::calculateValueFromVolt() {
    float slope = (WEAK_BASE_VALUE - STRONG_BASE_VALUE) / (_weakBaseVoltage - _strongBaseVoltage); // two point: (_weakBaseVoltage,7.0),(_strongBaseVoltage,4.0)
    float intercept = WEAK_BASE_VALUE - slope * _weakBaseVoltage;
    float value = slope * _voltage + intercept; //y = k*x + b
    return value;
}

void ESP_NH3N::calibStartMessage() {
    Serial.println();
    Serial.println(F(">>>Enter NH3N Calibration Mode<<<"));
    Serial.println(F(">>>Please put the probe into the weak or strong base standard buffer solution.<<<"));
    Serial.println(F(">>>Calibrate all for the best result.<<<"));
    Serial.println();
}