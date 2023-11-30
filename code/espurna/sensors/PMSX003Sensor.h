// -----------------------------------------------------------------------------
// PMS Dust Sensor
// Contribution by Òscar Rovira López
// Refine to support PMS5003T/PMS5003ST by Yonsm Guo
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && PMSX003_SUPPORT

#pragma once

#include "BaseSensor.h"

// Type of sensor
#define PMS_TYPE_X003       0
#define PMS_TYPE_X003_9     1
#define PMS_TYPE_5003T      2
#define PMS_TYPE_5003ST     3
#define PMS_TYPE_5003S      4

// Sensor type specified data
#define PMS_SLOT_MAX        4
#define PMS_DATA_MAX        17

// [MAGIC][LEN][DATA9|13|17][SUM]
inline int PMS_PACKET_SIZE(int size) {
    return (size + 3) * 2;
}

inline int PMS_PAYLOAD_SIZE(int size) {
    return (size + 1) * 2;
}

// PMS sensor utils
// Command functions copied from: https://github.com/fu-hsi/PMS/blob/master/src/PMS.cpp
// Reading function is rewrited to support flexible reading for PMS5003T/PMS5003ST
class PMSX003 {

    protected:
        Stream *_serial = nullptr; // Should initialized by child class

    public:

        // Standby mode. For low power consumption and prolong the life of the sensor.
        inline void sleep() {
            const uint8_t command[] { 0x42, 0x4D, 0xE4, 0x00, 0x00, 0x01, 0x73 };
            _serial->write(command, sizeof(command));
        }

        // Operating mode. Stable data should be got at least 30 seconds after the sensor wakeup from the sleep mode because of the fan's performance.
        inline void wakeUp() {
            const uint8_t command[] { 0x42, 0x4D, 0xE4, 0x00, 0x01, 0x01, 0x74 };
            _serial->write(command, sizeof(command));
        }

        // Active mode. Default mode after power up. In this mode sensor would send serial data to the host automatically.
        inline void activeMode() {
            const uint8_t command[] { 0x42, 0x4D, 0xE1, 0x00, 0x01, 0x01, 0x71 };
            _serial->write(command, sizeof(command));
        }

        // Passive mode. In this mode, sensor would send serial data to the host only for request.
        inline void passiveMode() {
            const uint8_t command[] { 0x42, 0x4D, 0xE1, 0x00, 0x00, 0x01, 0x70 };
            _serial->write(command, sizeof(command));
        }

        // Request read, ONLY needed in Passive Mode!!
        inline void requestRead() {
            const uint8_t command[] { 0x42, 0x4D, 0xE2, 0x00, 0x00, 0x01, 0x71 };
            _serial->write(command, sizeof(command));
        }

        // Read sensor's data
        bool readData(uint16_t* data, size_t data_count) {

            do {

                int avail = _serial->available();
#if SENSOR_DEBUG
                DEBUG_MSG_P(PSTR("[SENSOR] PMS: Packet available = %d\n"), avail);
#endif
                if (avail < PMS_PACKET_SIZE(data_count)) {
                    break;
                }

                if (_serial->read() == 0x42 && _serial->read() == 0x4D) {

                    uint16_t sum = 0x42 + 0x4D;
                    uint16_t size = read16(sum);
                    if (size != PMS_PAYLOAD_SIZE(data_count)) {
#if SENSOR_DEBUG
                        DEBUG_MSG_P(PSTR("[SENSOR] PMS: Payload size: %hu != %zu.\n"),
                                size, PMS_PAYLOAD_SIZE(data_count));
#endif
                        break;
                    }

                    for (size_t i = 0; i < data_count; i++) {
                        data[i] = read16(sum);
#if SENSOR_DEBUG
                        DEBUG_MSG_P(PSTR("[SENSOR] PMS:   data[%zu] = %hu\n"), i, data[i]);
#endif
                    }

                    uint16_t checksum = read16();
                    if (sum == checksum) {
                        return true;
                    } else {
#if SENSOR_DEBUG
                        DEBUG_MSG_P(PSTR("[SENSOR] PMS checksum: %04X != %04X\n"), sum, checksum);
#endif
                    }
                    break;
                }

            } while (true);

            return false;

        }

    private:

        // Read 16-bit
        uint16_t read16() {
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

    private:

        struct Spec {
            const char *name;
            unsigned char data_count;
            unsigned char slot_count;
            unsigned char slot_types[PMS_SLOT_MAX];
        };

        static constexpr Spec Specs[] {
            {"PMSX003", 13, 3, {MAGNITUDE_PM1DOT0, MAGNITUDE_PM2DOT5, MAGNITUDE_PM10}},
            {"PMSX003_9", 9, 3, {MAGNITUDE_PM1DOT0, MAGNITUDE_PM2DOT5, MAGNITUDE_PM10}},
            {"PMS5003T", 13, 3, {MAGNITUDE_PM2DOT5, MAGNITUDE_TEMPERATURE, MAGNITUDE_HUMIDITY}},
            {"PMS5003ST", 17, 4, {MAGNITUDE_PM2DOT5, MAGNITUDE_TEMPERATURE, MAGNITUDE_HUMIDITY, MAGNITUDE_HCHO}},
            {"PMS5003S", 13, 3, {MAGNITUDE_PM2DOT5, MAGNITUDE_PM10, MAGNITUDE_HCHO}},
        };

    public:
        void setPort(Stream* port) {
            _serial = port;
            _dirty = true;
        }

        // Should call setType after constructor immediately to enable corresponding slot count
        void setType(unsigned char type) {
            _type = type;
        }

        unsigned char getType() {
            return _type;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_PMSX003_ID;
        }

        unsigned char count() const override {
            return Specs[_type].slot_count;
        }

        // Initialization method, must be idempotent
        void begin() override {
            if (!_dirty) return;

            passiveMode();

            _startTime = TimeSource::now();
            _warmedUp = false;
            _ready = true;
            _dirty = false;
        }

        // Descriptive name of the sensor
        String description() const override {
            return String(Specs[_type].name);
        }

        // Descriptive name of the slot # index
        String description(unsigned char index) const override {
            String out;
            out += String(index + 1, 10);
            out += " @ ";
            out += description();
            return out;
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) const override {
            return String(PMS_PORT, 10);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            return Specs[_type].slot_types[index];
        }

        void pre() override {

            static constexpr auto WarmupDuration = espurna::duration::Seconds(30);
            if (!_warmedUp && TimeSource::now() - _startTime < WarmupDuration) {
                _error = SENSOR_ERROR_WARM_UP;
                return;
            }

            _warmedUp = true;

            #if PMS_SMART_SLEEP
                unsigned int readCycle;
                if (_readCount++ > 30) {
                    readCycle = _readCount % 30;
                    if (readCycle == 0) {
#if SENSOR_DEBUG
                        DEBUG_MSG_P(PSTR("[SENSOR] %s: Wake up: %d\n"),
                            Specs[_type].name, _readCount);
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
                   if (_readCount == 1) {
                       wakeUp();
                   }
                }
            #endif

            uint16_t data[PMS_DATA_MAX];
            if (readData(data, Specs[_type].data_count)) {
                if (_type == PMS_TYPE_5003ST) {
                    if (data[14] > 10 && data[14] < 1000 && data[13] < 1000) {
                        _slot_values[0] = data[4];
                        _slot_values[1] = (double)data[13] / 10;
                        _slot_values[2] = (double)data[14] / 10;
                        _slot_values[3] = (double)data[12] / 1000;
                        _error = SENSOR_ERROR_OK;
                    } else {
                        _error = SENSOR_ERROR_OUT_OF_RANGE;
#if SENSOR_DEBUG
                        DEBUG_MSG_P(PSTR("[SENSOR] %s: Invalid temperature=%d humidity=%d.\n"),
                            Specs[_type].name, (int)data[13], (int)data[14]);
#endif
                    }
                } else if (_type == PMS_TYPE_5003S) {
                    _slot_values[0] = data[4];
                    _slot_values[1] = data[5];
                    _slot_values[2] = (double)data[12] / 1000;
                    _error = SENSOR_ERROR_OK;
                } else if (_type == PMS_TYPE_5003T) {
                    if (data[11] > 10 && data[11] < 1000 && data[10] < 1000) {
                        _slot_values[0] = data[4];
                        _slot_values[1] = (double)data[10] / 10;
                        _slot_values[2] = (double)data[11] / 10;
                        _error = SENSOR_ERROR_OK;
                    } else {
                        _error = SENSOR_ERROR_OUT_OF_RANGE;
#if SENSOR_DEBUG
                        DEBUG_MSG_P(PSTR("[SENSOR] %s: Invalid temperature=%d humidity=%d.\n"),
                            Specs[_type].name, (int)data[10], (int)data[11]);
#endif
                    }
                } else {
                    _slot_values[0] = data[3];
                    _slot_values[1] = data[4];
                    _slot_values[2] = data[5];
                    _error = SENSOR_ERROR_OK;
                }
            }

            #if PMS_SMART_SLEEP
                if (readCycle == 6) {
                    sleep();
#if SENSOR_DEBUG
                    DEBUG_MSG_P(PSTR("[SENSOR] %s: Enter sleep mode: %d\n"),
                        Specs[_type].name, _readCount);
#endif
                    return;
                }
            #endif

            requestRead();

        }

        // Current value for slot # index
        double value(unsigned char index) override {
            return _slot_values[index];
        }

    protected:
        using TimeSource = espurna::time::CoreClock;
        TimeSource::time_point _startTime;
        bool _warmedUp = false;

        unsigned char _type = PMS_TYPE_X003;
        double _slot_values[PMS_SLOT_MAX] = {0};

#if PMS_SMART_SLEEP
        size_t _readCount = 0;
#endif

};

#if __cplusplus < 201703L
constexpr PMSX003Sensor::Spec PMSX003Sensor::Specs[];
#endif

#endif // SENSOR_SUPPORT && PMSX003_SUPPORT
