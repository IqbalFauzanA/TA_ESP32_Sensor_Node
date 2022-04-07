#include "ESP_Sensor.h"

extern OneWire oneWire;// Setup a oneWire instance to communicate with any OneWire devices
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
        float default_value = *_eepromCalibParamArray[i].calibratedValue; //set default_value with initial value
        *_eepromCalibParamArray[i].calibratedValue = EEPROM.readFloat(_eepromAddress); //read the calibrated value from EEPROM
        if (*_eepromCalibParamArray[i].calibratedValue == float() || isnan(*_eepromCalibParamArray[i].calibratedValue)) {
            *_eepromCalibParamArray[i].calibratedValue = default_value; // For new EEPROM, write default value to EEPROM
            EEPROM.writeFloat(_eepromAddress, *_eepromCalibParamArray[i].calibratedValue);
            EEPROM.commit();
            Serial.print(*_eepromCalibParamArray[i].calibratedValue);
            Serial.print(F(" written in EEPROM."));
        }
        _eepromAddress = _eepromAddress + (int)sizeof(float);
        Serial.print(_sensorName);
        Serial.print(" ");
        Serial.print(_eepromCalibParamArray[i].name);
        Serial.print(F(" in EEPROM: "));
        Serial.println(*_eepromCalibParamArray[i].calibratedValue);
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
                captureCalibValue(&isCalibSuccess);
                isPressed = false;
            }
            else if (mode_button.isReleased()) { //EXIT CALIB MODE
                saveCalibValueAndExit(&isCalibSuccess);
                isCalibrating = false;
            }
            if (millis() - timepoint > CALCULATE_PERIOD) {
                updateVoltAndValue();
                calibDisplay();
                timepoint = millis();
            }
        }
    }
    else {
        (*sensor)++; //skip disabled sensor
    }
}

void ESP_Sensor::captureCalibValue(bool* isCalibSuccess) {
    float calibTemporaryValue;
    for (int i = 0; i < 3; i++){
        if ((_eepromCalibParamArray[i].lowerBound < _voltage) && (_voltage < _eepromCalibParamArray[i].upperBound)) {
            calibTemporaryValue = calculateCalibTemporaryValue(_eepromCalibParamArray[i].solutionValue, _voltage);
            Serial.println();
            Serial.print(F(">>>Solution: "));
            Serial.print(_eepromCalibParamArray[i].solutionValue);
            Serial.print(F(" "));
            Serial.print(_sensorUnit);
            Serial.println(F("<<<"));
            if (isCalibrationTemporaryValueValid(calibTemporaryValue)){
                *_eepromCalibParamArray[i].calibratedValue = calibTemporaryValue;
                *isCalibSuccess = 1;
                Serial.println();
                display.println(F("CAL. SUCCESSFUL!"));
                display.display();
                delay(500);
            }
            else {
                Serial.println(F(">>>KValueTemp out of range 0.5-2.0<<<"));
            }
        }
    }
    if (*isCalibSuccess == false) {
        Serial.println();
        Serial.println(F(">>>Buffer Solution Error, Try Again<<<"));
        Serial.println();
        display.println(F("CAL. FAILED!"));
        display.display();
        delay(500);
    }
}

float ESP_Sensor::calculateCalibTemporaryValue(float solutionValue, float voltage){
    return voltage;
}

bool ESP_Sensor::isCalibrationTemporaryValueValid(float eepromTemporaryValue) {
    return true;
}

void ESP_Sensor::saveCalibValueAndExit(bool* isCalibSuccess) {
    if (*isCalibSuccess) {
        if ((_eepromCalibParamArray[0].lowerBound < _voltage) && (_voltage < _eepromCalibParamArray[0].upperBound)) {
            EEPROM.writeFloat(_eepromStartAddress, *_eepromCalibParamArray[0].calibratedValue);
            EEPROM.commit();
        } 
        else if ((_eepromCalibParamArray[1].lowerBound < _voltage) && (_voltage < _eepromCalibParamArray[1].upperBound)) {
            EEPROM.writeFloat(_eepromStartAddress + (int)sizeof(float), *_eepromCalibParamArray[1].calibratedValue);
            EEPROM.commit();
        }
        else if ((_eepromCalibParamArray[2].lowerBound < _voltage) && (_voltage < _eepromCalibParamArray[2].upperBound)) {
            //for EC
            if (_eepromCalibParamCount == 2) {
                EEPROM.writeFloat(_eepromStartAddress + (int)sizeof(float), *_eepromCalibParamArray[2].calibratedValue);
                EEPROM.commit();
            }
            //for tbd
            else if (_eepromCalibParamCount == 3) {
                EEPROM.writeFloat(_eepromStartAddress + (int)sizeof(float) + (int)sizeof(float), *_eepromCalibParamArray[2].calibratedValue);
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
        firstLine = String(_eepromCalibParamArray[0].solutionValue,2) + _sensorUnit + F(" BUFFER");
    } 
    else if ((_eepromCalibParamArray[1].lowerBound < _voltage) && (_voltage < _eepromCalibParamArray[1].upperBound)) {
        firstLine = String(_eepromCalibParamArray[1].solutionValue,2) + _sensorUnit + F(" BUFFER");
    }
    else if ((_eepromCalibParamArray[2].lowerBound < _voltage) && (_voltage < _eepromCalibParamArray[2].upperBound)) {
        firstLine = String(_eepromCalibParamArray[2].solutionValue,2) + _sensorUnit + F(" BUFFER");
    }
    else {
        firstLine = F("NOT BUFFER SOL.");
    }
    String secondLine = _sensorName + F(" ");
    if (isTbdOutOfRange()) {
        secondLine += F(">");
    }
    displayTwoLines(firstLine, secondLine + String(_value,2) + F(" ") + _sensorUnit);
}

void ESP_Sensor::updateVoltAndValue() {
    if (_enableSensor) {
        displayTwoLines("Reading " + _sensorName, F(""));
        float volt = 0;
        int m = 5;
        for (int i = 0; i < m; i++) {
            readAndAverageVolt();
            volt += compensateVoltWithTemperature();
        }
        _voltage = volt / m;
        _value = calculateValueFromVolt();
    }
    else {
        _voltage = NAN;
        _value = NAN;
    }
}

//virtual for EC (look ESP_EC.cpp)
void ESP_Sensor::readAndAverageVolt() {
    float voltage = 0;
    int n = 100;
    for(int i=0; i < n; i++) {
        voltage += (analogRead(_sensorPin)/4095.0)*3300;
    }
    _voltage = voltage / n;
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
        if (*_eepromCalibParamArray[i].calibratedValue != EEPROM.readFloat(eepromAddr)) {
            EEPROM.writeFloat(eepromAddr, *_eepromCalibParamArray[i].calibratedValue);
            EEPROM.commit();
        }
        eepromAddr += (int)sizeof(float);
    }
}

bool ESP_Sensor::isTbdOutOfRange() {
    return false;
}

float ESP_Sensor::compensateVoltWithTemperature() {//default, no temp compensation for volt
    return _voltage;
}
