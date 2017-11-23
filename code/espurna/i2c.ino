/*

I2C MODULE

Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if I2C_SUPPORT

#include "brzo_i2c.h"

bool i2cCheck(unsigned char address) {
    brzo_i2c_start_transaction(address, I2C_SCL_FREQUENCY);
    brzo_i2c_ACK_polling(1000);
    return brzo_i2c_end_transaction();
}

void i2cScan() {

    unsigned char nDevices = 0;

    for (unsigned char address = 1; address < 128; address++) {
        unsigned char response = i2cCheck(address);
        if (response == 0) {
            DEBUG_MSG_P(PSTR("[I2C] Device found at address 0x%02X\n"), address);
            nDevices++;
        } else if (response != 32) {
            //DEBUG_MSG_P(PSTR("[I2C] Unknown error at address 0x%02X\n"), address);
        }
    }

    if (nDevices == 0) DEBUG_MSG_P(PSTR("[I2C] No devices found\n"));

}

void i2cSetup() {
    brzo_i2c_setup(I2C_SDA_PIN, I2C_SCL_PIN, I2C_CLOCK_STRETCH_TIME);
    i2cScan();
}

#endif
