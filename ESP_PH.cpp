/*
 * By M. Iqbal F. A.
 * Modified from:
 * file DFRobot_ESP_PH.cpp * @ https://github.com/GreenPonik/DFRobot_ESP_PH_BY_GREENPONIK
 *
 * Arduino library for Gravity: Analog pH Sensor / Meter Kit V2, SKU: SEN0161-V2
 * Compatible for ESP32
 */

#include "ESP_PH.h"



extern OneWire oneWire;// Setup a oneWire instance to communicate with any OneWire devices
extern DallasTemperature tempSensor;// Pass our oneWire reference to Dallas Temperature sensor


ESP_PH::ESP_PH() {
    _resetCalibratedValueToDefault = 0;

    _eepromStartAddress = PHVALUEADDR;
    _acidVoltage = 1150.0;   //buffer solution 4.0 at 25C 
    _neutralVoltage = 1500.0; //buffer solution 7.0 at 25C 
    
    _eepromCalibParamArray[0] = {"Neutral (PH 7) Voltage", NEUTRAL_VALUE, &_neutralVoltage, NEUTRAL_LOW_VOLTAGE, NEUTRAL_HIGH_VOLTAGE};
    _eepromCalibParamArray[1] = {"Acid (PH 4) Voltage", ACID_VALUE, &_acidVoltage, ACID_LOW_VOLTAGE, ACID_HIGH_VOLTAGE};
    _eepromCalibParamArray[2] = {"", 0, 0, 0, 0};
    
    _sensorName = "PH";
    _eepromCalibParamCount = 2;
    _sensorUnit = "";
    _sensorPin = PH_SENSOR;
}

ESP_PH::~ESP_PH() {
}

float ESP_PH::calculateValueFromVolt() {
    float slope = (NEUTRAL_VALUE - ACID_VALUE) / (_neutralVoltage - _acidVoltage); // two point: (_neutralVoltage,7.0),(_acidVoltage,4.0)
    float intercept = NEUTRAL_VALUE - slope * _neutralVoltage;
    float value = slope * _voltage + intercept; //y = k*x + b
    return value;
}

float ESP_PH::compensateVoltWithTemperature() {
    float voltage;
    tempSensor.requestTemperatures(); 
    _temperature = tempSensor.getTempCByIndex(0);
    voltage = 1500 + (_voltage - 1500) * (298.15 / (_temperature + 273.15));
    return voltage;
}

void ESP_PH::calibStartMessage() {
    Serial.println();
    Serial.println(F(">>>Enter PH Calibration Mode<<<"));
    Serial.println(F(">>>Please put the probe into the 4.0 or 7.0 standard buffer solution.<<<"));
    Serial.println(F(">>>Calibrate all for the best result.<<<"));
    Serial.println();
}
