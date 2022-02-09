#include "ESP_Sensor.h"

unsigned long timepoint;

extern OneWire oneWire;// Setup a oneWire instance to communicate with any OneWire devices
extern DallasTemperature tempSensor;// Pass our oneWire reference to Dallas Temperature sensor 
extern LiquidCrystal_I2C lcd;
extern debounceButton cal_button;
extern debounceButton mode_button;

ESP_Sensor::ESP_Sensor()
{
    _calMode = 0;
    _isCalib = 0;
    m = 5; //repetition to acquire temperature compensation
    n = 100; //repetition to acquire voltage value
}

ESP_Sensor::~ESP_Sensor()
{
}

void ESP_Sensor::begin()
{
    _eepromAddress = _eepromStartAddress;
    for (int i = 0; i < _eepromN; i++)
    {
        float default_value = *_calibSolutionArr[i].value; //set default_value with initial value
        *_calibSolutionArr[i].value = EEPROM.readFloat(_eepromAddress); //read the calibrated value from EEPROM
        if (*_calibSolutionArr[i].value == float() || isnan(*_calibSolutionArr[i].value))
        {
            *_calibSolutionArr[i].value = default_value; // For new EEPROM, write default value to EEPROM
            EEPROM.writeFloat(_eepromAddress, *_calibSolutionArr[i].value);
            EEPROM.commit();
            Serial.print(*_calibSolutionArr[i].value);
            Serial.print(F(" written in EEPROM."));
        }
        _eepromAddress = _eepromAddress + (int)sizeof(float);
        Serial.print(_paramName);
        Serial.print(" ");
        Serial.print(_calibSolutionArr[i].name);
        Serial.print(F(" in EEPROM: "));
        Serial.println(*_calibSolutionArr[i].value);
    }

    //check if sensor is set as enabled in EEPROM or not
    _enableSensor = EEPROM.read(_eepromAddress);
    Serial.print(_paramName);
    if (_enableSensor == 0)
    {
        Serial.print(F(" sensor disabled in EEPROM: "));
    } 
    else if (_enableSensor == 1)
    {
        Serial.print(F(" sensor enabled in EEPROM: "));
    }
    else //if EEPROM is uninitialized
    {
        _enableSensor = 1;
        saveNewConfig();
        Serial.print(F(" sensor initialized as enabled in EEPROM: "));
    }
    Serial.println(_enableSensor);

    tempSensor.requestTemperatures(); 
    _temperature = tempSensor.getTempCByIndex(0);
}

float ESP_Sensor::getTemperature()
{
    return _temperature;
}

void ESP_Sensor::buttonParse()
{
    _calMode = 0;
    static bool isPressed = false;
    if (cal_button.isPressed())
    {
        isPressed = true;
    }
    if(isPressed && cal_button.isReleased() && (_isCalib == 0))
    {
        _calMode = 1; //ENTER CALIB MODE
        isPressed = false;
    }
    else if(isPressed && cal_button.isReleased() && (_isCalib == 1))
    {
        _calMode = 2; //CALIBRATE SOLUTION
        isPressed = false;
    }
    else if(mode_button.isReleased() && (_isCalib == 1))
    {
        _calMode = 3; //EXIT CALIBRATION MODE
    }
}

void ESP_Sensor::calState(byte* state)//function for calibration state (overall), return the next state if state changed
{
    if (_enableSensor)
    {
        buttonParse();
        if (_isCalib == 0)
        {
            lcd.setCursor(0,0);
            lcd.print(F("Select mode:    ")); 
            lcd.setCursor(0,1);
            lcd.print(_paramName);
            lcd.print(F(" Calibration  "));
            if (mode_button.isReleased())
            {
                *state = *state + 1;
            }
        }
        else if (_isCalib == 1)
        {
            if (millis() - timepoint > CALCULATE_PERIOD)
            {
                calculateValue();
                timepoint = millis();
            }
            lcdCal();
        }
        calibration();
    }
    else
    {
        *state = *state + 1; //skip disabled sensor
    }
}

void ESP_Sensor::lcdCal()
{
    lcd.setCursor(0,0);
    if ((_calibSolutionArr[0].lowerBound < _voltage) && (_voltage < _calibSolutionArr[0].upperBound))
    {
        lcd.print(_calibSolutionArr[0].paramValue,1);
        lcd.print(_unit);
        lcd.print(F(" BUFFER    "));
    } 
    else if ((_calibSolutionArr[1].lowerBound < _voltage) && (_voltage < _calibSolutionArr[1].upperBound))
    {
        lcd.print(_calibSolutionArr[1].paramValue,1);
        lcd.print(_unit);
        lcd.print(F(" BUFFER    "));
    }
    else if ((_calibSolutionArr[2].lowerBound < _voltage) && (_voltage < _calibSolutionArr[2].upperBound))
    {
        lcd.print(_calibSolutionArr[2].paramValue,1);
        lcd.print(_unit);
        lcd.print(F(" BUFFER    "));
    }
    else
    {
        lcd.print(F("NOT BUFFER SOL. "));
    }
    lcd.setCursor(0,1);
    lcd.print(_paramName);
    lcd.print(F(" "));
    if (isTbdOutOfRange())
    {
        lcd.print(F(">"));
    }
    lcd.print(_value,2);
    lcd.print(_unit);
    lcd.print(F("        "));
}

void ESP_Sensor::calculateValue()
{
    if (_enableSensor)
    {
        Serial.print(F("Reading "));
        Serial.println(_paramName);
        lcd.setCursor(0,0);
        lcd.print(F("Reading "));
        lcd.print(_paramName);
        lcd.print(F("      "));
        lcd.setCursor(0,1);
        lcd.print(F("                "));
        if (_isTempCompAcq)
        {
            tempCompAcq();
        }
        else if (!_isTempCompAcq)
        {
            voltAcq();
            _value = compensateRaw();
        }
    }
    else
    {
        Serial.print(_paramName);
        Serial.println(F(" sensor disabled"));
        _value = NAN;
    }
    Serial.print(_paramName);
    Serial.print(": ");
    Serial.println(_value); 
}

float ESP_Sensor::getValue()
{
    return _value;
}

void ESP_Sensor::tempCompAcq()
{
    float value = 0;
    for (int i = 0; i < m; i++)
    {
        tempSensor.requestTemperatures(); 
        _temperature = tempSensor.getTempCByIndex(0); //store last temperature value
        voltAcq();
        value += compensateRaw();
    }
    _value = value / m;
}

void ESP_Sensor::voltAcq()
{
    float voltage = 0;
    for(int i=0; i < n; i++)
    {
        voltage += (analogRead(_sensorPin)/4095.0)*3300;
    }
    _voltage = voltage / n;
}

void ESP_Sensor::calibration()
{
    static bool calibrationFinish = 0;
    switch (_calMode)
    {

    case 1:
        _isCalib = 1;
        calibrationFinish = 0;
        timepoint = millis() - CALCULATE_PERIOD;
        calibStartMessage();
        break;

    case 2:
        if(_isCalib)
        {
            acqCalibValue(&calibrationFinish);
        }
        break;


    case 3:
        if(_isCalib)
        {
            if (calibrationFinish)
            {
                if ((_calibSolutionArr[0].lowerBound < _voltage) && (_voltage < _calibSolutionArr[0].upperBound))
                {
                    EEPROM.writeFloat(_eepromStartAddress, *_calibSolutionArr[0].value);
                    EEPROM.commit();
                } 
                else if ((_calibSolutionArr[1].lowerBound < _voltage) && (_voltage < _calibSolutionArr[1].upperBound))
                {
                    EEPROM.writeFloat(_eepromStartAddress + (int)sizeof(float), *_calibSolutionArr[1].value);
                    EEPROM.commit();
                }
                else if ((_calibSolutionArr[2].lowerBound < _voltage) && (_voltage < _calibSolutionArr[2].upperBound))
                {
                    if (_eepromN == 2) //for EC
                    {
                        EEPROM.writeFloat(_eepromStartAddress + (int)sizeof(float), *_calibSolutionArr[2].value);
                        EEPROM.commit();
                    }
                    else if (_eepromN == 3) //for tbd
                    {
                        EEPROM.writeFloat(_eepromStartAddress + (int)sizeof(float) + (int)sizeof(float), *_calibSolutionArr[2].value);
                        EEPROM.commit();
                    }
                }
                Serial.print(F(">>>Calibration Successful"));
                lcd.setCursor(0,1);
                lcd.print(F("VALUE SAVED     "));
                delay(1000);
            }
            else
            {
                Serial.print(F(">>>Calibration Failed"));
                lcd.setCursor(0,1);
                lcd.print(F("VALUE NOT SAVED "));
                delay(1000);
            }
            Serial.print(F(", exit "));
            Serial.print(_paramName);
            Serial.print(F(" Calibration Mode<<<"));
            Serial.println();
            calibrationFinish = 0;
            _isCalib = 0;
        }
        break;
    }    
}

void ESP_Sensor::acqCalibValue(bool* calibrationFinish)
{
     if ((_calibSolutionArr[0].lowerBound < _voltage) && (_voltage < _calibSolutionArr[0].upperBound))
    {
        Serial.println();
        Serial.print(F(">>>Solution: "));
        Serial.print(_calibSolutionArr[0].paramValue);
        Serial.print(F(" "));
        Serial.print(_unit);
        Serial.println(F("<<<"));
        *_calibSolutionArr[0].value = _voltage;
        Serial.println();
        *calibrationFinish = 1;
        lcd.setCursor(0,1);
        lcd.print(F("CAL. SUCCESSFUL!"));
        delay(500);
    } 
    else if ((_calibSolutionArr[1].lowerBound < _voltage) && (_voltage < _calibSolutionArr[1].upperBound))
    {
        Serial.println();
        Serial.print(F(">>>Solution: "));
        Serial.println(_calibSolutionArr[1].paramValue);
        Serial.print(F(" "));
        Serial.print(_unit);
        Serial.println(F("<<<"));
        *_calibSolutionArr[1].value = _voltage;
        Serial.println();
        *calibrationFinish = 1;
        lcd.setCursor(0,1);
        lcd.print(F("CAL. SUCCESSFUL!"));
        delay(500);
    }
    else if ((_calibSolutionArr[2].lowerBound < _voltage) && (_voltage < _calibSolutionArr[2].upperBound))
    {
        Serial.println();
        Serial.print(F(">>>Solution: "));
        Serial.println(_calibSolutionArr[2].paramValue);
        Serial.print(F(" "));
        Serial.print(_unit);
        Serial.println(F("<<<"));
        *_calibSolutionArr[2].value = _voltage;
        Serial.println();
        *calibrationFinish = 1;
        lcd.setCursor(0,1);
        lcd.print(F("CAL. SUCCESSFUL!"));
        delay(500);
    }
    else
    {
        Serial.println();
        Serial.println(F(">>>Buffer Solution Error, Try Again<<<"));
        Serial.println();
        *calibrationFinish = 0;
        lcd.setCursor(0,1);
        lcd.print(F("CAL. FAILED!    "));
        delay(500);
    }
}

void ESP_Sensor::saveNewConfig()
{
    lcd.print(_paramName);
    lcd.print(EEPROM.read(_eepromAddress));
    if (_enableSensor != EEPROM.read(_eepromAddress))
    {
        EEPROM.write(_eepromAddress, _enableSensor);
        EEPROM.commit();
        lcd.print(F("V"));
    }
    else
    {
        lcd.print(F("X"));
    }
    lcd.print(EEPROM.read(_eepromAddress));
}

bool ESP_Sensor::isTbdOutOfRange()
{
    return false;
}
