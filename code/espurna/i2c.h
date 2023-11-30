/*

I2C MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <cstddef>
#include <cstdint>

void i2c_wakeup(uint8_t address);
uint8_t i2c_write_buffer(uint8_t address, uint8_t * buffer, size_t len);
uint8_t i2c_write_uint8(uint8_t address, uint8_t value);
uint8_t i2c_write_uint8(uint8_t address, uint8_t reg, uint8_t value);
uint8_t i2c_write_uint8(uint8_t address, uint8_t reg, uint8_t value1, uint8_t value2);
uint8_t i2c_write_uint16(uint8_t address, uint16_t value);
uint8_t i2c_write_uint16(uint8_t address, uint8_t reg, uint16_t value);
uint8_t i2c_read_uint8(uint8_t address);
uint8_t i2c_read_uint8(uint8_t address, uint8_t reg);
uint16_t i2c_read_uint16(uint8_t address);
uint16_t i2c_read_uint16(uint8_t address, uint8_t reg);
uint16_t i2c_read_uint16_le(uint8_t address, uint8_t reg);
int16_t i2c_read_int16(uint8_t address, uint8_t reg);
int16_t i2c_read_int16_le(uint8_t address, uint8_t reg);

void i2c_read_buffer(uint8_t address, uint8_t* buffer, size_t len);
uint32_t i2c_read_uint(uint8_t address, uint16_t reg, size_t len, bool stop);
void i2c_write_uint(uint8_t address, uint16_t reg, uint32_t input, size_t len);

uint8_t i2cFind(uint8_t);

bool i2cLock(uint8_t address);
void i2cUnlock(uint8_t address);

uint8_t i2cFind(const uint8_t* begin, const uint8_t* end);
uint8_t i2cFindAndLock(const uint8_t* begin, const uint8_t* end);

int i2cClearBus();
void i2cSetup();
