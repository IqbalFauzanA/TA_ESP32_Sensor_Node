#include "ESP_Sensor.h"

extern OneWire oneWire;// Setup a oneWire instance to communicate with any OneWire devices
extern DallasTemperature tempSensor;// Pass our oneWire reference to Dallas Temperature sensor 
extern Adafruit_SH1106G display;
extern debounceButton cal_button;
extern debounceButton mode_button;

ESP_Sensor::ESP_Sensor() {
}

ESP_Sensor::~ESP_Sensor() {
}

void ESP_Sensor::begin() {
    _eepromAddress = _eepromStartAddress;
    for (int i = 0; i < _eepromCalibParamCount; i++) {
        float default_value = *_eepromCalibParamArray[i].value; //set default_value with initial value
        *_eepromCalibParamArray[i].value = EEPROM.readFloat(_eepromAddress); //read the calibrated value from EEPROM
        if (*_eepromCalibParamArray[i].value == float() || isnan(*_eepromCalibParamArray[i].value)) {
            *_eepromCalibParamArray[i].value = default_value; // For new EEPROM, write default value to EEPROM
            EEPROM.writeFloat(_eepromAddress, *_eepromCalibParamArray[i].value);
            EEPROM.commit();
            Serial.print(*_eepromCalibParamArray[i].value);
            Serial.print(F(" written in EEPROM."));
        }
        _eepromAddress = _eepromAddress + (int)sizeof(float);
        Serial.print(_sensorName);
        Serial.print(" ");
        Serial.print(_eepromCalibParamArray[i].name);
        Serial.print(F(" in EEPROM: "));
        Serial.println(*_eepromCalibParamArray[i].value);
    }

    //check if sensor is set as enabled in EEPROM or not
    _enableSensor = EEPROM.read(_eepromAddress);
    Serial.print(_sensorName);
    if (_enableSensor == 0) {
        Serial.print(F(" sensor disabled in EEPROM: "));
    } 
    else if (_enableSensor == 1) {
        Serial.print(F(" sensor enabled in EEPROM: "));
    }
    //if EEPROM is uninitialized 
    else {
        _enableSensor = 1;
        saveNewConfig();
        Serial.print(F(" sensor initialized as enabled in EEPROM: "));
    }
    Serial.println(_enableSensor);

    tempSensor.requestTemperatures(); 
    _temperature = tempSensor.getTempCByIndex(0);
}

float ESP_Sensor::getTemperature() {
    return _temperature;
}

void ESP_Sensor::displayTwoLines(String firstLine, String secondLine) {
    display.clearDisplay();
    display.setCursor(0,2);
    display.println(firstLine);
    display.println(secondLine);
    display.display();
}

//function for changing calibration state
void ESP_Sensor::calibration(byte* sensor) {
    if (_enableSensor) {
        static bool isCalibSuccess = false;
        static bool isPressed = false;
        static bool isCalibrating = false;
        static unsigned long timepoint;
        if (cal_button.isPressed()) {
            isPressed = true;
        }
        //STILL OUTSIDE CALIB MODE
        if (isCalibrating == false) { 
            displayTwoLines(F("Select mode:"), _sensorName + F(" Calibration"));
            if (isPressed && cal_button.isReleased()) { //ENTER CALIB MODE
                calibStartMessage();
                isCalibrating = true;
                isCalibSuccess = false;
                isPressed = false;
                timepoint =  millis() - CALCULATE_PERIOD;
            }
            else if (mode_button.isReleased()) {
                (*sensor)++; //move to next sensor
            }
        }
        //INSIDE CALIB MODE
        else if (isCalibrating == true) {
            if (isPressed && cal_button.isReleased()) { //CAPTURE CALIB VOLT
                captureCalibVolt(&isCalibSuccess);
                isPressed = false;
            }
            else if (mode_button.isReleased()) { //EXIT CALIB MODE
                saveCalibVoltAndExit(&isCalibSuccess);
                isCalibrating = false;
            }
            if (millis() - timepoint > CALCULATE_PERIOD) {
                acquireValueFromVolt();
                calibDisplay();
                timepoint = millis();
            }
        }
    }
    else {
        (*sensor)++; //skip disabled sensor
    }
}

void ESP_Sensor::captureCalibVolt(bool* isCalibSuccess) {
    tempCompVolt(); //voltage temperature compensation
    if ((_eepromCalibParamArray[0].lowerBound < _voltage) && (_voltage < _eepromCalibParamArray[0].upperBound)) {
        Serial.println();
        Serial.print(F(">>>Solution: "));
        Serial.print(_eepromCalibParamArray[0].paramValue);
        Serial.print(F(" "));
        Serial.print(_sensorUnit);
        Serial.println(F("<<<"));
        *_eepromCalibParamArray[0].value = _voltage;
        Serial.println();
        *isCalibSuccess = 1;
        display.println(F("CAL. SUCCESSFUL!"));
        display.display();
        delay(500);
    } 
    else if ((_eepromCalibParamArray[1].lowerBound < _voltage) && (_voltage < _eepromCalibParamArray[1].upperBound)) {
        Serial.println();
        Serial.print(F(">>>Solution: "));
        Serial.println(_eepromCalibParamArray[1].paramValue);
        Serial.print(F(" "));
        Serial.print(_sensorUnit);
        Serial.println(F("<<<"));
        *_eepromCalibParamArray[1].value = _voltage;
        Serial.println();
        *isCalibSuccess = 1;
        display.println(F("CAL. SUCCESSFUL!"));
        display.display();
        delay(500);
    }
    else if ((_eepromCalibParamArray[2].lowerBound < _voltage) && (_voltage < _eepromCalibParamArray[2].upperBound)) {
        Serial.println();
        Serial.print(F(">>>Solution: "));
        Serial.println(_eepromCalibParamArray[2].paramValue);
        Serial.print(F(" "));
        Serial.print(_sensorUnit);
        Serial.println(F("<<<"));
        *_eepromCalibParamArray[2].value = _voltage;
        Serial.println();
        *isCalibSuccess = 1;
        display.println(F("CAL. SUCCESSFUL!"));
        display.display();
        delay(500);
    }
    else {
        Serial.println();
        Serial.println(F(">>>Buffer Solution Error, Try Again<<<"));
        Serial.println();
        display.println(F("CAL. FAILED!"));
        display.display();
        delay(500);
    }
}

void ESP_Sensor::saveCalibVoltAndExit(bool* isCalibSuccess) {
    if (*isCalibSuccess) {
        if ((_eepromCalibParamArray[0].lowerBound < _voltage) && (_voltage < _eepromCalibParamArray[0].upperBound)) {
            EEPROM.writeFloat(_eepromStartAddress, *_eepromCalibParamArray[0].value);
            EEPROM.commit();
        } 
        else if ((_eepromCalibParamArray[1].lowerBound < _voltage) && (_voltage < _eepromCalibParamArray[1].upperBound)) {
            EEPROM.writeFloat(_eepromStartAddress + (int)sizeof(float), *_eepromCalibParamArray[1].value);
            EEPROM.commit();
        }
        else if ((_eepromCalibParamArray[2].lowerBound < _voltage) && (_voltage < _eepromCalibParamArray[2].upperBound)) {
            //for EC
            if (_eepromCalibParamCount == 2) {
                EEPROM.writeFloat(_eepromStartAddress + (int)sizeof(float), *_eepromCalibParamArray[2].value);
                EEPROM.commit();
            }
            //for tbd
            else if (_eepromCalibParamCount == 3) {
                EEPROM.writeFloat(_eepromStartAddress + (int)sizeof(float) + (int)sizeof(float), *_eepromCalibParamArray[2].value);
                EEPROM.commit();
            }
        }
        Serial.print(F(">>>Calibration Successful"));
        display.println(F("VALUE SAVED"));
        display.display();
        delay(1000);
    }
    else {
        Serial.print(F(">>>Calibration Failed"));
        display.println(F("VALUE NOT SAVED"));
        display.display();
        delay(1000);
    }
    Serial.print(F(", exit "));
    Serial.print(_sensorName);
    Serial.print(F(" Calibration Mode<<<"));
    Serial.println();
    *isCalibSuccess = 0;
}

void ESP_Sensor::calibDisplay() {
    String firstLine;
    if ((_eepromCalibParamArray[0].lowerBound < _voltage) && (_voltage < _eepromCalibParamArray[0].upperBound)) {
        firstLine = String(_eepromCalibParamArray[0].paramValue,1) + _sensorUnit + F(" BUFFER");
    } 
    else if ((_eepromCalibParamArray[1].lowerBound < _voltage) && (_voltage < _eepromCalibParamArray[1].upperBound)) {
        firstLine = String(_eepromCalibParamArray[1].paramValue,1) + _sensorUnit + F(" BUFFER");
    }
    else if ((_eepromCalibParamArray[2].lowerBound < _voltage) && (_voltage < _eepromCalibParamArray[2].upperBound)) {
        firstLine = String(_eepromCalibParamArray[2].paramValue,1) + _sensorUnit + F(" BUFFER");
    }
    else {
        firstLine = F("NOT BUFFER SOL.");
    }
    String secondLine = _sensorName + F(" ");
    if (isTbdOutOfRange()) {
        secondLine += F(">");
    }
    displayTwoLines(firstLine, secondLine + String(_value,2) + _sensorUnit);
}

void ESP_Sensor::acquireValueFromVolt() {
    if (_enableSensor) {
        Serial.print(F("Reading "));
        Serial.println(_sensorName);
        displayTwoLines("Reading " + _sensorName, F(""));
        float value = 0;
        int m = 5;
        for (int i = 0; i < m; i++) {
            acquireVolt();
            value += calculateValueFromVolt();
        }
        _value = value / m;
    }
    else {
        Serial.print(_sensorName);
        Serial.println(F(" sensor disabled"));
        _value = NAN;
    }
    Serial.print(_sensorName);
    Serial.print(": ");
    Serial.println(_value); 
}

//virtual for EC (look ESP_EC.cpp)
void ESP_Sensor::acquireVolt() {
    float voltage = 0;
    int n = 100;
    for(int i=0; i < n; i++) {
        voltage += (analogRead(_sensorPin)/4095.0)*3300;
    }
    _voltage = voltage / n;
}

float ESP_Sensor::getValue() {
    return _value;
}

void ESP_Sensor::saveNewConfig() {
    if (_enableSensor != EEPROM.read(_eepromAddress)) {
        EEPROM.write(_eepromAddress, _enableSensor);
        EEPROM.commit();
    }
}

void ESP_Sensor::saveNewCalib() {
    int eepromAddr = _eepromStartAddress;
    for (int i = 0; i < _eepromCalibParamCount; i++) {
        if (*_eepromCalibParamArray[i].value != EEPROM.readFloat(eepromAddr)) {
            EEPROM.writeFloat(eepromAddr, *_eepromCalibParamArray[i].value);
            EEPROM.commit();
        }
        eepromAddr += (int)sizeof(float);
    }
}

bool ESP_Sensor::isTbdOutOfRange()
{
    return false;
}

void ESP_Sensor::tempCompVolt()//default, no temp compensation for volt
{
}
