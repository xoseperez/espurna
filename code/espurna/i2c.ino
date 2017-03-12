/*

I2C MODULE

Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_I2C

#include "brzo_i2c.h"

void i2cScan() {

    uint8_t address;
    uint8_t response;
    uint8_t buffer[1];
    int nDevices = 0;

    for (address = 1; address < 128; address++) {

        brzo_i2c_start_transaction(address, I2C_SCL_FREQUENCY);
        brzo_i2c_ACK_polling(1000);
        response = brzo_i2c_end_transaction();

        if (response == 0) {
            DEBUG_MSG_P(PSTR("[I2C] Device found at address 0x%02X\n"), address);
            nDevices++;
        } else if (response != 32) {
            //DEBUG_MSG_P(PSTR("[I2C] Unknown error at address 0x%02X\n"), address);
        }
    }

    if (nDevices == 0) DEBUG_MSG_P(PSTR("[I2C] No devices found"));

}

void i2cSetup() {
    brzo_i2c_setup(I2C_SDA_PIN, I2C_SCL_PIN, I2C_CLOCK_STRETCH_TIME);
    i2cScan();
}

#endif
