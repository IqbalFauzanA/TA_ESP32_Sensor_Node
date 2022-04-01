/*
 * By M. Iqbal F. A.
 * Modified from:
 * file DFRobot_ESP_EC.cpp * @ https://github.com/GreenPonik/DFRobot_ESP_EC_BY_GREENPONIK
 *
 * Arduino library for Gravity: Analog EC Sensor / Meter Kit V2, SKU: DFR0300
 * Compatible for ESP32
 */

#include "ESP_EC.h"

#define KVALUEADDR 0xA //the start address of the K value stored in the EEPROM
#define EC_1413_LOW_VOLTAGE 115.0 //0.7
#define EC_1413_HIGH_VOLTAGE 295.0 //1.8
#define EC_276_LOW_VOLTAGE 320.0 //1.95
#define EC_276_HIGH_VOLTAGE 525.0 //3.2
#define EC_1288_LOW_VOLTAGE 1312.0 //8
#define EC_1288_HIGH_VOLTAGE 2755.0 //16.8
#define EC_LOW_VALUE 1.413
#define EC_HIGH_VALUE_1 2.76
#define EC_HIGH_VALUE_2 12.88
#define RES2 820.0
#define ECREF 200.0

extern OneWire oneWire;// Setup a oneWire instance to communicate with any OneWire devices
extern DallasTemperature tempSensor;// Pass our oneWire reference to Dallas Temperature sensor
extern debounceButton cal_button;
extern debounceButton mode_button;
extern Adafruit_ADS1115 ads;
extern Adafruit_SH1106G display;

ESP_EC::ESP_EC() {
    _eepromStartAddress = KVALUEADDR;
    
    //default values
    _kvalueLow = 1.0;
    _kvalueHigh = 1.0;
    _eepromCalibParamArray[0] = {"K Value Low (1.413 mS/cm)", EC_LOW_VALUE, &_kvalueLow, EC_1413_LOW_VOLTAGE, EC_1413_HIGH_VOLTAGE};
    _eepromCalibParamArray[1] = {"K Value High (2.76 mS/cm or 12.88 mS/cm)", EC_HIGH_VALUE_1, &_kvalueHigh, EC_276_LOW_VOLTAGE, EC_276_HIGH_VOLTAGE};
    _eepromCalibParamArray[2] = {"K Value High (2.76 mS/cm or 12.88 mS/cm)", EC_HIGH_VALUE_2, &_kvalueHigh, EC_1288_LOW_VOLTAGE, EC_1288_HIGH_VOLTAGE};
    
    _sensorName = "EC";
    _eepromCalibParamCount = 2;
    _sensorUnit = "mS/cm";
}

ESP_EC::~ESP_EC() {
}

//compensate raw EC with calibration value and temperature
float ESP_EC::calculateValueFromVolt() {
    tempSensor.requestTemperatures(); 
    _temperature = tempSensor.getTempCByIndex(0); //store last temperature value
    float kvalue = _kvalueLow; // set default K value: K = kvalueLow
    float value, valueTemp;
    float _rawEC = 0;
    _rawEC = 1000 * _voltage / RES2 / ECREF;
    valueTemp = _rawEC * kvalue;
    //automatic shift process
    //First Range:(0,2.5); Second Range:(2.5,20)
    //if > 2.5, kvalue high, else low (stays default)
    if (valueTemp > 2.5)
    {
        kvalue = _kvalueHigh;
    }
    
    value = _rawEC * kvalue;//calculate the EC value after automatic shift
    value = value / (1.0 + 0.0185 * (_temperature - 25.0)); //temperature compensation
    return value;
}

void ESP_EC::acquireVolt() {
    float adsvoltage;
    float voltage = 0;
    int n = 100;
    for (int i = 0; i < n; i++)
    {
        adsvoltage = ads.readADC_SingleEnded(0) / 10;
        voltage += 0.0000022091*pow(adsvoltage, 3.0) - 0.00243269*pow(adsvoltage, 2.0) + 1.74097*adsvoltage - 8.11739;
    }
    _voltage = voltage / n;
}

void ESP_EC::calibStartMessage() {
    Serial.println();
    Serial.println(F(">>>Enter EC Calibration Mode<<<"));
    Serial.println(F(">>>Please put the probe into the 1413us/cm, 2.76ms/cm, or 12.88ms/cm standard buffer solution<<<"));
    Serial.println(F(">>>Only need two point for calibration, one low (1413us/com) and one high (2.76ms/cm or 12.88ms/cm)<<<"));
    Serial.println();
}

void ESP_EC::captureCalibVolt(bool* isCalibSuccess) {
    static float compECsolution;
    float KValueTemp;
    if ((_voltage > EC_1413_LOW_VOLTAGE) && (_voltage < EC_1413_HIGH_VOLTAGE))
    {
        Serial.print(F(">>>Buffer 1.413ms/cm<<<"));                            //recognize 1.413us/cm buffer solution
        compECsolution = EC_LOW_VALUE * (1.0 + 0.0185 * (_temperature - 25.0)); //temperature compensation
        Serial.print(F(">>>compECsolution: "));
        Serial.print(compECsolution);
        Serial.println(F("<<<"));
    }
    else if ((_voltage > EC_276_LOW_VOLTAGE) && (_voltage < EC_276_HIGH_VOLTAGE))
    {
        Serial.print(F(">>>Buffer 2.76ms/cm<<<"));                            //recognize 2.76ms/cm buffer solution
        compECsolution = EC_HIGH_VALUE_1 * (1.0 + 0.0185 * (_temperature - 25.0)); //temperature compensation
        Serial.print(F(">>>compECsolution: "));
        Serial.print(compECsolution);
        Serial.println(F("<<<"));
    }
    else if ((_voltage > EC_1288_LOW_VOLTAGE) && (_voltage < EC_1288_HIGH_VOLTAGE))
    {
        Serial.print(F(">>>Buffer 12.88ms/cm<<<"));                            //recognize 12.88ms/cm buffer solution
        compECsolution = EC_HIGH_VALUE_2 * (1.0 + 0.0185 * (_temperature - 25.0)); //temperature compensation
        Serial.print(F(">>>compECsolution: "));
        Serial.print(compECsolution);
        Serial.println(F("<<<"));
    }
    else
    {
        Serial.print(F(">>>Buffer Solution Error Try Again<<<   "));
        isCalibSuccess = 0;
    }
    Serial.println();
    Serial.print(F(">>>KValueTemp calculation formule: "));
    Serial.print(F("RES2"));
    Serial.print(F(" * "));
    Serial.print(F("ECREF"));
    Serial.print(F(" * "));
    Serial.print(F("compECsolution"));
    Serial.print(F(" / 1000.0 / "));
    Serial.print(F("voltage"));
    Serial.println(F("<<<"));
    Serial.print(F(">>>KValueTemp calculation: "));
    Serial.print(RES2);
    Serial.print(F(" * "));
    Serial.print(ECREF);
    Serial.print(F(" * "));
    Serial.print(compECsolution);
    Serial.print(F(" / 1000.0 / "));
    Serial.print(_voltage);
    Serial.println(F("<<<"));
    KValueTemp = RES2 * ECREF * compECsolution / 1000.0 / _voltage; //calibrate the k value
    Serial.println();
    Serial.print(F(">>>KValueTemp: "));
    Serial.print(KValueTemp);
    Serial.println(F("<<<"));
    if ((KValueTemp > 0.5) && (KValueTemp < 2.0))
    {
        Serial.println();
        Serial.print(F(">>>Successful,K:"));
        Serial.print(KValueTemp);
        Serial.println(F(", Send EXITEC to Save and Exit<<<"));
        display.println(F("CAL. SUCCESSFUL!"));
        display.display();
        delay(500);
        if ((_voltage > EC_1413_LOW_VOLTAGE) && (_voltage < EC_1413_HIGH_VOLTAGE))
        {
            _kvalueLow = KValueTemp;
            Serial.print(">>>kvalueLow: ");
            Serial.print(_kvalueLow);
            Serial.println(F("<<<"));
        }
        else if ((_voltage > EC_276_LOW_VOLTAGE) && (_voltage < EC_276_HIGH_VOLTAGE))
        {
            _kvalueHigh = KValueTemp;
            Serial.print(">>>kvalueHigh: ");
            Serial.print(_kvalueHigh);
            Serial.println(F("<<<"));
        }
        else if ((_voltage > EC_1288_LOW_VOLTAGE) && (_voltage < EC_1288_HIGH_VOLTAGE))
        {
            _kvalueHigh = KValueTemp;
            Serial.print(">>>kvalueHigh: ");
            Serial.print(_kvalueHigh);
            Serial.println(F("<<<"));
        }
        *isCalibSuccess = 1;
    }
    else
    {
        Serial.println();
        Serial.println(F(">>>KValueTemp out of range 0.5-2.0<<<"));
        Serial.print(">>>KValueTemp: ");
        Serial.print(KValueTemp, 4);
        Serial.println("<<<");
        Serial.println(F(">>>Failed,Try Again<<<"));
        Serial.println();
        *isCalibSuccess = 0;
        display.println(F("CAL. FAILED!    "));
        display.display();
        delay(500);
    }
}
