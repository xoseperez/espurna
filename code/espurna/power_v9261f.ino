/*

POWER V9261F MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if POWER_PROVIDER == POWER_PROVIDER_V9261F

// -----------------------------------------------------------------------------
// MODULE GLOBALS AND CACHE
// -----------------------------------------------------------------------------

#include <SoftwareSerial.h>
SoftwareSerial * _v9261f_uart;

double _v9261f_active = 0;
double _v9261f_reactive = 0;
double _v9261f_voltage = 0;
double _v9261f_current = 0;
unsigned char _v9261f_data[24];

// -----------------------------------------------------------------------------
// HAL
// -----------------------------------------------------------------------------

void _v9261fRead() {

    static unsigned char state = 0;
    static unsigned long last = 0;
    static bool found = false;
    static unsigned char index = 0;

    if (state == 0) {

        while (_v9261f_uart->available()) {
            _v9261f_uart->flush();
            found = true;
            last = millis();
        }

        if (found && (millis() - last > V9261F_SYNC_INTERVAL)) {
            _v9261f_uart->flush();
            index = 0;
            state = 1;
        }

    } else if (state == 1) {

        while (_v9261f_uart->available()) {
            _v9261f_uart->read();
            if (index++ >= 7) {
                _v9261f_uart->flush();
                index = 0;
                state = 2;
            }
        }

    } else if (state == 2) {

        while (_v9261f_uart->available()) {
            _v9261f_data[index] = _v9261f_uart->read();
            if (index++ >= 19) {
                _v9261f_uart->flush();
                last = millis();
                state = 3;
            }
        }

    } else if (state == 3) {

        if (_v9261fChecksum()) {

            _v9261f_active = (double) (
                (_v9261f_data[3]) +
                (_v9261f_data[4] << 8) +
                (_v9261f_data[5] << 16) +
                (_v9261f_data[6] << 24)
            ) / V9261F_POWER_FACTOR;

            _v9261f_reactive = (double) (
                (_v9261f_data[7]) +
                (_v9261f_data[8] <<  8) +
                (_v9261f_data[9] << 16) +
                (_v9261f_data[10] << 24)
            ) / V9261F_RPOWER_FACTOR;

            _v9261f_voltage = (double) (
                (_v9261f_data[11]) +
                (_v9261f_data[12] <<  8) +
                (_v9261f_data[13] << 16) +
                (_v9261f_data[14] << 24)
            ) / V9261F_VOLTAGE_FACTOR;

            _v9261f_current = (double) (
                (_v9261f_data[15]) +
                (_v9261f_data[16] <<  8) +
                (_v9261f_data[17] << 16) +
                (_v9261f_data[18] << 24)
            ) / V9261F_CURRENT_FACTOR;

            if (_v9261f_active < 0) _v9261f_active = 0;
            if (_v9261f_reactive < 0) _v9261f_reactive = 0;
            if (_v9261f_voltage < 0) _v9261f_voltage = 0;
            if (_v9261f_current < 0) _v9261f_current = 0;

            _power_newdata = true;

        }

        last = millis();
        index = 0;
        state = 4;

    } else if (state == 4) {

        while (_v9261f_uart->available()) {
            _v9261f_uart->flush();
            last = millis();
        }

        if (millis() - last > V9261F_SYNC_INTERVAL) {
            state = 1;
        }

    }

}

bool _v9261fChecksum() {
    unsigned char checksum = 0;
    for (unsigned char i = 0; i < 19; i++) {
        checksum = checksum + _v9261f_data[i];
    }
    checksum = ~checksum + 0x33;
    return checksum == _v9261f_data[19];
}

// -----------------------------------------------------------------------------
// POWER API
// -----------------------------------------------------------------------------

double _powerCurrent() {
    return _v9261f_current;
}

double _powerVoltage() {
    return _v9261f_voltage;
}

double _powerActivePower() {
    return _v9261f_active;
}

double _powerApparentPower() {
    return sqrt(_v9261f_reactive * _v9261f_reactive + _v9261f_active * _v9261f_active);
}

double _powerReactivePower() {
    return _v9261f_reactive;
}

double _powerPowerFactor() {
    double apparent = _powerApparentPower();
    if (apparent > 0) return _powerActivePower() / apparent;
    return 1;
}

void _powerEnabledProvider() {
    // Nothing to do
}

void _powerConfigureProvider() {
    // Nothing to do
}

void _powerSetupProvider() {
    _v9261f_uart = new SoftwareSerial(V9261F_PIN, SW_SERIAL_UNUSED_PIN, V9261F_PIN_INVERSE, 256);
    _v9261f_uart->begin(V9261F_BAUDRATE);
}

void _powerLoopProvider(bool before) {

    if (before) {
        _v9261fRead();
    }

}

#endif // POWER_PROVIDER & POWER_PROVIDER_EMON
