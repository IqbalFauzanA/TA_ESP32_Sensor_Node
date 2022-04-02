#include "ESP_EC.h"
#include "ESP_PH.h"
#include "ESP_Turbidity.h"

#define PI_PIN 26 //for GPIO, receive request from Raspi
#define MODE_PIN 27 //button for switching to calibration selection mode
#define CAL_PIN 14 //universal calibration push button, also wake up button
#define BUTTON_PIN_BITMASK 0x4004000 //set pin 14 and 26 as wake up trigger
#define ONE_WIRE_BUS 4 // temperature sensor
#define SENSOR_COUNT 3 //total number of sensors, enabled+disabled
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define DISPLAY_I2C_ADDRESS 0x3C
#define DATA_RESEND_PERIOD 1000U

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
OneWire oneWire(ONE_WIRE_BUS);// Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature tempSensor(&oneWire);// Pass our oneWire reference to Dallas Temperature sensor 

ESP_Sensor** sensors = new ESP_Sensor*[SENSOR_COUNT];

debounceButton cal_button(CAL_PIN);
debounceButton mode_button(MODE_PIN);
Adafruit_ADS1115 ads;

String piTime; // waktu dari Raspi

void setup() {
    
    sensors[0] = new ESP_EC;
    sensors[1] = new ESP_Turbidity;
    sensors[2] = new ESP_PH;
    
    Serial.begin(9600);
    Serial.setTimeout(3000); //set serial timeout to 3 seconds
    
    pinMode(CAL_PIN, INPUT);//using external pull down, because it's used for wake up trigger
    //because ezbutton can't be used for long press, CAL_BUTTON also needs to use ordinary scheme
    pinMode(PI_PIN, INPUT);
    cal_button.setDebounceTime(50);
    mode_button.setDebounceTime(50);
    
    EEPROM.begin(128);
    
    for (int i = 0; i < SENSOR_COUNT; i++) {
        sensors[i]->begin();
    }
    
    ads.setGain(GAIN_ONE);
    ads.begin();

    
    tempSensor.begin();//temperature sensor init
    
    display.begin(DISPLAY_I2C_ADDRESS, true);
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.clearDisplay();
    
    esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);//wake up trigger
    
    //JIKA MENERIMA REQUEST DARI RASPI 
    bool isDisplayMain = false;
    unsigned long timepoint = 0U;
    while (digitalRead(PI_PIN)) { 
        sensors[0]->displayTwoLines(F("Reading Serial"), F("waiting for cmd"));
        String inString = Serial.readStringUntil('\n');
        //when sending request, raspi will send local time
        if (isPiTime(inString)) { 
            display.println(F("responding req"));
            display.display();
            dataRequestResponse();
            isDisplayMain = true;
            break;
        }
        else if (inString == "config") {
            display.println(F("configurating"));
            display.display();
            sendInitAndProcessNewData(&sendConfigInitData, &processNewConfig);
            break;
        }
        else if (inString == "manualcalib") {
            display.println(F("manual calib"));
            display.display();
            sendInitAndProcessNewData(&sendCalibInitData, &processNewCalib);
            break;
        }
        else if (millis() - timepoint > 30000U) {
            sensors[0]->displayTwoLines(F("Request timeout"), inString);
            delay(1000);
            break;
        }
        else {
            sensors[0]->displayTwoLines(F("Wrong cmd format "), inString);
        }
    }
    //JIKA TIDAK KALIBRASI ATAU SUDAH SELESAI
    if (!digitalRead(CAL_PIN)) {
        if (isDisplayMain == false) {
            sensors[0]->displayTwoLines(F("Press CAL to"), F("wake up & calib."));
        }
        esp_deep_sleep_start();
    }
}

//calibration mode
void loop() {
    static unsigned long timepoint = 0U;
    static byte sensor = 0;
    cal_button.loop();
    mode_button.loop();
    if (millis() - timepoint > 3000U) {
        Serial.println(F("CALIB"));//signalling calibration state to raspi
        timepoint = millis();
    }

    if (sensor < SENSOR_COUNT) {
        sensors[sensor]->calibration(&sensor);
    }
    //exit
    else {
        sensors[0]->displayTwoLines(F("Select mode:"), F("Exit and Sleep"));
        if (mode_button.isReleased()) {
            sensor = 0;
        }
        else if (cal_button.isPressed()) {
            sensors[0]->displayTwoLines(F("Press CAL to"), F("wake up & calib."));
            esp_deep_sleep_start();
        } 
    }
}

bool isPiTime(String inStr) {
    if (isdigit(inStr[0]) && isdigit(inStr[1]) && (inStr[2] == ':') && 
        isdigit(inStr[3]) && isdigit(inStr[4]) && (inStr[5] == NULL)) {
        piTime = inStr;
        return true;
    }
    else {
        return false;
    }
}

void dataRequestResponse() {
    for (int i = 0; i < SENSOR_COUNT; i++) {
        sensors[i]->getFinalVoltAndValue();
    }
    sensors[0]->displayTwoLines(F("Send sensor data"), F(""));
    unsigned long timepoint = millis() - DATA_RESEND_PERIOD;
    unsigned long timepoint1 = millis();
    //keep sending data every 1 seconds for a minute until pi pin turned off (request finished)
    while (digitalRead(PI_PIN) && (millis() - timepoint1 < 60000U)) {
        if (millis() - timepoint > DATA_RESEND_PERIOD) {
            Serial.print(F("Data#"));
            Serial.print(F("Time:")); Serial.print(piTime);
            Serial.print(F(" ;Temperature:"));
            Serial.print(sensors[0]->_temperature);
            Serial.print(F(" "));
            for (int i = 0; i < SENSOR_COUNT; i++) {
                if (sensors[i]->_enableSensor) {
                    Serial.print(F(";"));
                    Serial.print(sensors[i]->_sensorName);
                    Serial.print(F(":"));
                    Serial.print(sensors[i]->_value);
                    Serial.print(F(" "));
                    Serial.print(sensors[i]->_sensorUnit);
                }
            }
            Serial.println(F(";"));
            timepoint = millis();
        }
    }
    if (digitalRead(PI_PIN)) {
        sensors[0]->displayTwoLines(F("Error (timeout)"), F("PI_PIN still on"));
    }
    displayMain();
} 

void displayMain() {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Reading time: " + piTime);
    display.println("Temp: " + String(sensors[0]->_temperature,2) + "^C");
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (sensors[i]->_enableSensor) {
            display.print(sensors[i]->_sensorName + ": ");
            if (sensors[i]->_sensorName == "Tbd") {
                if(sensors[i]->isTbdOutOfRange()) {
                    display.print(F(">"));
                }
            }
            display.print(sensors[i]->_value,2);
            display.println(" " + sensors[i]->_sensorUnit);
        }
    }
    display.display();
}

void sendInitAndProcessNewData(void (*sendInitData)(), void (*processNewData)()) {
    String inString = "";
    unsigned long timepoint = millis() - DATA_RESEND_PERIOD;
    unsigned long timepoint1 = millis();
    //keep sending data every 1 seconds for a minute until data received by Raspi
    while ((inString != "initdatareceived") && (millis() - timepoint1 < 60000U) 
            && digitalRead(PI_PIN)) {
        if (millis() - timepoint > DATA_RESEND_PERIOD) {
            sendInitData();
            timepoint = millis();
        }
        sensors[0]->displayTwoLines(F("Reading Serial"), F(""));
        inString = Serial.readStringUntil('\n');
    }
    if (!digitalRead(PI_PIN)) {
        sensors[0]->displayTwoLines(F("Raspi is"), F("disconnected"));
        delay(1000);
    }
    else if (inString == "initdatareceived") {
        sensors[0]->displayTwoLines(F("Inputting new data"), F("in Raspi"));
        while ((inString != "newdata") && (inString != "cancel") && 
                digitalRead(PI_PIN)) {
            inString = Serial.readStringUntil(':');
        }
        if (inString == "newdata") {
            processNewData();
        }
        else if (inString == "cancel") {
            Serial.println(F("cancelreceived"));
            sensors[0]->displayTwoLines(F("Data input"), F("cancelled"));
            delay(1000);
        }
        else {
            sensors[0]->displayTwoLines(F("Raspi is"), F("disconnected"));
            delay(1000);
        }
    }
    else {
        sensors[0]->displayTwoLines(F("Timeout no"), F("response"));
        delay(1000);
    }
}

void sendCalibInitData() {
    Serial.print(F("Data#"));
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (sensors[i]->_enableSensor) {
            Serial.print(sensors[i]->_sensorName);
            Serial.print(F(":"));
            for (int j = 0; j < sensors[i]->_eepromCalibParamCount; j++) {
                Serial.print(sensors[i]->_eepromCalibParamArray[j].name);
                Serial.print(F("_"));
                Serial.print(*sensors[i]->_eepromCalibParamArray[j].calibratedValue);
                Serial.print(F(","));
            }
            if (i < SENSOR_COUNT-1) {
                if (sensors[i+1]->_enableSensor) {
                    Serial.print(F(";"));
                }
            }
        }
    }
    Serial.println();
    sensors[0]->displayTwoLines(F("Send init calib"), F(""));
}

void processNewCalib() {
    Serial.println(F("newdatareceived"));
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (sensors[i]->_enableSensor) {
            for (int j = 0; j < sensors[i]->_eepromCalibParamCount; j++) {
                float inFloat = Serial.parseFloat();
                *sensors[i]->_eepromCalibParamArray[j].calibratedValue = inFloat;
                sensors[i]->saveNewCalib();
            }
        }
    }
    sensors[0]->displayTwoLines(F("Manual calib"), F("successful"));
}

void sendConfigInitData() {
    Serial.print(F("Data#"));
    for (int i = 0; i < SENSOR_COUNT; i++) {
        Serial.print(sensors[i]->_sensorName);
        Serial.print(sensors[i]->_enableSensor);
        Serial.print(F(";"));
    }
    Serial.println();
    sensors[0]->displayTwoLines(F("Send en. sensors"), F(""));
}

void processNewConfig() {
    Serial.println(F("newdatareceived"));
    for (int i = 0; i < SENSOR_COUNT; i ++) {
        byte inInt = Serial.parseInt();
        sensors[i]->_enableSensor = inInt;
        sensors[i]->saveNewConfig();
    }
    sensors[0]->displayTwoLines(F("Configuration"), F("successful"));
}