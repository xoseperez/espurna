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
    void begin()
    {
        pinMode(0, INPUT);
        _ready = true;
    }

    // Descriptive name of the sensor
    String description()
    {
        return String("THERMISTOR @ TOUT");
    }

    // Descriptive name of the slot # index
    String slot(unsigned char index)
    {
        return description();
    };

    // Address of the sensor (it could be the GPIO or I2C address)
    String address(unsigned char index)
    {
        return String("0");
    }

    // Type for slot # index
    unsigned char type(unsigned char index)
    {
        if (index == 0)
            return MAGNITUDE_TEMPERATURE;
        return MAGNITUDE_NONE;
    }

    // Current value for slot # index
    double value(unsigned char index)
    {
        if (index != 0)
            return 0;

        //https://en.wikipedia.org/wiki/Thermistor#Steinhart.E2.80.93Hart_equation

        double analogValue = 0;
        for (char i = 0; i < THERMISTOR_SAMPLES; i++)
        {
            analogValue += analogRead(0);
            delayMicroseconds(100);
        }

        analogValue /= THERMISTOR_SAMPLES; //Average
#if SENSOR_DEBUG
        char buffer[16];
        dtostrf(analogValue, 1 - sizeof(buffer), 4, buffer);
        DEBUG_MSG("[THERMISTOR] averag analogValue=%s\n", buffer);
#endif

        //ADC resolution is 10 bits - max=1023
        double resistance = THERMISTOR_SERIES_RESISTANCE * (1023 / analogValue - 1);
#if SENSOR_DEBUG
        dtostrf(resistance, 1 - sizeof(buffer), 4, buffer);
        DEBUG_MSG("[THERMISTOR] resistance=%s\n", buffer);
#endif

        //Equation
        //1/T = 1/T0 + (1/B)*ln(R/R0)
        double steinhart = (1.0 / (THERMISTOR_NOMIMAL_TEMPERATURE + 273.15)) + (log(resistance / THERMISTOR_NOMINAL_RESISTANCE) / THERMISTOR_BETA_COEFFICIENT);
        steinhart = (1.0 / steinhart) - 273.15; //Invert and subtract to get Celsius value

        return steinhart;
    }
};

#endif // SENSOR_SUPPORT && THERMISTOR_SUPPORT
