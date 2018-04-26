// -----------------------------------------------------------------------------
// PMSX003 Dust Sensor
// Uses SoftwareSerial library
// Contribution by Òscar Rovira López
// Refine to support PMS5003T/PMS5003ST by Yonsm Guo
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && PMSX003_SUPPORT

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"

#include <PMS.h>
#include <SoftwareSerial.h>

//
#define PMS_TYPE_X003    0
#define PMS_TYPE_X003_9  1
#define PMS_TYPE_5003T   2
#define PMS_TYPE_5003ST  3

#ifndef PMS_TYPE
#define PMS_TYPE PMS_TYPE_X003
#endif

// You can enable smart sleep (read 6-times then sleep on 24-reading-cycles) to extend PMS sensor's life.
// Otherwise the default lifetime of PMS sensor is about 8000-hours/1-years.
// The PMS's fan will stop working on sleeping cycle, and will wake up on reading cycle.
#ifndef PMS_SMART_SLEEP
#define PMS_SMART_SLEEP 0
#endif

// [MAGIC][LEN][DATA9|13|17][SUM]
#if PMS_TYPE == PMS_TYPE_5003ST
#define PMS_TYPE_NAME "PMS5003ST"
#define PMS_DATA_COUNT 17
#define PMS_SLOT_COUNT 4
#define PMS_SLOT_NAMES {"PM2.5", "TEMP", "HUMI", "HCHO"}
#define PMS_SLOT_TYPES {MAGNITUDE_PM2dot5, MAGNITUDE_TEMPERATURE, MAGNITUDE_HUMIDITY, MAGNITUDE_ANALOG}
#elif PMS_TYPE == PMS_TYPE_5003T
#define PMS_TYPE_NAME "PMS5003T"
#define PMS_DATA_COUNT 13
#define PMS_SLOT_COUNT 3
#define PMS_SLOT_NAMES {"PM2.5", "TEMP", "HUMI"}
#define PMS_SLOT_TYPES {MAGNITUDE_PM2dot5, MAGNITUDE_TEMPERATURE, MAGNITUDE_HUMIDITY}
#elif PMS_TYPE == PMS_TYPE_X003_9
#define PMS_TYPE_NAME "PMSX003_9"
#define PMS_DATA_COUNT 9
#define PMS_SLOT_COUNT 3
#define PMS_SLOT_NAMES {"PM1.0", "PM2.5", "PM10"}
#define PMS_SLOT_TYPES {MAGNITUDE_PM1dot0, MAGNITUDE_PM2dot5, MAGNITUDE_PM10}
#else
#define PMS_TYPE_NAME "PMSX003"
#define PMS_DATA_COUNT 13
#define PMS_SLOT_COUNT 3
#define PMS_SLOT_NAMES {"PM1.0", "PM2.5", "PM10"}
#define PMS_SLOT_TYPES {MAGNITUDE_PM1dot0, MAGNITUDE_PM2dot5, MAGNITUDE_PM10}
#endif

#define PMS_PACKET_SIZE ((PMS_DATA_COUNT + 3) * 2)
#define PMS_PAYLOAD_SIZE (PMS_DATA_COUNT * 2 + 2)


// PMSX003 sensor utils
// Command functions copied from: https://github.com/fu-hsi/PMS/blob/master/src/PMS.cpp
// Reading function is rewrited to support flexible reading for PMS5003T/PMS5003ST
class PMSX003 {
protected:
    SoftwareSerial *_serial = NULL; // Should initialized by child class

public:
    // Standby mode. For low power consumption and prolong the life of the sensor.
    inline void sleep() {
        uint8_t command[] = { 0x42, 0x4D, 0xE4, 0x00, 0x00, 0x01, 0x73 };
        _serial->write(command, sizeof(command));
    }

    // Operating mode. Stable data should be got at least 30 seconds after the sensor wakeup from the sleep mode because of the fan's performance.
    inline void wakeUp() {
        uint8_t command[] = { 0x42, 0x4D, 0xE4, 0x00, 0x01, 0x01, 0x74 };
        _serial->write(command, sizeof(command));
    }

    // Active mode. Default mode after power up. In this mode sensor would send serial data to the host automatically.
    inline void activeMode() {
        uint8_t command[] = { 0x42, 0x4D, 0xE1, 0x00, 0x01, 0x01, 0x71 };
        _serial->write(command, sizeof(command));
    }

    // Passive mode. In this mode, sensor would send serial data to the host only for request.
    inline void passiveMode() {
        uint8_t command[] = { 0x42, 0x4D, 0xE1, 0x00, 0x00, 0x01, 0x70 };
        _serial->write(command, sizeof(command));
    }

    // Request read, ONLY needed in Passive Mode!!
    inline void requestRead() {
        uint8_t command[] = { 0x42, 0x4D, 0xE2, 0x00, 0x00, 0x01, 0x71 };
        _serial->write(command, sizeof(command));
    }

    // Read sensor's data
    bool readData(uint16_t data[PMS_DATA_COUNT]) {
        do
        {
            int avail = _serial->available();
            #if SENSOR_DEBUG
                //debugSend("[SENSOR] %s: Packet available = %d\n", PMS_TYPE_NAME, avail);
            #endif
            if (avail < PMS_PACKET_SIZE)
                break;

            if (_serial->read() == 0x42 && _serial->read() == 0x4D)
            {
                uint16_t sum = 0x42 + 0x4D;
                uint16_t size = read16(sum);
                #if SENSOR_DEBUG
                    debugSend("[SENSOR] %s: Payload size = %d\n", PMS_TYPE_NAME, size);
                #endif
                if (size != PMS_PAYLOAD_SIZE)
                {
                    #if SENSOR_DEBUG
                        debugSend(("[SENSOR] %s: Payload size != %d \n"), PMS_TYPE_NAME, PMS_PAYLOAD_SIZE);
                    #endif
                    break;
                }

                for (int i = 0; i < PMS_DATA_COUNT; i++)
                {
                    data[i] = read16(sum);
                    #if SENSOR_DEBUG
                        //debugSend(("[SENSOR] %s:   data[%d] = %d\n"), PMS_TYPE_NAME, i, data[i]);
                    #endif
                }

                uint16_t checksum = read16();
                #if SENSOR_DEBUG
                    debugSend(("[SENSOR] %s:   Sum=%04X, Checksum=%04X\n"), PMS_TYPE_NAME, sum, checksum);
                #endif
                if (sum == checksum)
                {
                    return true;
                }
                break;
            }
        }
        while (true);
        return false;
    }

private:
    // Read 16-bit
    inline uint16_t read16() {
        return ((uint16_t) _serial->read()) << 8 | _serial->read();
    }

    // Read 16-bit and calculate checksum
    uint16_t read16(uint16_t &checksum) {
        uint8_t high = _serial->read();
        uint8_t low = _serial->read();
        checksum += high;
        checksum += low;
        return ((uint16_t) high) << 8 | low;
    }    
};

class PMSX003Sensor : public BaseSensor, PMSX003 {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        PMSX003Sensor(): BaseSensor() {
            _count = PMS_SLOT_COUNT;
            _sensor_id = SENSOR_PMSX003_ID;
        }

        ~PMSX003Sensor() {
            if (_serial) delete _serial;
        }

        void setRX(unsigned char pin_rx) {
            if (_pin_rx == pin_rx) return;
            _pin_rx = pin_rx;
            _dirty = true;
        }

        void setTX(unsigned char pin_tx) {
            if (_pin_tx == pin_tx) return;
            _pin_tx = pin_tx;
            _dirty = true;
        }

        // ---------------------------------------------------------------------

        unsigned char getRX() {
            return _pin_rx;
        }

        unsigned char getTX() {
            return _pin_tx;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            if (!_dirty) return;

            if (_serial) delete _serial;

            _serial = new SoftwareSerial(_pin_rx, _pin_tx, false, 64);
            _serial->enableIntTx(false);
            _serial->begin(9600);
            passiveMode();

            _startTime = millis();
            _ready = true;
            _dirty = false;

        }

        // Descriptive name of the sensor
        String description() {
            char buffer[28];
            snprintf(buffer, sizeof(buffer), "%s @ SwSerial(%u,%u)", PMS_TYPE_NAME, _pin_rx, _pin_tx);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            char buffer[36] = {0};
            const static char *_slot_names[] = PMS_SLOT_NAMES;
            snprintf(buffer, sizeof(buffer), "%s @ %s @ SwSerial(%u,%u)", _slot_names[index], PMS_TYPE_NAME, _pin_rx, _pin_tx);
            return String(buffer);
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            char buffer[6];
            snprintf(buffer, sizeof(buffer), "%u:%u", _pin_rx, _pin_tx);
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            const static unsigned char _slot_types[] = PMS_SLOT_TYPES;
            return _slot_types[index];
        }

        void pre() {

            if (millis() - _startTime < 30000) {
                _error = SENSOR_ERROR_WARM_UP;
                return;
            }

            _error = SENSOR_ERROR_OK;

            #if PMS_SMART_SLEEP
                unsigned int readCycle;
                if (_readCount++ > 30) {
                    readCycle = _readCount % 30;
                    if (readCycle == 0) {
                        #if SENSOR_DEBUG
                            debugSend("[SENSOR] %s: Wake up: %d\n", PMS_TYPE_NAME, _readCount);
                        #endif
                        wakeUp();
                        return;
                    } else if (readCycle == 1) {
                        requestRead();
                    } else if (readCycle > 6) {
                        return;
                    }
                } else {
                   readCycle  = -1;
                }
            #endif

            uint16_t data[PMS_DATA_COUNT];
            if (readData(data)) {
                #if PMS_TYPE == PMS_TYPE_5003ST
                    _slot_values[0] = data[4];
                    _slot_values[1] = (double)data[13] / 10;
                    _slot_values[2] = (double)data[14] / 10;
                    _slot_values[3] = (double)data[12] / 1000;
                #elif PMS_TYPE == PMS_TYPE_5003T
                    _slot_values[0] = data[4];
                    _slot_values[1] = (double)data[10] / 10;
                    _slot_values[2] = (double)data[11] / 10;
                #else
                    _slot_values[0] = data[3];
                    _slot_values[1] = data[4];
                    _slot_values[2] = data[5];
                #endif
            }

            #if PMS_SMART_SLEEP
                if (readCycle == 6) {
                    sleep();
                    #if SENSOR_DEBUG
                        debugSend("[SENSOR] %s: Enter sleep mode: %d\n", PMS_TYPE_NAME, _readCount);
                    #endif
                    return;
                }
            #endif

            requestRead();
        }

        // Current value for slot # index
        double value(unsigned char index) {
            return _slot_values[index];
        }

    protected:
        unsigned int _pin_rx;
        unsigned int _pin_tx;
        unsigned long _startTime;
        double _slot_values[PMS_SLOT_COUNT] = {0};

#if PMS_SMART_SLEEP
        unsigned int _readCount = 0;
#endif
};

#endif // SENSOR_SUPPORT && PMSX003_SUPPORT
