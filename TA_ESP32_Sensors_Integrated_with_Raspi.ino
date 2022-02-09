#include "ESP_EC.h"
#include "ESP_PH.h"
#include "ESP_Turbidity.h"

#define PI_PIN 26 //for GPIO, receive request from Raspi
#define MODE_PIN 27 //button for switching to calibration selection mode
#define CAL_PIN 14 //universal calibration push button, also wake up button
#define BUTTON_PIN_BITMASK 0x4004000 //set pin 14 and 26 as wake up trigger
#define ONE_WIRE_BUS 4 // temperature sensor

OneWire oneWire(ONE_WIRE_BUS);// Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature tempSensor(&oneWire);// Pass our oneWire reference to Dallas Temperature sensor 

ESP_Sensor** sensors = new ESP_Sensor*[3];

debounceButton cal_button(CAL_PIN);
debounceButton mode_button(MODE_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);//columns = 16, rows = 2 
Adafruit_ADS1115 ads;

byte state = 0;
String piTime; // waktu dari Raspi
bool isLcdMain = false;

byte sensorsN = 3; //number of sensors, enabled+disabled

void setup() {
  
    sensors[0] = new ESP_EC;
    sensors[1] = new ESP_Turbidity;
    sensors[2] = new ESP_PH;
    
    Serial.begin(115200);
    Serial.setTimeout(30000); //set serial timeout to 30 seconds
    delay(10);
    
    pinMode(CAL_PIN, INPUT);//using external pull down, because it's used for wake up trigger
    //because ezbutton can't be used for long press, CAL_BUTTON also needs to use ordinary scheme
    pinMode(PI_PIN, INPUT);
    cal_button.setDebounceTime(50);
    mode_button.setDebounceTime(50);
    
    EEPROM.begin(512);
    
    for (int i = 0; i < sensorsN; i++)
    {
        sensors[i]->begin();
    }
    
    ads.setGain(GAIN_ONE);
    ads.begin();

    
    tempSensor.begin();//temperature sensor init
    
    lcd.begin();
    lcd.backlight();
    
    esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);//wake up trigger
    
    while (digitalRead(PI_PIN)) //JIKA MENERIMA REQUEST DARI RASPI
    {
        String inString;
        static unsigned long timepoint = 0U;
        lcd.setCursor(0,0);
        lcd.print(F("Reading Serial  "));
        lcd.setCursor(0,1);
        lcd.print(F("waiting for cmd "));
        inString = Serial.readStringUntil('\n');
        if (isPiTime(inString))//when sending request, raspi will send local time
        {
            lcd.setCursor(0,1);
            lcd.print(F("responding req  "));
            dataRequestResponse();
            break;
        }
        else if (inString == "config")
        {
            lcd.setCursor(0,1);
            lcd.print(F("configurating   "));
            configuration();
            break;
        }
        else if (inString == "manualcalib")
        {
            lcd.setCursor(0,1);
            lcd.print(F("manual calib    "));
            manualCalib();
            break;
        }
        else if (millis() - timepoint > 30000U)
        {
            lcd.setCursor(0,0);
            lcd.print(F("Request timeout "));
            lcd.setCursor(0,1);
            lcd.print(inString);
            delay(1000);
            break;
        }
        else
        {
            lcd.setCursor(0,0);
            lcd.print(F("Wrong cmd format "));
            lcd.setCursor(0,1);
            lcd.print(inString);
        }
    }
    if (!digitalRead(CAL_PIN)) //JIKA TIDAK KALIBRASI
    {
        Serial.println(F("Going to sleep now"));
        if (isLcdMain == false)
        {
            lcd.setCursor(0,0);
            lcd.print(F("Press CAL to    "));
            lcd.setCursor(0,1);
            lcd.print(F("wake up & calib."));
        }
        esp_deep_sleep_start();
    }
}

void loop() //calibration mode
{
    static unsigned long timepoint = 0U;
    cal_button.loop();
    mode_button.loop();
    if (millis() - timepoint > 5000U)
    {
        Serial.println(F("CALIB"));//signalling calibration state to raspi
        timepoint = millis();
    }
    
    if (state < sensorsN)
    {
        sensors[state]->calState(&state);
    }
    else if (state == sensorsN)//exit
    {
        lcd.setCursor(0,0);
        lcd.print(F("Select mode:    "));
        lcd.setCursor(0,1);
        lcd.print(F("Exit and Sleep  "));
        if (mode_button.isReleased())
        {
            state = 0;
        }
        else if (cal_button.isPressed())
        {
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print(F("Press CAL to    "));
            lcd.setCursor(0,1);
            lcd.print(F("wake up & calib."));
            esp_deep_sleep_start();
        } 
    }
}

bool isPiTime(String inStr)
{
    if (isdigit(inStr[0]) && isdigit(inStr[1]) && (inStr[2] == ':') && 
        isdigit(inStr[3]) && isdigit(inStr[4]) && (inStr[5] == NULL))
    {
        piTime = inStr;
        return true;
    }
    else
    {
        return false;
    }
}

void manualCalib()
{
    unsigned long timepoint = millis() - 3000U;
    unsigned long timepoint1 = millis();
    String inString = "";
    while ((inString != "initcalibdatareceived") && (millis() - timepoint1 < 60000U) &&
            digitalRead(PI_PIN))
    //keep sending data every 3 seconds for a minute until initcalib data received
    {
        lcd.setCursor(0,0);
        lcd.print(F("Send init calib "));
        lcd.setCursor(0,1);
        lcd.print(F("                "));
        if (millis() - timepoint > 3000U)
        {
            Serial.print(F("Initial calibration data;"));
            for (int i = 0; i < sensorsN; i++)
            {
                if (sensors[i]->_enableSensor)
                {
                    Serial.print(sensors[i]->_paramName);
                    Serial.print(F(":"));
                    for (int j = 0; j < sensors[i]->_eepromN; j++)
                    {
                        Serial.print(sensors[i]->_calibSolutionArr[j].name);
                        Serial.print(F("-"));
                        Serial.print(sensors[i]->*_calibSolutionArr[j].value);
                        Serial.print(F(","));
                    }
                }
            }
            Serial.println();
            timepoint = millis();
        }
        lcd.setCursor(0,0);
        lcd.print(F("Reading Serial  "));
        inString = Serial.readStringUntil('\n');
    }
    if (!digitalRead(PI_PIN))
    {
        lcd.setCursor(0,0);
        lcd.print(F("Raspi is        "));
        lcd.setCursor(0,1);
        lcd.print(F("disconnected    "));
        delay(1000);
    }
    else if (inString == "initcalibdatareceived")
    {
        lcd.setCursor(0,0);
        lcd.print(F("Inputting manual"));
        lcd.setCursor(0,1);
        lcd.print(F("calib in raspi  "));
        while ((inString != "newcalibdata") && (inString != "cancelcalib") && 
                digitalRead(PI_PIN))
        {
            inString = Serial.readStringUntil(';');
        }
        if (inString == "newcalibdata")
        {
            Serial.println(F("newcalibdatareceived"));
            lcd.setCursor(0,0);
            int inInt;
            for (int i = 0; i < sensorsN; i++)
            {
                if (sensors[i]->_enableSensor)
                {
                    Serial.print(sensors[i]->_paramName);
                    Serial.print(F(":"));
                    for (int j = 0; j < sensors[i]->_eepromN; j++)
                    {
                        inInt = Serial.parseInt();
                        sensors[i]->*_calibSolutionArr[j].value = inInt;
                        Serial.print(sensors[i]->_calibSolutionArr[j].name);
                        Serial.print(F("-"));
                        Serial.print(sensors[i]->*_calibSolutionArr[j].value);
                        Serial.print(F(","));
                        //sensors[i]->saveNewCalib();
                    }
                }
            }
            Serial.println();
            lcd.setCursor(0,0);
            lcd.print(F("Manual calib    "));
            lcd.setCursor(0,1);
            lcd.print(F("successful      "));
        }
        else if (inString == "cancelcalib")
        {
            lcd.setCursor(0,0);
            lcd.print(F("Manual calib    "));
            lcd.setCursor(0,1);
            lcd.print(F("cancelled       "));
            delay(1000);
        }
        else
        {
            lcd.setCursor(0,0);
            lcd.print(F("Raspi is        "));
            lcd.setCursor(0,1);
            lcd.print(F("disconnected    "));
            delay(1000);
        }
    }
    else
    {
        lcd.setCursor(0,0);
        lcd.print(F("Timeout no calib"));
        lcd.setCursor(0,1);
        lcd.print(F("data response   "));
        delay(1000);
    }
}

void dataRequestResponse()
{
    for (int i = 0; i < sensorsN; i++)
    {
        sensors[i]->calculateValue();
    }
    sendSerialPi();
    lcdMain();
}

void sendSerialPi()
{
    unsigned long timepoint = millis() - 3000U;
    unsigned long timepoint1 = millis();
    while (digitalRead(PI_PIN) && (millis() - timepoint1 < 60000U))
    //keep sending data every 3 seconds for a minute until pi pin turned off (request finished)
    {
        lcd.setCursor(0,0);
        lcd.print(F("Send sensor data"));
        lcd.setCursor(0,1);
        lcd.print(F("                "));
        if (millis() - timepoint > 3000U)
        {
            Serial.print(F("Time: "));
            Serial.print(piTime);

            for (int i = 0; i < sensorsN; i++)
            {
                Serial.print(F("; "));
                Serial.print(sensors[i]->_paramName);
                Serial.print(F(": "));
                Serial.print(sensors[i]->getValue());
            }
            Serial.print(F("; Temperature: "));
            Serial.print(sensors[0]->getTemperature());
            Serial.println(F(";"));
            timepoint = millis();
        }
    }

    if (digitalRead(PI_PIN))
    {
        lcd.setCursor(0,0);
        lcd.print(F("Error (timeout) "));
        lcd.setCursor(0,1);
        lcd.print(F("PI_PIN still on "));
    }
}

void lcdMain()
{
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(piTime);
    lcd.setCursor(6,0);
    lcd.print(F("E "));
    lcd.print(sensors[0]->getValue(),4);
    lcd.setCursor(0,1);
    lcd.print(F("P "));
    lcd.print(sensors[2]->getValue(),2);
    lcd.setCursor(8,1);
    lcd.print(F("T "));
    if(sensors[1]->isTbdOutOfRange())
    {
        lcd.print(">");
    }
    lcd.print(sensors[1]->getValue(),1);
    isLcdMain = true;
}

void configuration()
{
    unsigned long timepoint = millis() - 3000U;
    unsigned long timepoint1 = millis();
    String inString = "";
    while ((inString != "ensensorsreceived") && (millis() - timepoint1 < 60000U) &&
            digitalRead(PI_PIN))
    //keep sending data every 3 seconds for a minute until en sensors received
    {
        lcd.setCursor(0,0);
        lcd.print(F("Send en. sensors"));
        lcd.setCursor(0,1);
        lcd.print(F("                "));
        if (millis() - timepoint > 3000U)
        {
            Serial.print(F("En Sensors: "));
            for (int i = 0; i < sensorsN; i++)
            {
                Serial.print(sensors[i]->_paramName);
                Serial.print(F(":"));
                Serial.print(sensors[i]->_enableSensor);
                Serial.print(F(";"));
            }
            Serial.println();
            timepoint = millis();
        }
        lcd.setCursor(0,0);
        lcd.print(F("Reading Serial  "));
        inString = Serial.readStringUntil('\n');
    }

    if (!digitalRead(PI_PIN))
    {
        lcd.setCursor(0,0);
        lcd.print(F("Raspi is        "));
        lcd.setCursor(0,1);
        lcd.print(F("disconnected    "));
        delay(1000);
    }
    else if (inString == "ensensorsreceived")
    {
        lcd.setCursor(0,0);
        lcd.print(F("Raspi is        "));
        lcd.setCursor(0,1);
        lcd.print(F("configurating   "));
        while ((inString != "newconfig") && (inString != "cancelconfig") && 
                digitalRead(PI_PIN))
        {
            inString = Serial.readStringUntil(':');
        }
        if (inString == "newconfig")
        {  
            Serial.println(F("newconfigreceived"));
            lcd.setCursor(0,0);
            for (int i = 0; i < sensorsN; i ++)
            {
                inString = Serial.parseInt();
                sensors[i]->_enableSensor = (inString == "1");
                sensors[i]->saveNewConfig();
            }
            delay(3000);
            lcd.setCursor(0,0);
            lcd.print(F("Configuration   "));
            lcd.setCursor(0,1);
            lcd.print(F("successful      "));
        }
        else if (inString == "cancelconfig")
        {
            lcd.setCursor(0,0);
            lcd.print(F("Configuration   "));
            lcd.setCursor(0,1);
            lcd.print(F("cancelled       "));
            delay(1000);
        }
        else
        {
            lcd.setCursor(0,0);
            lcd.print(F("Raspi is        "));
            lcd.setCursor(0,1);
            lcd.print(F("disconnected    "));
            delay(1000);
        }
    }
    else
    {
        lcd.setCursor(0,0);
        lcd.print(F("Timeout no      "));
        lcd.setCursor(0,1);
        lcd.print(F("config response "));
        delay(1000);
    }
}
