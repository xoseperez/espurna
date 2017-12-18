// -----------------------------------------------------------------------------
// BME280 Sensor
// -----------------------------------------------------------------------------

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"
#include <SparkFunBME280.h>

#define BME280_ERROR_UNKNOW_CHIP    -1

class BME280Sensor : public BaseSensor {

    public:

        BME280Sensor(unsigned char address = BME280_ADDRESS): BaseSensor() {

            // Cache
            _address = address;
            _measurement_delay = bmeMeasurementTime();

            #if BME280_TEMPERATURE > 0
                ++_count;
            #endif
            #if BME280_HUMIDITY > 0
                ++_count;
            #endif
            #if BME280_PRESSURE > 0
                ++_count;
            #endif

            // Init
            bme = new BME280();
            bme->settings.commInterface = I2C_MODE;
            bme->settings.I2CAddress = _address;
            bme->settings.runMode = BME280_MODE;
            bme->settings.tStandby = 0;
            bme->settings.filter = 0;
            bme->settings.tempOverSample = BME280_TEMPERATURE;
            bme->settings.pressOverSample = BME280_PRESSURE;
            bme->settings.humidOverSample = BME280_HUMIDITY;

            // Fix when not measuring temperature, t_fine should have a sensible value
            if (BME280_TEMPERATURE == 0) bme->t_fine = 100000; // aprox 20ºC

            // Make sure sensor had enough time to turn on. BME280 requires 2ms to start up
            delay(10);

            // Check sensor correctly initialized
            unsigned char response = bme->begin();
            if (response == 0x60) {
                _ready = true;
            } else {
                _error = BME280_ERROR_UNKNOW_CHIP;
            }

        }

        // Descriptive name of the sensor
        String name() {
            char buffer[20];
            snprintf(buffer, sizeof(buffer), "BME280 @ I2C (0x%02X)", _address);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return name();
        }

        // Type for slot # index
        magnitude_t type(unsigned char index) {
            if (index < _count) {
                _error = SENSOR_ERROR_OK;
                unsigned char i = 0;
                #if BME280_TEMPERATURE > 0
                    if (index == i++) return MAGNITUDE_TEMPERATURE;
                #endif
                #if BME280_HUMIDITY > 0
                    if (index == i++) return MAGNITUDE_HUMIDITY;
                #endif
                #if BME280_PRESSURE > 0
                    if (index == i) return MAGNITUDE_PRESSURE;
                #endif
            }
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return MAGNITUDE_NONE;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        virtual void pre() {

            if (!_ready) {
                _error = BME280_ERROR_UNKNOW_CHIP;
                return;
            }

            #if BME280_MODE == 1
                bmeForceRead();
            #endif

        }

        // Current value for slot # index
        double value(unsigned char index) {

            if (index < _count) {
                _error = SENSOR_ERROR_OK;
                unsigned char i = 0;
                #if BME280_TEMPERATURE > 0
                    if (index == i++) return bme->readTempC();
                #endif
                #if BME280_HUMIDITY > 0
                    if (index == i++) return bme->readFloatHumidity();
                #endif
                #if BME280_PRESSURE > 0
                    if (index == i) return bme->readFloatPressure() / 100;
                #endif
            }
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return 0;
        }

    protected:

        unsigned long bmeMeasurementTime() {

            // Measurement Time (as per BME280 datasheet section 9.1)
            // T_max(ms) = 1.25
            //  + (2.3 * T_oversampling)
            //  + (2.3 * P_oversampling + 0.575)
            //  + (2.4 * H_oversampling + 0.575)
            //  ~ 9.3ms for current settings

            double t = 1.25;
            #if BME280_TEMPERATURE > 0
                t += (2.3 * BME280_TEMPERATURE);
            #endif
            #if BME280_HUMIDITY > 0
                t += (2.4 * BME280_HUMIDITY + 0.575);
            #endif
            #if BME280_PRESSURE > 0
                t += (2.3 * BME280_PRESSURE + 0.575);
            #endif

            return round(t + 1); // round up

        }

        void bmeForceRead() {

            // We set the sensor in "forced mode" to force a reading.
            // After the reading the sensor will go back to sleep mode.
            uint8_t value = bme->readRegister(BME280_CTRL_MEAS_REG);
            value = (value & 0xFC) + 0x01;
            bme->writeRegister(BME280_CTRL_MEAS_REG, value);

            delay(_measurement_delay);

        }

        // ---------------------------------------------------------------------

        BME280 * bme;
        unsigned char _address;
        unsigned long _measurement_delay;
        bool _ready = false;

};
