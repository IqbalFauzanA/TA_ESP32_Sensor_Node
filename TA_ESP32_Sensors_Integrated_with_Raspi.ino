#include "ESP_EC.h"
#include "ESP_PH.h"
#include "ESP_Turbidity.h"

#define PI_PIN 26 //for GPIO, receive request from Raspi
#define MODE_PIN 27 //button for switching to calibration selection mode
#define CAL_PIN 14 //universal calibration push button, also wake up button
#define BUTTON_PIN_BITMASK 0x4004000 //set pin 14 and 26 as wake up trigger
#define ONE_WIRE_BUS 4 // temperature sensor
#define SENSORS_N 3 //number of sensors, enabled+disabled

OneWire oneWire(ONE_WIRE_BUS);// Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature tempSensor(&oneWire);// Pass our oneWire reference to Dallas Temperature sensor 

ESP_Sensor** sensors = new ESP_Sensor*[SENSORS_N];

debounceButton cal_button(CAL_PIN);
debounceButton mode_button(MODE_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);//columns = 16, rows = 2 
Adafruit_ADS1115 ads;

String piTime; // waktu dari Raspi
bool isLcdMain = false;

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
    
    for (int i = 0; i < SENSORS_N; i++)
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
        sensors[0]->lcdDisplay(F("Reading Serial"), F("waiting for cmd"));
        inString = Serial.readStringUntil('\n');
        if (isPiTime(inString))//when sending request, raspi will send local time
        {
            sensors[0]->lcdSecondLine(F("responding req"));
            dataRequestResponse();
            break;
        }
        else if (inString == "config")
        {
            sensors[0]->lcdSecondLine(F("configurating"));
            configuration();
            break;
        }
        else if (inString == "manualcalib")
        {
            sensors[0]->lcdSecondLine(F("manual calib"));
            manualCalib();
            break;
        }
        else if (millis() - timepoint > 30000U)
        {
            sensors[0]->lcdDisplay(F("Request timeout"), inString);
            delay(1000);
            break;
        }
        else
        {
            sensors[0]->lcdDisplay(F("Wrong cmd format "), inString);
        }
    }
    if (!digitalRead(CAL_PIN)) //JIKA TIDAK KALIBRASI
    {
        Serial.println(F("Going to sleep now"));
        if (isLcdMain == false)
        {
            sensors[0]->lcdDisplay(F("Press CAL to"), F("wake up & calib."));
        }
        esp_deep_sleep_start();
    }
}

void loop() //calibration mode
{
    static unsigned long timepoint = 0U;
    static byte state = 0;
    cal_button.loop();
    mode_button.loop();
    if (millis() - timepoint > 5000U)
    {
        Serial.println(F("CALIB"));//signalling calibration state to raspi
        timepoint = millis();
    }
    
    if (state < SENSORS_N)
    {
        sensors[state]->calState(&state);
    }
    else if (state == SENSORS_N)//exit
    {
        sensors[0]->lcdDisplay(F("Select mode:"), F("Exit and Sleep"));
        if (mode_button.isReleased())
        {
            state = 0;
        }
        else if (cal_button.isPressed())
        {
            sensors[0]->lcdDisplay(F("Press CAL to"), F("wake up & calib."));
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
        sensors[0]->lcdDisplay(F("Send init calib "), F(""));
        if (millis() - timepoint > 3000U)
        {
            Serial.print(F("Data#"));
            for (int i = 0; i < SENSORS_N; i++)
            {
                if (sensors[i]->_enableSensor)
                {
                    Serial.print(sensors[i]->_paramName);
                    Serial.print(F(":"));
                    for (int j = 0; j < sensors[i]->_eepromN; j++)
                    {
                        Serial.print(sensors[i]->_calibSolutionArr[j].name);
                        Serial.print(F("_"));
                        Serial.print(*sensors[i]->_calibSolutionArr[j].value);
                        Serial.print(F(","));
                    }
                }
                if (i < SENSORS_N-1)
                {
                    Serial.print(F(";"));
                }
            }
            Serial.println();
            timepoint = millis();
        }
        sensors[0]->lcdFirstLine(F("Reading Serial"));
        inString = Serial.readStringUntil('\n');
    }
    if (!digitalRead(PI_PIN))
    {
        sensors[0]->lcdDisplay(F("Raspi is"), F("disconnected"));
        delay(1000);
    }
    else if (inString == "initcalibdatareceived")
    {
        sensors[0]->lcdDisplay(F("Inputting manual"), F("calib in raspi"));
        while ((inString != "newcalibdata") && (inString != "cancelcalib") && 
                digitalRead(PI_PIN))
        {
            inString = Serial.readStringUntil(':');
        }
        if (inString == "newcalibdata")
        {
            Serial.println(F("newdatareceived"));
            lcd.setCursor(0,0);
            float inFloat;
            for (int i = 0; i < SENSORS_N; i++)
            {
                if (sensors[i]->_enableSensor)
                {
                    //Serial.print(sensors[i]->_paramName);
                    //Serial.print(F(":"));
                    for (int j = 0; j < sensors[i]->_eepromN; j++)
                    {
                        inFloat = Serial.parseFloat();
                        *sensors[i]->_calibSolutionArr[j].value = inFloat;
                        //Serial.print(sensors[i]->_calibSolutionArr[j].name);
                        //Serial.print(F("_"));
                        //Serial.print(*sensors[i]->_calibSolutionArr[j].value);
                        //Serial.print(F(","));
                        sensors[i]->saveNewCalib();
                    }
                }
            }
            Serial.println();
            sensors[0]->lcdDisplay(F("Manual calib"), F("successful"));
        }
        else if (inString == "cancelcalib")
        {
            sensors[0]->lcdDisplay(F("Manual calib"), F("cancelled"));
            delay(1000);
        }
        else
        {
            sensors[0]->lcdDisplay(F("Raspi is"), F("disconnected"));
            delay(1000);
        }
    }
    else
    {
        sensors[0]->lcdDisplay(F("Timeout no calib"), F("data response"));
        delay(1000);
    }
}

void dataRequestResponse()
{
    for (int i = 0; i < SENSORS_N; i++)
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
        sensors[0]->lcdDisplay(F("Send sensor data"), F(""));
        if (millis() - timepoint > 3000U)
        {
            Serial.print(F("Data#"));
            Serial.print(F("Time:"));
            Serial.print(piTime);
            Serial.print(F(" ;Temperature:"));
            Serial.print(sensors[0]->getTemperature());
            Serial.print(F(" "));
            for (int i = 0; i < SENSORS_N; i++)
            {
                if (sensors[i]->_enableSensor)
                {
                    Serial.print(F(";"));
                    Serial.print(sensors[i]->_paramName);
                    Serial.print(F(":"));
                    Serial.print(sensors[i]->getValue());
                    Serial.print(F(" "));
                    Serial.print(sensors[i]->_unit);
                }
            }
            Serial.println(F(";"));
            timepoint = millis();
        }
    }

    if (digitalRead(PI_PIN))
    {
        sensors[0]->lcdDisplay(F("Error (timeout)"), F("PI_PIN still on"));
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
        sensors[0]->lcdDisplay(F("Send en. sensors"), F(""));
        if (millis() - timepoint > 3000U)
        {
            Serial.print(F("Data#"));
            for (int i = 0; i < SENSORS_N; i++)
            {
                Serial.print(sensors[i]->_paramName);
                Serial.print(sensors[i]->_enableSensor);
                Serial.print(F(";"));
            }
            Serial.println();
            timepoint = millis();
        }
        sensors[0]->lcdFirstLine(F("Reading Serial"));
        inString = Serial.readStringUntil('\n');
    }
    if (!digitalRead(PI_PIN))
    {
        sensors[0]->lcdDisplay(F("Raspi is"), F("disconnected"));
        delay(1000);
    }
    else if (inString == "ensensorsreceived")
    {
        sensors[0]->lcdDisplay(F("Raspi is"), F("configurating"));
        while ((inString != "newconfig") && (inString != "cancelconfig") && 
                digitalRead(PI_PIN))
        {
            inString = Serial.readStringUntil(':');
        }
        if (inString == "newconfig")
        {  
            Serial.println(F("newdatareceived"));
            lcd.setCursor(0,0);
            for (int i = 0; i < SENSORS_N; i ++)
            {
                inString = Serial.parseInt();
                sensors[i]->_enableSensor = (inString == "1");
                sensors[i]->saveNewConfig();
            }
            delay(3000);
            sensors[0]->lcdDisplay(F("Configuration"), F("successful"));
        }
        else if (inString == "cancelconfig")
        {
            sensors[0]->lcdDisplay(F("Configuration"), F("cancelled"));
            delay(1000);
        }
        else
        {
            sensors[0]->lcdDisplay(F("Raspi is"), F("disconnected"));
            delay(1000);
        }
    }
    else
    {
        sensors[0]->lcdDisplay(F("Timeout no"), F("config response"));
        delay(1000);
    }
}
