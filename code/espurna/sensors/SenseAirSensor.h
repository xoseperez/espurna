// -----------------------------------------------------------------------------
// SenseAir S8 CO2 Sensor
// Contribution by Yonsm Guo
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && SENSEAIR_SUPPORT

#pragma once

#include "BaseSensor.h"

// SenseAir sensor utils. Notice that we only read a single register.
// 0xFE is the address aka "Any sensor"
class SenseAir {
public:
    struct ValueResult {
        int16_t value { 0 };
        uint8_t error { 0 };
        bool status { false };
    };

    static ValueResult readCo2(Stream& stream) {
        return sendFrame(stream, buildFrame(0xFE, 0x04, 0x03, 1));
    }

private:
    using Frame = std::array<uint8_t, 8>;

    static uint16_t modRTU_CRC(const uint8_t* begin, const uint8_t* end) {
        uint16_t crc = 0xFFFF;

        for (auto it = begin; it != end; ++it) {
            crc ^= (uint16_t)(*it);              // XOR byte into least sig. byte of crc

            for (size_t i = 8; i != 0; i--) {    // Loop over each bit
                if ((crc & 0x0001) != 0) {       // If the LSB is set
                    crc >>= 1;                   // Shift right and XOR 0xA001
                    crc ^= 0xA001;
                } else {                         // Else LSB is not set
                    crc >>= 1;                   // Just shift right
                }
            }
        }

        return crc;
    }

    static Frame buildFrame(
        uint8_t slaveAddress,
        uint8_t  functionCode,
        uint16_t startAddress,
        uint16_t numberOfRegisters)
    {
        Frame out;

        out[0] = slaveAddress;
        out[1] = functionCode;
        out[2] = (uint8_t)(startAddress >> 8);
        out[3] = (uint8_t)(startAddress);
        out[4] = (uint8_t)(numberOfRegisters >> 8);
        out[5] = (uint8_t)(numberOfRegisters);

        // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
        uint16_t crc = modRTU_CRC(&out[0], &out[6]);
        out[6] = (uint8_t)(crc & 0xFF);
        out[7] = (uint8_t)((crc >> 8) & 0xFF);

        return out;
    }

    static void discardData(Stream& stream) {
        while (stream.available() > 0) {
            stream.read();
        }
    }

    // Since we only care about the CO2 value, frame size is known in advance
    // Only exceptional things are:
    // - read timeout, read values would be 0 (zero-init'ed)
    // - error code, reported through the result object
    // - unexpected value length, in case address / register numbers change
    // - invalid crc due to faulty transmission

    static ValueResult sendFrame(Stream& stream, const Frame& frame) {
        stream.write(frame.data(), frame.size());
        delay(50);

        ValueResult out;

        uint8_t buffer[7] = {0};
        stream.readBytes(buffer, 3);
        if (buffer[0] != 0xFE) {
            discardData(stream);
            return out;
        }

        if (buffer[1] & 0x80) {
            out.error = buffer[2];
            discardData(stream);
            return out;
        }

        if (buffer[2] != 2) {
            discardData(stream);
            return out;
        }

        stream.readBytes(&buffer[3], 4);

        const uint16_t received = (buffer[6] << 8) | buffer[5];
        const uint16_t calculated = modRTU_CRC(std::begin(buffer), std::end(buffer) - 2);
        if (received != calculated) {
            return out;
        }

        out.value = (buffer[4] << 8) | (buffer[3]);
        out.status = true;

        return out;
    }
};

class SenseAirSensor : public BaseSensor, SenseAir {
    public:

        void setPort(Stream* port) {
            _serial = port;
            _dirty = true;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_SENSEAIR_ID;
        }

        unsigned char count() const override {
            return 1;
        }

        // Initialization method, must be idempotent
        void begin() override {
            if (!_dirty) return;

            _startTime = TimeSource::now();
            _warmedUp = false;

            _ready = true;
            _dirty = false;
        }

        // Descriptive name of the sensor
        String description() const override {
            return F("SenseAir");
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char) const override {
            return String(SENSEAIR_PORT, 10);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index == 0) {
                return MAGNITUDE_CO2;
            }

            return MAGNITUDE_NONE;
        }

        void pre() override {
            static constexpr auto WarmupDuration = espurna::duration::Seconds(20);
            if (!_warmedUp && (TimeSource::now() - _startTime < WarmupDuration)) {
                _error = SENSOR_ERROR_WARM_UP;
                return;
            }

            _warmedUp = true;

            const auto result = readCo2(*_serial);
            if (!result.status) {
                if (result.error) {
                    DEBUG_MSG_P(PSTR("[SENSEAIR] Modbus error: 0x%02X\n"), result.error);
                }
                _error = SENSOR_ERROR_OTHER;
                return;
            }

            const auto co2 = result.value;
            if (co2 >= 5000 || co2 < 100)
            {
                _co2 = _lastCo2;
                _error = SENSOR_ERROR_OUT_OF_RANGE;
            }
            else
            {
                _co2 = (co2 > _lastCo2 + 2000) ? _lastCo2 : co2;
                _lastCo2 = co2;
                _error = SENSOR_ERROR_OK;
            }
        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (index == 0) {
                return _co2;
            }
            return 0.0;
        }

    private:
        Stream* _serial;

        using TimeSource = espurna::time::CoreClock;
        TimeSource::time_point _startTime;
        bool _warmedUp = false;

        int16_t _co2 = 0;
        int16_t _lastCo2 = 0;
};


#endif // SENSOR_SUPPORT && SENSEAIR_SUPPORT
