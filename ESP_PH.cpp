/*
 * By M. Iqbal F. A.
 * Modified from:
 * file DFRobot_ESP_PH.cpp * @ https://github.com/GreenPonik/DFRobot_ESP_PH_BY_GREENPONIK
 *
 * Arduino library for Gravity: Analog pH Sensor / Meter Kit V2, SKU: SEN0161-V2
 * Compatible for ESP32
 */

#include "ESP_PH.h"

#define PHVALUEADDR 0x00 //the start address of the pH calibration parameters stored in the EEPROM
#define PH_8_VOLTAGE 1122.0
#define PH_6_VOLTAGE 1478.0
#define PH_5_VOLTAGE 1654.0
#define PH_3_VOLTAGE 2010.0
#define NEUTRAL_VALUE 7.0
#define ACID_VALUE 4.0
#define PH_SENSOR 35 //pH sensor pin

extern LiquidCrystal_I2C lcd;
extern debounceButton cal_button;
extern debounceButton mode_button;

ESP_PH::ESP_PH()
{
    _eepromStartAddress = PHVALUEADDR;

    //default values
    _acidVoltage = 2032.44;   //buffer solution 4.0 at 25C
    _neutralVoltage = 1500.0; //buffer solution 7.0 at 25C
    _calibSolutionArr[0] = {"Neutral (PH 7) Voltage", NEUTRAL_VALUE, &_neutralVoltage, PH_8_VOLTAGE, PH_6_VOLTAGE};
    _calibSolutionArr[1] = {"Acid (PH 4) Voltage", ACID_VALUE, &_acidVoltage, PH_5_VOLTAGE, PH_3_VOLTAGE};
    _calibSolutionArr[2] = {"", 0, 0, 0, 0};
    
    _paramName = "PH";
    _eepromN = 2;
    _unit = "";
    _sensorPin = PH_SENSOR;
}

ESP_PH::~ESP_PH()
{
}

float ESP_PH::compensateRaw()
{

    float slope = (NEUTRAL_VALUE - ACID_VALUE) / ((_neutralVoltage - 1500.0) / 3.0 - (_acidVoltage - 1500.0) / 3.0); // two point: (_neutralVoltage,7.0),(_acidVoltage,4.0)
    float intercept = NEUTRAL_VALUE - slope * (_neutralVoltage - 1500.0) / 3.0;;
    float value = slope * (_voltage - 1500.0) / 3.0 + intercept; //y = k*x + b
    return value;
}

void ESP_PH::calibStartMessage()
{
    Serial.println();
    Serial.println(F(">>>Enter PH Calibration Mode<<<"));
    Serial.println(F(">>>Please put the probe into the 4.0 or 7.0 standard buffer solution.<<<"));
    Serial.println(F(">>>Calibrate all for best result.<<<"));
    Serial.println();
}
