#ifndef _ESP_NH3N_H_
#define _ESP_NH3N_H_

#include "ESP_Sensor.h"

#define NH3NVALUEADDR 34

#define STRONG_BASE_VALUE 131.58 // NH3-N concentration in mg/L
#define WEAK_BASE_VALUE 36.956
#define NH3N_SENSOR 35
#define WEAK_BASE_HIGH_VOLTAGE 2300.0
#define WEAK_BASE_LOW_VOLTAGE 0.0
#define STRONG_BASE_HIGH_VOLTAGE 3300.0
#define STRONG_BASE_LOW_VOLTAGE 2300.0



class ESP_NH3N: public ESP_Sensor
{
    public:
        ESP_NH3N();
        ~ESP_NH3N();                        //initialization
        
    private:
        float _strongBaseVoltage;
        float _weakBaseVoltage;
            
        float calculateValueFromVolt();
        void calibStartMessage();
};

#endif