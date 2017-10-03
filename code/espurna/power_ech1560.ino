/*

POWER ECH1560 MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if POWER_PROVIDER == POWER_PROVIDER_ECH1560

// -----------------------------------------------------------------------------
// MODULE GLOBALS AND CACHE
// -----------------------------------------------------------------------------

volatile long _ech1560_bits_count = 0;
volatile long _ech1560_clk_count = 0;
volatile bool _ech1560_dosync = false;
volatile bool _ech1560_nextbit = true;

double _ech1560_apparent = 0;
double _ech1560_voltage = 0;
double _ech1560_current = 0;

// -----------------------------------------------------------------------------
// HAL
// -----------------------------------------------------------------------------

void _ech1560_sync() {

    unsigned int byte1 = 0;
    unsigned int byte2 = 0;
    unsigned int byte3 = 0;

    _ech1560_bits_count = 0;
    while (_ech1560_bits_count < 40); // skip the uninteresting 5 first bytes
    _ech1560_bits_count = 0;

    while (_ech1560_bits_count < 24) { // loop through the next 3 Bytes (6-8) and save byte 6 and 7 in Ba and Bb

        if (_ech1560_nextbit) {

            if (_ech1560_bits_count < 9) { // first Byte/8 bits in Ba

                byte1 = byte1 << 1;
                if (digitalRead(ECH1560_MISO_PIN) == HIGH) byte1 |= 1;
                _ech1560_nextbit = false;

            } else if (_ech1560_bits_count < 17) { // bit 9-16 is byte 7, stor in Bb

                byte2 = byte2 << 1;
                if (digitalRead(ECH1560_MISO_PIN) == HIGH) byte2 |= 1;
                _ech1560_nextbit = false;

            }

        }

    }

    if (byte2 != 3) { // if bit Bb is not 3, we have reached the important part, U is allready in Ba and Bb and next 8 Bytes will give us the Power.

        // voltage = 2 * (Ba + Bb / 255)
        _ech1560_voltage = 2.0 * ((float) byte1 + (float) byte2 / 255.0);

        // power:
        _ech1560_bits_count = 0;
        while (_ech1560_bits_count < 40); // skip the uninteresting 5 first bytes
        _ech1560_bits_count = 0;

        byte1 = 0;
        byte2 = 0;
        byte3 = 0;

        while (_ech1560_bits_count < 24) { //store byte 6, 7 and 8 in Ba and Bb & Bc.

            if (_ech1560_nextbit) {

                if (_ech1560_bits_count < 9) {

                    byte1 = byte1 << 1;
                    if (digitalRead(ECH1560_MISO_PIN) == HIGH) byte1 |= 1;
                    _ech1560_nextbit = false;

                } else if (_ech1560_bits_count < 17) {

                    byte2 = byte2 << 1;
                    if (digitalRead(ECH1560_MISO_PIN) == HIGH) byte2 |= 1;
                    _ech1560_nextbit = false;

                } else {

                    byte3 = byte3 << 1;
                    if (digitalRead(ECH1560_MISO_PIN) == HIGH) byte3 |= 1;
                    _ech1560_nextbit = false;

                }
            }
        }

        #if ECH1560_INVERTED
            byte1 = 255 - byte1;
            byte2 = 255 - byte2;
            byte3 = 255 - byte3;
        #endif

        // power = (Ba*255+Bb+Bc/255)/2
        _ech1560_apparent = ( (float) byte1 * 255 + (float) byte2 + (float) byte3 / 255.0) / 2;
        _ech1560_current = _ech1560_apparent / _ech1560_voltage;

        _power_newdata = true;
        _ech1560_dosync = false;

    }

    // If Bb is not 3 or something else than 0, something is wrong!
    if (byte2 == 0) _ech1560_dosync = false;

}

void ICACHE_RAM_ATTR _ech1560_isr() {

    // if we are trying to find the sync-time (CLK goes high for 1-2ms)
    if (_ech1560_dosync == false) {

        _ech1560_clk_count = 0;

        // register how long the ClkHigh is high to evaluate if we are at the part wher clk goes high for 1-2 ms
        while (digitalRead(ECH1560_CLK_PIN) == HIGH) {
            _ech1560_clk_count += 1;
            delayMicroseconds(30);  //can only use delayMicroseconds in an interrupt.
        }

        // if the Clk was high between 1 and 2 ms than, its a start of a SPI-transmission
        if (_ech1560_clk_count >= 33 && _ech1560_clk_count <= 67) {
            _ech1560_dosync = true;
        }

    // we are in sync and logging CLK-highs
    } else {

        // increment an integer to keep track of how many bits we have read.
        _ech1560_bits_count += 1;
        _ech1560_nextbit = true;

    }

}

// -----------------------------------------------------------------------------
// POWER API
// -----------------------------------------------------------------------------

double _powerCurrent() {
    return _ech1560_current;
}

double _powerVoltage() {
    return _ech1560_voltage;
}

double _powerActivePower() {
    return 0;
}

double _powerApparentPower() {
    return _ech1560_apparent;
}

double _powerReactivePower() {
    return 0;
}

double _powerPowerFactor() {
    return 1;
}

void _powerEnabledProvider() {
    // Nothing to do
}

void _powerConfigureProvider() {
    // Nothing to do
}

void _powerCalibrateProvider(unsigned char magnitude, double value) {
    // Nothing to do
}

void _powerResetCalibrationProvider() {
    // Nothing to do
}

void _powerSetupProvider() {
    pinMode(ECH1560_CLK_PIN, INPUT);
    pinMode(ECH1560_MISO_PIN, INPUT);
    attachInterrupt(ECH1560_CLK_PIN, _ech1560_isr, RISING);
 }

void _powerLoopProvider(bool before) {

    if (!before) {
        if (_ech1560_dosync) _ech1560_sync();
    }

}

#endif // POWER_PROVIDER == POWER_PROVIDER_ECH1560
