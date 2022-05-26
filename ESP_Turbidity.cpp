/*
 * By M. Iqbal F. A.
 * Modified from:
 * file DFRobot_ESP_EC.cpp * @ https://github.com/GreenPonik/DFRobot_ESP_EC_BY_GREENPONIK
 * file DFRobot_ESP_PH.cpp * @ https://github.com/GreenPonik/DFRobot_ESP_PH_BY_GREENPONIK
 *
 * Arduino library for Gravity: Analog Turbidity Sensor
 * Compatible for ESP32
 */

#include "ESP_Turbidity.h"

extern OneWire oneWire;// Setup a oneWire instance to communicate with any OneWire devices
extern DallasTemperature tempSensor;// Pass our oneWire reference to Dallas Temperature sensor

ESP_Turbidity::ESP_Turbidity() {
    _resetCalibratedValueToDefault = 0;

    _eepromStartAddress = TBDVALUEADDR;
    
    //default values
    _transparentVoltage = 2600;   //solution 0.0 NTU at 25C 2460
    _translucentVoltage = 2450; //solution 304.5 NTU at 25C 2320
    _opaqueVoltage = 2300; //solution 511.5 NTU at 25C 2170
    _eepromCalibParamArray[0] = {"Opaque (511.5 NTU) Voltage", OPAQUE_VALUE, &_opaqueVoltage, OPAQUE_LOW_VOLTAGE, OPAQUE_HIGH_VOLTAGE};
    _eepromCalibParamArray[1] = {"Translucent (304.5 NTU) Voltage", TRANSLUCENT_VALUE, &_translucentVoltage, TRANSLUCENT_LOW_VOLTAGE, TRANSLUCENT_HIGH_VOLTAGE};
    _eepromCalibParamArray[2] = {"Transparent (0 NTU) Voltage", TRANSPARENT_VALUE, &_transparentVoltage, TRANSPARENT_LOW_VOLTAGE, TRANSPARENT_HIGH_VOLTAGE};

    _sensorName = "Tbd";
    _eepromCalibParamCount = 3;
    _sensorUnit = "NTU";
    _sensorPin = TBD_SENSOR;
}

ESP_Turbidity::~ESP_Turbidity() {
}

bool ESP_Turbidity::isTbdOutOfRange() {
    if (_voltage < _vPeak)
    {
        if (isnan(_value))
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return false;
    }
}

float ESP_Turbidity::calculateValueFromVolt() {
    float value = 0;
    float x1 = _opaqueVoltage; //opaque voltage
    float y1 = OPAQUE_VALUE; //opaque NTU
    float x2 = _translucentVoltage; //translucent voltage
    float y2 = TRANSLUCENT_VALUE; //translucent NTU
    float x3 = _transparentVoltage;
    float y3 = TRANSPARENT_VALUE; //transparent NTU
    
    float denom = (x1-x2) * (x1-x3) * (x2-x3);
    float a = (x3 * (y2-y1) + x2 * (y1-y3) + x1 * (y3-y2)) / denom;
    float b = (x3*x3 * (y1-y2) + x2*x2 * (y3-y1) + x1*x1 * (y2-y3)) / denom;
    float c = (x2 * x3 * (x2-x3) * y1+x3 * x1 * (x3-x1) * y2+x1 * x2 * (x1-x2) * y3) / denom;
    
    _vPeak = -b/(2*a);
    float ntu_peak = a*pow(_vPeak, 2.0) + b*_vPeak + c;
    if (isTbdOutOfRange())
    {
        value = ntu_peak;
    }
    else
    {
        value = a*pow(_voltage, 2.0) + b*_voltage + c; //y = a*x^2 + b*x + c
    }
    return value;
}

float ESP_Turbidity::compensateVoltWithTemperature() {
    float voltage;
    tempSensor.requestTemperatures(); 
    _temperature = tempSensor.getTempCByIndex(0); //store last temperature value
    voltage = (1455*_voltage-3795*_temperature+94875)/(2*_temperature+1405);
    return voltage;
}

void ESP_Turbidity::calibStartMessage() {
    Serial.println();
    Serial.println(F(">>>Enter Turbidity Calibration Mode<<<"));
    Serial.println(F(">>>Please put the probe into the 0.0, 1000.0, or 2000.0 NTU standard solution.<<<"));
    Serial.println(F(">>>Calibrate all for best result.<<<"));
    Serial.println();
}
