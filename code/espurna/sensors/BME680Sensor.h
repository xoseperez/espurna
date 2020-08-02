// -----------------------------------------------------------------------------
// BME680 Sensor over I2C
// Copyright (C) 2020 by Rui Marinho <ruipmarinho at gmail dot com>
//
// The BSEC software binaries and includes are only available for use after accepting its software
// license agreement. By enabling this sensor integration, you are agreeing to the terms of the license
// agreement available at the following URL:
//
// https://ae-bst.resource.bosch.com/media/_tech/media/bsec/2017-07-17_ClickThrough_License_Terms_Environmentalib_SW_CLEAN.pdf
//
// The Arduino wrapper and BME680 Sensor API used for this integration are licensed under the following terms:
//
//   Copyright (c) 2020 Bosch Sensortec GmbH. All rights reserved.
//
//   BSD-3-Clause
//
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions are met:
//
//   1. Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//   3. Neither the name of the copyright holder nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
//   COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
//   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
//   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
//   IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//   POSSIBILITY OF SUCH DAMAGE.
//
// For more details, please refer to https://github.com/BoschSensortec/BSEC-Arduino-library.
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && BME680_SUPPORT

#pragma once

#include <Arduino.h>
#include <bsec.h>

#include "I2CSensor.h"

// Available configuration modes based on parameters:
// voltage / maximum time between sensor calls / time considered
// for background calibration.
#define BME680_BSEC_CONFIG_GENERIC_18V_3S_4D        0
#define BME680_BSEC_CONFIG_GENERIC_18V_3S_28D       1
#define BME680_BSEC_CONFIG_GENERIC_18V_300S_4D      2
#define BME680_BSEC_CONFIG_GENERIC_18V_300S_28D     3
#define BME680_BSEC_CONFIG_GENERIC_33V_3S_4D        4
#define BME680_BSEC_CONFIG_GENERIC_33V_3S_28D       5
#define BME680_BSEC_CONFIG_GENERIC_33V_300S_4D      6
#define BME680_BSEC_CONFIG_GENERIC_33V_300S_28D     7

const uint8_t bsec_config_iaq[] = {
  #if BME680_BSEC_CONFIG == BME680_BSEC_CONFIG_GENERIC_18V_3S_4D
    #include <config/generic_18v_3s_28d/bsec_iaq.txt>
  #elif BME680_BSEC_CONFIG == BME680_BSEC_CONFIG_GENERIC_18V_3S_28D
    #include <config/generic_18v_300s_4d/bsec_iaq.txt>
  #elif BME680_BSEC_CONFIG == BME680_BSEC_CONFIG_GENERIC_18V_300S_4D
    #include <config/generic_18v_300s_28d/bsec_iaq.txt>
  #elif BME680_BSEC_CONFIG == BME680_BSEC_CONFIG_GENERIC_18V_300S_28D
    #include <config/generic_33v_3s_4d/bsec_iaq.txt>
  #elif BME680_BSEC_CONFIG == BME680_BSEC_CONFIG_GENERIC_33V_3S_4D
    #include <config/generic_33v_3s_28d/bsec_iaq.txt>
  #elif BME680_BSEC_CONFIG == BME680_BSEC_CONFIG_GENERIC_33V_3S_28D
    #include <config/generic_33v_300s_4d/bsec_iaq.txt>
  #elif BME680_BSEC_CONFIG == BME680_BSEC_CONFIG_GENERIC_33V_300S_4D
    #include <config/generic_33v_300s_28d/bsec_iaq.txt>
  #elif BME680_BSEC_CONFIG == BME680_BSEC_CONFIG_GENERIC_33V_300S_28D
    #include <config/generic_33v_3s_4d/bsec_iaq.txt>
  #endif
};

class BME680Sensor : public I2CSensor<> {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        BME680Sensor() {
            _sensor_id = SENSOR_BME680_ID;
            _count = 9;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        void begin() {
            if (!_dirty) {
              return;
            }

            // I2C auto-discover
            unsigned char addresses[] = {BME680_I2C_ADDR_PRIMARY, BME680_I2C_ADDR_SECONDARY};
            _address = _begin_i2c(_address, sizeof(addresses), addresses);
            if (_address == 0) return;

            iaqSensor.begin(_address, Wire);

            #if SENSOR_DEBUG
                DEBUG_MSG("[BME680] BSEC library version v%u.%u.%u.%u\n",
                  iaqSensor.version.major,
                  iaqSensor.version.minor,
                  iaqSensor.version.major_bugfix,
                  iaqSensor.version.minor_bugfix
                );
            #endif

            if (!_isSensorOk()) {
              _error = SENSOR_ERROR_OTHER;
              return;
            }

            iaqSensor.setConfig(bsec_config_iaq);

            _loadState();

            float sampleRate;

            // BSEC configuration with 300s allows for the sensor to sleep for 300s
            // on the ULP mode in order to minimize power consumption.
            if (BME680_BSEC_CONFIG == BME680_BSEC_CONFIG_GENERIC_18V_300S_4D ||
              BME680_BSEC_CONFIG == BME680_BSEC_CONFIG_GENERIC_18V_300S_4D ||
              BME680_BSEC_CONFIG == BME680_BSEC_CONFIG_GENERIC_18V_300S_4D) {
              sampleRate = BSEC_SAMPLE_RATE_ULP;
            } else {
              sampleRate = BSEC_SAMPLE_RATE_LP;
            }

            iaqSensor.updateSubscription(sensorList, 12, sampleRate);

            if (!_isSensorOk()) {
              _error = SENSOR_ERROR_OTHER;
              return;
            }

            _ready = true;
            _dirty = false;
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[21];
            snprintf(buffer, sizeof(buffer), "BME680 @ I2C (0x%02X)", _address);
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_TEMPERATURE;
            if (index == 1) return MAGNITUDE_HUMIDITY;
            if (index == 2) return MAGNITUDE_PRESSURE;
            if (index == 3) return MAGNITUDE_RESISTANCE;
            if (index == 4) return MAGNITUDE_IAQ_ACCURACY;
            if (index == 5) return MAGNITUDE_IAQ;
            if (index == 6) return MAGNITUDE_IAQ_STATIC;
            if (index == 7) return MAGNITUDE_CO2;
            if (index == 8) return MAGNITUDE_VOC;

            return MAGNITUDE_NONE;
        }

        void _loadState() {
            String storedState = getSetting("bsecState");

            if (storedState.length() == 0) {
              #if SENSOR_DEBUG
                  DEBUG_MSG("[BME680] Previous state not found\n");
              #endif

              return;
            }

            hexDecode(storedState.c_str(), storedState.length(), _bsecState, sizeof(_bsecState));

            iaqSensor.setState(_bsecState);

            #if SENSOR_DEBUG
                DEBUG_MSG("[BME680] Loaded previous state %s\n", storedState.c_str());
            #endif
        }

        void _saveState() {
            if (BME680_STATE_SAVE_INTERVAL == 0) {
                return;
            }

            static unsigned long last_millis = 0;

            if (_iaqAccuracy < 3 || (millis() - last_millis < BME680_STATE_SAVE_INTERVAL)) {
                return;
            }

            iaqSensor.getState(_bsecState);

            char storedState[BSEC_MAX_STATE_BLOB_SIZE * 2 + 1] = {0};
            hexEncode(_bsecState, BSEC_MAX_STATE_BLOB_SIZE, storedState, sizeof(storedState));

            setSetting("bsecState", storedState);

            last_millis = millis();
        }

        // The maximum allowed time between two `bsec_sensor_control` calls depends on
        // configuration profile `bsec_config_iaq` below.
        void tick() {
            _error = SENSOR_ERROR_OK;

            if (!_isSensorOk()) {
              _error = SENSOR_ERROR_OTHER;
              return;
            }

            if (iaqSensor.run()) {
              _rawTemperature = iaqSensor.rawTemperature;
              _rawHumidity = iaqSensor.rawHumidity;
              _temperature = iaqSensor.temperature;
              _humidity = iaqSensor.humidity;
              _pressure = iaqSensor.pressure / 100;
              _gasResistance = iaqSensor.gasResistance;
              _iaqAccuracy = iaqSensor.iaqAccuracy;
              _iaq = iaqSensor.iaq;
              _iaqStatic = iaqSensor.staticIaq;
              _co2Equivalent = iaqSensor.co2Equivalent;
              _breathVocEquivalent = iaqSensor.breathVocEquivalent;

              _saveState();
            }
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return _temperature;
            if (index == 1) return _humidity;
            if (index == 2) return _pressure;
            if (index == 3) return _gasResistance;
            if (index == 4) return _iaqAccuracy;
            if (index == 5) return _iaq;
            if (index == 6) return _iaqStatic;
            if (index == 7) return _co2Equivalent;
            if (index == 8) return _breathVocEquivalent;

            return 0;
        }

    protected:

        bool _isSensorOk() {
            if (iaqSensor.status == BSEC_OK && iaqSensor.bme680Status == BME680_OK) {
                return true;
            }

            #if SENSOR_DEBUG
            if (iaqSensor.status != BSEC_OK) {
              if (iaqSensor.status < BSEC_OK) {
                DEBUG_MSG("[BME680] BSEC error code %d\n", iaqSensor.status);
              } else {
                DEBUG_MSG("[BME680] BSEC warning code %d\n", iaqSensor.status);
              }
            }
            #endif

            #if SENSOR_DEBUG
            if (iaqSensor.bme680Status != BME680_OK) {
              if (iaqSensor.bme680Status < BME680_OK) {
                DEBUG_MSG("[BME680] Error code %d\n", iaqSensor.bme680Status);
              } else {
                DEBUG_MSG("[BME680] Warning code %d\n", iaqSensor.bme680Status);
              }
            }
            #endif

            return false;
        }

        bsec_virtual_sensor_t sensorList[12] = {
            BSEC_OUTPUT_RAW_TEMPERATURE,                      // Unscaled (raw) temperature (ºC).

            BSEC_OUTPUT_RAW_PRESSURE,                         // Unscaled (raw) pressure (Pa).

            BSEC_OUTPUT_RAW_HUMIDITY,                         // Unscaled (raw) relative humidity (%).

            BSEC_OUTPUT_RAW_GAS,                              // Gas resistance (Ohm). The resistance value changes according to the
                                                              // VOC concentration (the higher the concentration of reducing VOCs,
                                                              // the lower the resistance and vice versa).

            BSEC_OUTPUT_IAQ,                                  // Scaled Indoor Air Quality based on the recent sensor history, ideal
                                                              // for mobile applications (e.g. carry-on devices). The scale ranges from
                                                              // 0 (clean air) to 500 (heavily polluted air). The automatic background
                                                              // calibration process ensures that consistent IAQ performance is achieved
                                                              // after certain of days (depending on BSEC configuration - 4d or 28d).

            BSEC_OUTPUT_STATIC_IAQ,                           // Unscaled Indoor Air Quality, optimized for stationary applications
                                                              // (e.g. fixed indoor devices).
            BSEC_OUTPUT_CO2_EQUIVALENT,                       // Estimate of CO2 measured in the air.
            BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,                // Breath VOC represents the most important compounds in an exhaled
                                                              // breath of healthy humans.

            BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,  // Temperature compensated for the influence of sensor heater (ºC).

            BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,     // Relative humidity compensated for the influence of sensor heater (%).

            BSEC_OUTPUT_STABILIZATION_STATUS,                 // Indicates initial stabilization status of the gas sensor element:
                                                              // ongoing (0) or finished (1).

            BSEC_OUTPUT_RUN_IN_STATUS,                        // Indicates power-on stabilization status of the gas sensor element:
                                                              // ongoing (0) or finished (1).
        };

        float _breathVocEquivalent = 0.0f;
        float _co2Equivalent = 0.0f;
        float _gasResistance = 0.0f;
        float _humidity = 0.0f;
        float _iaq = 0.0f;
        float _pressure = 0.0f;
        float _rawHumidity = 0.0f;
        float _rawTemperature = 0.0f;
        float _temperature = 0.0f;
        uint8_t _bsecState[BSEC_MAX_STATE_BLOB_SIZE] = {0};
        uint8_t _iaqAccuracy = 0;
        float _iaqStatic = 0;

        Bsec iaqSensor;

};

#endif // SENSOR_SUPPORT && BME680_SUPPORT
