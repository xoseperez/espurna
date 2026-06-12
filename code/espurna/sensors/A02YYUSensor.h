// -----------------------------------------------------------------------------
// DYP A02YYUx Ultrasonic Distance Sensor
// Copyright (C) 2024 by tsouthiv @ gmail dot com
// -----------------------------------------------------------------------------

// This should be also compatible with other similar sensors like JSN-SR04T
// or AJ-SR04M in auto UART mode.

#if SENSOR_SUPPORT && A02YYU_SUPPORT

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"

class A02YYUSensor : public BaseSensor {

    public:

        void setPort(Stream* port) {
            _serial = port;
            _dirty = true;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_A02YYU_ID;
        }

        unsigned char count() const override {
            return 1;
        }

        // Initialization method, must be idempotent
        void begin() {
            if (!_dirty) return;
            _ready = true;
            _dirty = false;
        }

        // Descriptive name of the sensor
        String description() const override {
            return F("A02YYU");
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index == 0) return MAGNITUDE_DISTANCE;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (index == 0) return _distance;
            return 0;
        }

        // Loop-like method, call it in your main loop
        void tick() override {
            _read();
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        // Parse the frame buffer and verify the checksum, it returns either:
        //   the measured distance value (in mm)
        //   -1 out of sync
        //   -2 invalid checksum
        int _parse_frame()
        {
            if (_frame[0] != 0xff)
                return -1;

            int vh = _frame[1];
            int vl = _frame[2];
            int cs = _frame[3];

            if (cs != ((0xff + vh + vl) & 0xff))
                return -2;

            return (vh << 8) + vl;
        }

        void _read() {

            int available = _serial->available();

            while (available) 
            {
                // read bytes from the device into the frame buffer
                int n = _serial->readBytes(_frame + _free, min(_frame_len - _free, available));

                available -= n;
                _free += n;
                
                if (_free == _frame_len) // the frame buffer is filled
                {
                    int d = _parse_frame();
                    
                    if (d >= 0)
                    {
                        // distance successfully parsed
                        _distance = d / 1000.0;     // update the distance
                        _free = 0;                  // empty the frame buffer
                    } else {
                        // out of sync or corrupted, shift the frame buffer by one byte
                        for (int i = 0; i < _frame_len - 1; i++)
                            _frame[i] = _frame[i + 1];

                        _free = _frame_len - 1;
                    }
                }
            }
        }

        // ---------------------------------------------------------------------

        static constexpr int _frame_len = 4;
        uint8_t _frame[_frame_len];
        Stream* _serial { nullptr };
        double _distance = 0;           // measured distance
        int _free = 0;                  // index of the first free byte in the frame buffer
};

#endif // SENSOR_SUPPORT && A02YYU_SUPPORT

