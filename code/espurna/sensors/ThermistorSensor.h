// -----------------------------------------------------------------------------
// Thermistor Sensor
// Copyright (C) 2017-2018 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && THERMISTOR_SUPPORT

#pragma once

// Set ADC to TOUT pin
#undef ADC_MODE_VALUE
#define ADC_MODE_VALUE ADC_TOUT

#include "Arduino.h"
#include "BaseSensor.h"

class ThermistorSensor : public BaseSensor
{
  public:
    // ---------------------------------------------------------------------
    // Public
    // ---------------------------------------------------------------------

    ThermistorSensor() : BaseSensor()
    {
        _count = 1;
        _sensor_id = SENSOR_THERMISTOR_ID;
    }

    // ---------------------------------------------------------------------
    // Sensor API
    // ---------------------------------------------------------------------

    // Initialization method, must be idempotent
    void begin() {
        pinMode(0, INPUT);
        _ready = true;
    }
        
    // Descriptive name of the sensor
    String description()
    {
        return String("THERMISTOR @ TOUT");
    }

    // Descriptive name of the slot # index
    String slot(unsigned char index) {
        return description();
    };

    // Address of the sensor (it could be the GPIO or I2C address)
    String address(unsigned char index) {
        return String("0");
    }
        
        // Type for slot # index
    unsigned char type(unsigned char index)
    {        
        if (index == 0) return MAGNITUDE_TEMPERATURE;
        return MAGNITUDE_NONE;
    }

    // Current value for slot # index
    double value(unsigned char index)
    {
        if (index != 0) return 0;
        
        //https://en.wikipedia.org/wiki/Thermistor#Steinhart.E2.80.93Hart_equation

        double A = 1.484778004e-03, B = 2.348962910e-04, C = 1.006037158e-07;  // Steinhart-Hart and Hart Coefficients
        double sample = analogRead(0);                    

        #if SENSOR_DEBUG
        DEBUG_MSG_P(PSTR("[THERMISTOR] sample=%f"), sample);
        #endif

        //ADC resolution is 10 bits - max=1023
        double resistance = THERMISTOR_SERIES_RESISTANCE * (1023 / sample - 1);
        #if SENSOR_DEBUG
        DEBUG_MSG_P(PSTR("[THERMISTOR] resistance=%f"), resistance);
        #endif

        double logR2 = log(resistance);
        double steinhart = (1.0 / (A + B*logR2 + C*logR2*logR2*logR2));  // Steinhart and Hart Equation. T  = 1 / {A + B[ln(R)] + C[ln(R)]^3}        
        steinhart -= 273.15;         // Convert Kelvin to Celsius

        #if SENSOR_DEBUG
        DEBUG_MSG_P(PSTR("[THERMISTOR] steinhart=%f"), steinhart);
        #endif

        return steinhart;
    }
};

#endif // SENSOR_SUPPORT && THERMISTOR_SUPPORT
