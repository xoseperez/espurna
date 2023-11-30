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

#include "I2CSensor.h"

#include <bsec.h>

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
        // Sensor API
        // ---------------------------------------------------------------------

        static constexpr Magnitude Magnitudes[] {
            MAGNITUDE_TEMPERATURE,
            MAGNITUDE_HUMIDITY,
            MAGNITUDE_PRESSURE,
            MAGNITUDE_RESISTANCE,
            MAGNITUDE_IAQ_ACCURACY,
            MAGNITUDE_IAQ,
            MAGNITUDE_IAQ_STATIC,
            MAGNITUDE_CO2,
            MAGNITUDE_VOC,
        };

        unsigned char id() const override {
            return SENSOR_BME680_ID;
        }

        unsigned char count() const override {
            return std::size(Magnitudes);
        }

        void begin() override {
            if (!_dirty) {
              return;
            }

            // I2C auto-discover
            static constexpr uint8_t addresses[] {BME680_I2C_ADDR_PRIMARY, BME680_I2C_ADDR_SECONDARY};
            auto address = findAndLock(addresses);
            if (address == 0) {
              return;
            }

            iaqSensor.begin(address, Wire);

            DEBUG_MSG_P(PSTR("[BME680] BSEC library version v%u.%u.%u.%u\n"),
                iaqSensor.version.major,
                iaqSensor.version.minor,
                iaqSensor.version.major_bugfix,
                iaqSensor.version.minor_bugfix
            );

            if (!_isOk()) {
              _showSensorErrors();
              _error = SENSOR_ERROR_OTHER;
              return;
            }

            iaqSensor.setConfig(bsec_config_iaq);

            _loadState();
            _last_state = TimeSource::now();

            // BSEC configuration with 300s allows for the sensor to sleep for 300s
            // on the ULP mode in order to minimize power consumption.
            const float sampleRate =
                ((BME680_BSEC_CONFIG == BME680_BSEC_CONFIG_GENERIC_18V_300S_4D)
                 || (BME680_BSEC_CONFIG == BME680_BSEC_CONFIG_GENERIC_18V_300S_4D)
                 || (BME680_BSEC_CONFIG == BME680_BSEC_CONFIG_GENERIC_18V_300S_4D))
                ? BSEC_SAMPLE_RATE_ULP
                : BSEC_SAMPLE_RATE_LP;

            iaqSensor.updateSubscription(
                SensorList, std::size(SensorList), sampleRate);

            if (!_isOk()) {
              _showSensorErrors();
              _error = SENSOR_ERROR_OTHER;
              return;
            }

            _error = SENSOR_ERROR_OK;
            _ready = true;
            _dirty = false;
        }

        // Descriptive name of the sensor
        String description() const override {
            char buffer[21];
            snprintf_P(buffer, sizeof(buffer),
                PSTR("BME680 @ I2C (0x%02X)"), lockedAddress());
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index < std::size(Magnitudes)) {
                return Magnitudes[index].type;
            }

            return MAGNITUDE_NONE;
        }

        // The maximum allowed time between two `bsec_sensor_control` calls depends on
        // configuration profile `bsec_config_iaq` below.
        void tick() override {
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
              _error = SENSOR_ERROR_OK;
            } else if (_isError()) {
              _error = SENSOR_ERROR_OTHER;
            }
        }

        // Ensure we show any possible issues with the sensor, post() is called regardless of sensor status() / error() codes
        void post() override {
          _showSensorErrors();
        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (index < std::size(Magnitudes)) {
              switch (Magnitudes[index].type) {
              case MAGNITUDE_TEMPERATURE:
                return _temperature;
              case MAGNITUDE_HUMIDITY:
                return _humidity;
              case MAGNITUDE_PRESSURE:
                return _pressure;
              case MAGNITUDE_RESISTANCE:
                return _gasResistance;
              case MAGNITUDE_IAQ_ACCURACY:
                return _iaqAccuracy;
              case MAGNITUDE_IAQ:
                return _iaq;
              case MAGNITUDE_IAQ_STATIC:
                return _iaqStatic;
              case MAGNITUDE_CO2:
                return _co2Equivalent;
              case MAGNITUDE_VOC:
                return _breathVocEquivalent;
              }
            }

            return 0;
        }

    protected:

        void _loadState() {
            auto storedState = getSetting("bsecState");
            if (!storedState.length()) {
              return;
            }

            if (hexDecode(storedState.c_str(), storedState.length(), _state, sizeof(_state))) {
              DEBUG_MSG_P(PSTR("[BME680] Restoring saved state...\n"));

              iaqSensor.setState(_state);
              _showSensorErrors();
            }
        }

        void _saveState() {
            if (!SaveInterval.count()) {
              return;
            }

            if (_iaqAccuracy < 3) {
              return;
            }

            const auto now = TimeSource::now();
            if (now - _last_state < SaveInterval) {
              return;
            }

            iaqSensor.getState(_state);
            setSetting("bsecState", hexEncode(_state));

            _last_state = now;
        }

        bool _isError() const {
            return (iaqSensor.status < BSEC_OK)
                || (iaqSensor.bme680Status < BME680_OK);
        }

        bool _isOk() const {
            return (iaqSensor.status == BSEC_OK)
                && (iaqSensor.bme680Status == BME680_OK);
        }

        void _showSensorErrors() const {
            // see `enum { ... } bsec_library_return_t` values & description at:
            // BSEC Software Library/src/inc/bsec_datatypes.h
            if (iaqSensor.status != BSEC_OK) {
              if (iaqSensor.status < BSEC_OK) {
                DEBUG_MSG_P(PSTR("[BME680] BSEC error code (%d)\n"), iaqSensor.status);
              } else {
                DEBUG_MSG_P(PSTR("[BME680] BSEC warning code (%d)\n"), iaqSensor.status);
              }
            }

            // see `BME680_{W,E}_...` at:
            // BSEC Software Library/src/bme680/bme680_defs.h
            switch (iaqSensor.bme680Status) {
            case BME680_OK:
              break;
            case BME680_E_COM_FAIL:
            case BME680_E_DEV_NOT_FOUND:
              DEBUG_MSG_P(PSTR("[BME680] Communication error / device not found (%d)\n"), iaqSensor.bme680Status);
            case BME680_W_DEFINE_PWR_MODE:
              DEBUG_MSG_P(PSTR("[BME680] Power mode not defined (%d)\n"), iaqSensor.bme680Status);
              break;
            case BME680_W_NO_NEW_DATA:
              DEBUG_MSG_P(PSTR("[BME680] No new data (%d)\n"), iaqSensor.bme680Status);
              break;
            default:
              if (iaqSensor.bme680Status < BME680_OK) {
                DEBUG_MSG_P(PSTR("[BME680] Error code (%d)\n"), iaqSensor.bme680Status);
              } else {
                DEBUG_MSG_P(PSTR("[BME680] Warning code (%d)\n"), iaqSensor.bme680Status);
              }
              break;
            }
        }

        bsec_virtual_sensor_t SensorList[12] = {
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

        float _temperature = 0.0f;
        float _rawTemperature = 0.0f;

        float _rawHumidity = 0.0f;
        float _humidity = 0.0f;

        float _pressure = 0.0f;
        float _gasResistance = 0.0f;

        uint8_t _iaqAccuracy = 0;
        float _iaq = 0.0f;
        float _iaqStatic = 0;

        float _breathVocEquivalent = 0.0f;

        float _co2Equivalent = 0.0f;

        using TimeSource = espurna::time::CoreClock;
        static constexpr auto SaveInterval = TimeSource::duration { BME680_STATE_SAVE_INTERVAL };
        TimeSource::time_point _last_state;
        uint8_t _state[BSEC_MAX_STATE_BLOB_SIZE] = {0};

        Bsec iaqSensor;

};

#if __cplusplus < 201703L
constexpr BaseSensor::Magnitude BME680Sensor::Magnitudes[];
#endif

#endif // SENSOR_SUPPORT && BME680_SUPPORT
