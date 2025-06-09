/*

I2C MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <cstddef>
#include <cstdint>

#include "compat.h"

namespace espurna {
namespace i2c {

CONSTEXPR_INLINE auto Ok = uint8_t{ 0 }; // ok
CONSTEXPR_INLINE auto Busy = uint8_t{ 4 }; // line is busy
CONSTEXPR_INLINE auto NackAddr = uint8_t{ 2 }; // received NACK on transmit of address
CONSTEXPR_INLINE auto NackData = uint8_t{ 3 }; // received NACK on transmit of data

} // namespace i2c
} // namespace espurna

uint8_t i2c_wakeup(uint8_t address);

uint8_t i2c_write_buffer(uint8_t address, const uint8_t* buffer, size_t len, bool stop);
uint8_t i2c_write_buffer(uint8_t address, const uint8_t* buffer, size_t len);

uint8_t i2c_write_buffer(uint8_t address, uint32_t reg, const uint8_t* buffer, size_t len, bool stop);
uint8_t i2c_write_buffer(uint8_t address, uint32_t reg, const uint8_t* buffer, size_t len);

uint8_t i2c_write_uint8(uint8_t address, uint8_t value);
uint8_t i2c_write_uint8(uint8_t address, uint32_t reg, uint8_t value);

uint8_t i2c_write_uint16(uint8_t address, uint16_t value);
uint8_t i2c_write_uint16(uint8_t address, uint32_t reg, uint16_t value);

uint8_t i2c_write_uint32(uint8_t address, uint32_t value);
uint8_t i2c_write_uint32(uint8_t address, uint32_t reg, uint32_t value);

uint8_t i2c_write_most(uint8_t address, uint32_t reg, uint32_t value, size_t len, bool stop);
uint8_t i2c_write_most(uint8_t address, uint32_t reg, uint32_t value, size_t len);

uint8_t i2c_read_buffer(uint8_t address, uint8_t* buffer, size_t len);
uint8_t i2c_read_buffer(uint8_t address, uint32_t reg, uint8_t* buffer, size_t len, bool stop);
uint8_t i2c_read_buffer(uint8_t address, uint32_t reg, uint8_t* buffer, size_t len);

uint8_t i2c_read_uint8(uint8_t address);
uint8_t i2c_read_uint8(uint8_t address, uint32_t reg, bool stop);
uint8_t i2c_read_uint8(uint8_t address, uint32_t reg);

uint16_t i2c_read_uint16(uint8_t address);
uint16_t i2c_read_uint16(uint8_t address, uint32_t reg, bool stop);
uint16_t i2c_read_uint16(uint8_t address, uint32_t reg);
uint16_t i2c_read_uint16_le(uint8_t address, uint32_t reg, bool stop);
uint16_t i2c_read_uint16_le(uint8_t address, uint32_t reg);

int16_t i2c_read_int16(uint8_t address);
int16_t i2c_read_int16(uint8_t address, uint32_t reg, bool stop);
int16_t i2c_read_int16(uint8_t address, uint32_t reg);
int16_t i2c_read_int16_le(uint8_t address, uint32_t reg, bool stop);
int16_t i2c_read_int16_le(uint8_t address, uint32_t reg);

uint32_t i2c_read_uint32(uint8_t address);
uint32_t i2c_read_uint32(uint8_t address, uint32_t reg, bool stop);
uint32_t i2c_read_uint32(uint8_t address, uint32_t reg);
uint32_t i2c_read_uint32_le(uint8_t address, uint32_t reg, bool stop);
uint32_t i2c_read_uint32_le(uint8_t address, uint32_t reg);

int32_t i2c_read_int32(uint8_t address);
int32_t i2c_read_int32(uint8_t address, uint32_t reg, bool stop);
int32_t i2c_read_int32_le(uint8_t address, uint32_t reg, bool stop);
int32_t i2c_read_int32_le(uint8_t address, uint32_t reg);

uint32_t i2c_read_most(uint8_t address, uint32_t reg, size_t len, bool stop);
uint32_t i2c_read_most(uint8_t address, uint32_t reg, size_t len);

uint8_t i2cFind(uint8_t);

bool i2cLock(uint8_t address);
void i2cUnlock(uint8_t address);

uint8_t i2cFind(const uint8_t* begin, const uint8_t* end);
uint8_t i2cFindAndLock(const uint8_t* begin, const uint8_t* end);

int i2cClearBus();
void i2cSetup();
