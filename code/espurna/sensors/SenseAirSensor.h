// -----------------------------------------------------------------------------
// SenseAir S8 CO2 Sensor
// Uses SoftwareSerial library
// Contribution by Yonsm Guo
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && SENSEAIR_SUPPORT

#pragma once

#include <Arduino.h>
#include <SoftwareSerial.h>

#include "BaseSensor.h"


// SenseAir sensor utils
class SenseAir
{
protected:
    SoftwareSerial *_serial; // Should initialized by child class

public:
    int sendCommand(byte command[]) {
        byte recv_buf[7] = {0xff};
        byte data_buf[2] = {0xff};
        long value       = -1;

        _serial->write(command, 8); //Send the byte array
        delay(50);

        // Read answer from sensor
        int ByteCounter = 0;
        while(_serial->available()) {
            recv_buf[ByteCounter] = _serial->read();
            ByteCounter++;
        }

        data_buf[0] = recv_buf[3];
        data_buf[1] = recv_buf[4];
        value = (data_buf[0] << 8) | (data_buf[1]);

        return value;
    }

    int readCo2(void) {
        int co2 = 0;
        byte frame[8] = {0};
        buildFrame(0xFE, 0x04, 0x03, 1, frame);
        co2 = sendCommand(frame);
        return co2;
    }

private:
    // Compute the MODBUS RTU CRC
    static unsigned int modRTU_CRC(byte buf[], int len, byte checkSum[2]) {
        unsigned int crc = 0xFFFF;

        for (int pos = 0; pos < len; pos++) {
            crc ^= (unsigned int)buf[pos];          // XOR byte into least sig. byte of crc

            for (int i = 8; i != 0; i--) {    // Loop over each bit
            if ((crc & 0x0001) != 0) {      // If the LSB is set
                crc >>= 1;                    // Shift right and XOR 0xA001
                crc ^= 0xA001;
            }
            else                            // Else LSB is not set
                crc >>= 1;                    // Just shift right
            }
        }
        // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
        checkSum[1] = (byte)((crc >> 8) & 0xFF);
        checkSum[0] = (byte)(crc & 0xFF);
        return crc;
    }

    static int getBitOfInt(int reg, int pos) {
        // Create a mask
        int mask = 0x01 << pos;

        // Mask the status register
        int masked_register = mask & reg;

        // Shift the result of masked register back to position 0
        int result = masked_register >> pos;

        return result;
    }


    static void buildFrame(byte slaveAddress,
                byte  functionCode,
                short startAddress,
                short numberOfRegisters,
                byte frame[8]) {
        frame[0] = slaveAddress;
        frame[1] = functionCode;
        frame[2] = (byte)(startAddress >> 8);
        frame[3] = (byte)(startAddress);
        frame[4] = (byte)(numberOfRegisters >> 8);
        frame[5] = (byte)(numberOfRegisters);
        // CRC-calculation
        byte checkSum[2] = {0};
        modRTU_CRC(frame, 6, checkSum);
        frame[6] = checkSum[0];
        frame[7] = checkSum[1];
    }
};

//
class SenseAirSensor : public BaseSensor, SenseAir {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        SenseAirSensor() {
            _count = 1;
            _co2 = 0;
            _lastCo2 = 0;
            _serial = NULL;
            _sensor_id = SENSOR_SENSEAIR_ID;
        }

        ~SenseAirSensor() {
            if (_serial) delete _serial;
            _serial = NULL;
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
            _serial->enableRx(true);

            _startTime = 0;
            _ready = true;
            _dirty = false;
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[28];
            snprintf(buffer, sizeof(buffer), "SenseAir S8 @ SwSerial(%u,%u)", _pin_rx, _pin_tx);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return description();
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            char buffer[6];
            snprintf(buffer, sizeof(buffer), "%u:%u", _pin_rx, _pin_tx);
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            return MAGNITUDE_CO2;
        }

        void pre() {

            if (millis() - _startTime < 20000) {
                _error = SENSOR_ERROR_WARM_UP;
                return;
            }

            unsigned int co2 = readCo2();
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
        double value(unsigned char index) {
            return _co2;
        }

    protected:
        unsigned int _pin_rx;
        unsigned int _pin_tx;
        unsigned long _startTime;
        unsigned int _co2;
        unsigned int _lastCo2;
};


#endif // SENSOR_SUPPORT && SENSEAIR_SUPPORT
