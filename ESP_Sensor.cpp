#include "ESP_Sensor.h"

extern Adafruit_SH1106G display;
extern debounceButton cal_button;
extern debounceButton mode_button;
extern OneWire oneWire;              // Setup a oneWire instance to communicate with any OneWire devices
extern DallasTemperature tempSensor; // Pass our oneWire reference to Dallas Temperature sensor

ESP_Sensor::ESP_Sensor()
{
}

ESP_Sensor::~ESP_Sensor()
{
}

void ESP_Sensor::begin()
{
    _eepromAddress = _eepromStartAddress;
    for (int i = 0; i < _calibParamCount; i++)
    {
        float default_value = *_calibParamArray[i].calibVolt;              // set default_value with initial value
        *_calibParamArray[i].calibVolt = EEPROM.readFloat(_eepromAddress); // read the calibrated value from EEPROM
        if (*_calibParamArray[i].calibVolt == float() || isnan(*_calibParamArray[i].calibVolt) ||
            _resetCalibratedValueToDefault)
        {
            *_calibParamArray[i].calibVolt = default_value; // For new EEPROM, write default value to EEPROM
            EEPROM.writeFloat(_eepromAddress, *_calibParamArray[i].calibVolt);
            EEPROM.commit();
            Serial.print(*_calibParamArray[i].calibVolt);
            Serial.print(F(" written in EEPROM."));
        }
        _eepromAddress = _eepromAddress + (int)sizeof(float);
        Serial.print(_sensorName);
        Serial.print(F(" "));
        Serial.print(_calibParamArray[i].solutionValue);
        Serial.print(F(" "));
        Serial.print(_sensorUnit);
        Serial.print(F(" Voltage in EEPROM: "));
        Serial.println(*_calibParamArray[i].calibVolt);
    }

    // check if sensor is set as enabled in EEPROM or not
    _enableSensor = EEPROM.read(_eepromAddress);
    Serial.print(_sensorName);
    if (_enableSensor == 0)
    {
        Serial.print(F(" sensor disabled in EEPROM: "));
    }
    else if (_enableSensor == 1)
    {
        Serial.print(F(" sensor enabled in EEPROM: "));
    }
    // if EEPROM is uninitialized
    else
    {
        _enableSensor = 1;
        saveNewConfig();
        Serial.print(F(" sensor initialized as enabled in EEPROM: "));
    }
    Serial.println(_enableSensor);
}

void ESP_Sensor::displayTwoLines(String firstLine, String secondLine)
{
    display.clearDisplay();
    display.setCursor(0, 2);
    display.println(firstLine);
    display.println(secondLine);
    display.display();
}

// function for changing calibration state
void ESP_Sensor::calibration(byte *sensor)
{
    if (_enableSensor)
    {
        static bool isCalibSuccess = false;
        static bool isPressed = false;
        static bool isCalibrating = false;
        static bool isParamMenu = false;
        static byte calibParamIdx = 10;
        static unsigned long timepoint;
        if (cal_button.isPressed())
        {
            isPressed = true;
        }
        // STILL OUTSIDE CALIB MODE
        if (isCalibrating == false)
        {
            if (calibParamIdx == 10) // OUTSIDE PARAMETER MENU
            {
                displayTwoLines(F("Select mode:"),
                                _sensorName + F(" Calibration"));
                if (isPressed && cal_button.isReleased())
                { // ENTER CALIB MODE
                    isPressed = false;
                    calibParamIdx = 0; // ENTER PARAMETER MENU
                }
                else if (mode_button.isReleased())
                {
                    (*sensor)++; // move to next sensor
                }
            }
            else if (calibParamIdx < _calibParamCount) // INSIDE PARAMETER MENU
            {
                displayTwoLines(F("Select solution:"),
                                _sensorName + " " +
                                    String(_calibParamArray[calibParamIdx].solutionValue) +
                                    " " + _sensorUnit);
                if (isPressed && cal_button.isReleased())
                {
                    isPressed = false;
                    isCalibrating = true;
                    isCalibSuccess = false;
                    timepoint = millis() - CALCULATE_PERIOD;
                }
                else if (mode_button.isReleased())
                {
                    calibParamIdx++;
                }
            }
            else if (calibParamIdx == _calibParamCount)
            {
                displayTwoLines(F("Select solution:"),
                                F("Save & Back"));
                if (mode_button.isReleased())
                {
                    calibParamIdx++;
                }
                else if (isPressed && cal_button.isReleased())
                {
                    isPressed = false;
                    calibParamIdx = 10; // SAVE & EXIT PARAM MENU
                    saveCalibVoltAndExit(&isCalibSuccess);
                }
            }
            else
            {
                displayTwoLines(F("Select solution:"),
                                F("Cancel & Back"));
                if (mode_button.isReleased())
                {
                    calibParamIdx = 0;
                }
                else if (isPressed && cal_button.isReleased())
                {
                    isPressed = false;
                    calibParamIdx = 10; // CANCEL & EXIT PARAM MENU
                    isCalibSuccess = 0;
                    saveCalibVoltAndExit(&isCalibSuccess);
                }
            }
        }
        // INSIDE CALIB MODE
        else if (isCalibrating == true)
        {
            if (millis() - timepoint > CALCULATE_PERIOD)
            {
                updateVoltAndValue();
                calibDisplay(calibParamIdx);
                timepoint = millis();
            }
            if (isPressed && cal_button.isReleased())
            { // CAPTURE CALIB VOLT
                captureCalibVolt(&isCalibSuccess, calibParamIdx);
                isPressed = false;
            }
            else if (mode_button.isReleased())
            { // EXIT CALIB MODE
                isCalibrating = false;
            }
        }
    }
    else
    {
        (*sensor)++; // skip disabled sensor
    }
}

void ESP_Sensor::captureCalibVolt(bool *isCalibSuccess, byte calibParamIdx)
{
    if ((0 < _voltage) && (_voltage < 3300))
    {
        *_calibParamArray[calibParamIdx].calibVolt = _voltage;
        *isCalibSuccess = 1;
        display.println(F("CAL. SUCCESSFUL!"));
        display.display();
        delay(500);
    }
    else
    {
        display.println(F("VOLT OUT OF RANGE!"));
        display.display();
        delay(500);
    }
}

void ESP_Sensor::saveCalibVoltAndExit(bool *isCalibSuccess)
{
    if (*isCalibSuccess)
    {
        saveNewCalib();
        display.println(F("VALUE SAVED"));
        display.display();
        delay(1000);
    }
    else
    {
        display.println(F("VALUE NOT SAVED"));
        display.display();
        delay(1000);
    }
    *isCalibSuccess = 0;
}

void ESP_Sensor::calibDisplay(byte calibParamIdx)
{
    String firstLine = String(_calibParamArray[calibParamIdx].solutionValue) + F(" ") + _sensorUnit + F(" SOLUTION");
    String secondLine = _sensorName + F(" ");
    if (isTbdOutOfRange())
    {
        secondLine += F(">");
    }
    displayTwoLines(firstLine, secondLine + String(_value) + F(" ") + _sensorUnit);
    display.println("Voltage (mV): " + String(_voltage));
    display.println(String(_temperature, 2) + F(" ^C"));
    display.display();
}

void ESP_Sensor::updateVoltAndValue()
{
    if (_enableSensor)
    {
        displayTwoLines("Reading " + _sensorName, F(""));
        float volt = 0;
        int m = 5;
        for (int i = 0; i < m; i++)
        {
            readAndAverageVolt();
            volt += compensateVoltWithTemperature();
        }
        _voltage = volt / m;
        _value = calculateValueFromVolt();
    }
    else
    {
        _voltage = NAN;
        _value = NAN;
    }
}

// virtual for EC (look ESP_EC.cpp)
void ESP_Sensor::readAndAverageVolt()
{
    float voltage = 0;
    int n = 100;
    for (int i = 0; i < n; i++)
    {
        voltage += (analogRead(_sensorPin) / 4095.0) * 3300;
    }
    _voltage = voltage / n;
}

void ESP_Sensor::saveNewConfig()
{
    if (_enableSensor != EEPROM.read(_eepromAddress))
    {
        EEPROM.write(_eepromAddress, _enableSensor);
        EEPROM.commit();
    }
}

void ESP_Sensor::saveNewCalib()
{
    int eepromAddr = _eepromStartAddress;
    for (int i = 0; i < _calibParamCount; i++)
    {
        if (*_calibParamArray[i].calibVolt != EEPROM.readFloat(eepromAddr))
        {
            EEPROM.writeFloat(eepromAddr, *_calibParamArray[i].calibVolt);
            EEPROM.commit();
        }
        eepromAddr += (int)sizeof(float);
    }
}

bool ESP_Sensor::isTbdOutOfRange()
{
    return false;
}

float ESP_Sensor::compensateVoltWithTemperature()
{ // default, no temp compensation for volt
    tempSensor.requestTemperatures();
    _temperature = tempSensor.getTempCByIndex(0); // store last temperature value
    return _voltage;
}
