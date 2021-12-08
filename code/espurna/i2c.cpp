/*

I2C MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if I2C_SUPPORT

#include <Wire.h>

#if I2C_USE_BRZO
#include <brzo_i2c.h>
#endif

#include "i2c.h"

#include <cstring>

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

namespace {
namespace i2c {

struct Bus {
    unsigned char sda { GPIO_NONE };
    unsigned char scl { GPIO_NONE };
};

namespace internal {

Bus bus;
unsigned int locked[16] = {0};
#if I2C_USE_BRZO
unsigned long sclFrequency = 0;
#endif

} // namespace

#if I2C_USE_BRZO
void brzo_i2c_start_transaction(uint8_t address) {
    ::brzo_i2c_start_transaction(address, internal::sclFrequency);
}
#endif

namespace build {

constexpr unsigned char sda() {
    return I2C_SDA_PIN;
}

constexpr unsigned char scl() {
    return I2C_SCL_PIN;
}

#if I2C_USE_BRZO
constexpr unsigned long cst() {
    return I2C_CLOCK_STRETCH_TIME;
}

constexpr unsigned long sclFrequency() {
    return I2C_SCL_FREQUENCY;
}
#endif

} // namespace build

namespace settings {

unsigned char sda() {
    return getSetting("i2cSDA", build::sda());
}

unsigned char scl() {
    return getSetting("i2cSCL", build::scl());
}

#if I2C_USE_BRZO
unsigned long cst() {
    return getSetting("i2cCST", build::cst());
}

unsigned long sclFrequency() {
    return getSetting("i2cFreq", build::sclFrequency());
}
#endif

} // namespace settings

bool check(unsigned char address) {
#if I2C_USE_BRZO
    i2c::start_brzo_transaction(address);
    brzo_i2c_ACK_polling(1000);
    return brzo_i2c_end_transaction();
#else
    Wire.beginTransmission(address);
    return Wire.endTransmission();
#endif
}

template <typename Callback>
void scan(Callback&& callback) {
    constexpr unsigned char AddressMin { 1 };
    constexpr unsigned char AddressMax { 127 };

    for (unsigned char address = AddressMin; address < AddressMax; ++address) {
        if (i2c::check(address) == 0) {
            callback(address);
        }
    }
}

int clear(unsigned char sda, unsigned char scl) {
#if defined(TWCR) && defined(TWEN)
    // Disable the Atmel 2-Wire interface so we can control the SDA and SCL pins directly
    TWCR &= ~(_BV(TWEN));
#endif

    // Make SDA (data) and SCL (clock) pins inputs with pullup
    pinMode(sda, INPUT_PULLUP);
    pinMode(scl, INPUT_PULLUP);

    // Wait 2.5 secs. This is strictly only necessary on the first power
    // up of the DS3231 module to allow it to initialize properly,
    // but is also assists in reliable programming of FioV3 boards as it gives the
    // IDE a chance to start uploaded the program
    // before existing sketch confuses the IDE by sending Serial data.
    espurna::time::blockingDelay(
        espurna::duration::Milliseconds(2500));

    // If it is held low the device cannot become the I2C master
    // I2C bus error. Could not clear SCL clock line held low
    boolean scl_low = (digitalRead(scl) == LOW);
    if (scl_low) {
        return 1;
    }

    boolean sda_low = (digitalRead(sda) == LOW);
    int clockCount = 20; // > 2x9 clock

    // While SDA is low for at most 20 cycles
    while (sda_low && (clockCount > 0)) {

        clockCount--;

        // Note: I2C bus is open collector so do NOT drive SCL or SDA high
        pinMode(scl, INPUT);        // release SCL pullup so that when made output it will be LOW
        pinMode(scl, OUTPUT);       // then clock SCL Low
        delayMicroseconds(10);      // for >5uS
        pinMode(scl, INPUT);        // release SCL LOW
        pinMode(scl, INPUT_PULLUP); // turn on pullup resistors again
                                    // do not force high as slave may be holding it low for clock stretching

        delayMicroseconds(10);      // The >5uS is so that even the slowest I2C devices are handled

        //  loop waiting for SCL to become high only wait 2sec
        scl_low = (digitalRead(scl) == LOW);
        int counter = 20;
        while (scl_low && (counter > 0)) {
            counter--;
            espurna::time::blockingDelay(
                espurna::duration::Milliseconds(100));
            scl_low = (digitalRead(scl) == LOW);
        }

        // If still low after 2 sec error
        // I2C bus error. Could not clear. SCL clock line held low by slave clock stretch for >2sec
        if (scl_low) {
            return 2;
        }

        sda_low = (digitalRead(sda) == LOW); //   and check SDA input again and loop

    }

    // If still low
    // I2C bus error. Could not clear. SDA data line held low
    if (sda_low) {
        return 3;
    }

    // Pull SDA line low for "start" or "repeated start"
    pinMode(sda, INPUT);        // remove pullup
    pinMode(sda, OUTPUT);       // and then make it LOW i.e. send an I2C Start or Repeated start control

    // When there is only one I2C master a "start" or "repeat start" has the same function as a "stop" and clears the bus
    // A Repeat Start is a Start occurring after a Start with no intervening Stop.

    delayMicroseconds(10);      // wait >5uS
    pinMode(sda, INPUT);        // remove output low
    pinMode(sda, INPUT_PULLUP); // and make SDA high i.e. send I2C STOP control.

    delayMicroseconds(10);      // wait >5uS
    pinMode(sda, INPUT);        // and reset pins as tri-state inputs which is the default state on reset
    pinMode(scl, INPUT);

    // Everything OK
    return 0;
}

int clear(const Bus& bus) {
    return clear(bus.sda, bus.scl);
}

int clear() {
    return clear(internal::bus);
}

void init() {
    internal::bus.sda = settings::sda();
    internal::bus.scl = settings::scl();

    #if I2C_USE_BRZO
        internal::sclFrequency = settings::sclFrequency();
        brzo_i2c_setup(internal::bus.sda, internal::bus.scl, settings::cst());
    #else
        Wire.begin(internal::bus.sda, internal::bus.scl);
    #endif

    DEBUG_MSG_P(PSTR("[I2C] Initialized with sda:GPIO%hhu scl:GPIO%hhu\n"),
            internal::bus.sda, internal::bus.scl);

#if I2C_CLEAR_BUS
    clear(internal::bus);
#endif
}

#if TERMINAL_SUPPORT

void initTerminalCommands() {
    terminalRegisterCommand(F("I2C.SCAN"), [](::terminal::CommandContext&& ctx) {
        unsigned char devices { 0 };
        i2c::scan([&](unsigned char address) {
            ++devices;
            ctx.output.printf("found 0x%02X\n", address);
        });

        if (devices) {
            terminalOK(ctx);
            return;
        }

        terminalError(ctx, F("No devices found"));
    });

    terminalRegisterCommand(F("I2C.CLEAR"), [](::terminal::CommandContext&& ctx) {
        ctx.output.printf("result: %d\n", i2c::clear());
        terminalOK(ctx);
    });
}

#endif // TERMINAL_SUPPORT

} // namespace i2c
} // namespace

// ---------------------------------------------------------------------
// I2C API
// ---------------------------------------------------------------------

#if I2C_USE_BRZO

void i2c_wakeup(uint8_t address) {
    i2c::brzo_i2c_start_transaction(address);
    brzo_i2c_end_transaction();
}

uint8_t i2c_write_buffer(uint8_t address, uint8_t * buffer, size_t len) {
    i2c::brzo_i2c_start_transaction(address);
    brzo_i2c_write(buffer, len, false);
    return brzo_i2c_end_transaction();
}

uint8_t i2c_write_uint8(uint8_t address, uint8_t value) {
    uint8_t buffer[1] = {value};
    return i2c_write_buffer(address, buffer, sizeof(buffer));
}

uint8_t i2c_read_uint8(uint8_t address) {
    uint8_t buffer[1] = {0};
    i2c::brzo_i2c_start_transaction(address);
    brzo_i2c_read(buffer, 1, false);
    brzo_i2c_end_transaction();
    return buffer[0];
}

uint8_t i2c_read_uint8(uint8_t address, uint8_t reg) {
    uint8_t buffer[1] = {reg};
    i2c::brzo_i2c_start_transaction(address);
    brzo_i2c_write(buffer, 1, true);
    brzo_i2c_read(buffer, 1, false);
    brzo_i2c_end_transaction();
    return buffer[0];
}

uint16_t i2c_read_uint16(uint8_t address) {
    uint8_t buffer[2] = {0, 0};
    i2c::brzo_i2c_start_transaction(address);
    brzo_i2c_read(buffer, 2, false);
    brzo_i2c_end_transaction();
    return (buffer[0] * 256) | buffer[1];
}

uint16_t i2c_read_uint16(uint8_t address, uint8_t reg) {
    uint8_t buffer[2] = {reg, 0};
    i2c::brzo_i2c_start_transaction(address);
    brzo_i2c_write(buffer, 1, true);
    brzo_i2c_read(buffer, 2, false);
    brzo_i2c_end_transaction();
    return (buffer[0] * 256) | buffer[1];
}

void i2c_read_buffer(uint8_t address, uint8_t * buffer, size_t len) {
    i2c::start_brzo_transaction(address);
    brzo_i2c_read(buffer, len, false);
    brzo_i2c_end_transaction();
}

#else // not I2C_USE_BRZO

void i2c_wakeup(uint8_t address) {
    Wire.beginTransmission((uint8_t) address);
    Wire.endTransmission();
}

uint8_t i2c_write_uint8(uint8_t address, uint8_t value) {
    Wire.beginTransmission((uint8_t) address);
    Wire.write((uint8_t) value);
    return Wire.endTransmission();
}

uint8_t i2c_write_buffer(uint8_t address, uint8_t * buffer, size_t len) {
    Wire.beginTransmission((uint8_t) address);
    Wire.write(buffer, len);
    return Wire.endTransmission();
}

uint8_t i2c_read_uint8(uint8_t address) {
    uint8_t value;
    Wire.beginTransmission((uint8_t) address);
    Wire.requestFrom((uint8_t) address, (uint8_t) 1);
    value = Wire.read();
    Wire.endTransmission();
    return value;
}

uint8_t i2c_read_uint8(uint8_t address, uint8_t reg) {
    uint8_t value;
    Wire.beginTransmission((uint8_t) address);
    Wire.write((uint8_t) reg);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t) address, (uint8_t) 1);
    value = Wire.read();
    Wire.endTransmission();
    return value;
}

uint16_t i2c_read_uint16(uint8_t address) {
    uint16_t value;
    Wire.beginTransmission((uint8_t) address);
    Wire.requestFrom((uint8_t) address, (uint8_t) 2);
    value = (Wire.read() * 256) | Wire.read();
    Wire.endTransmission();
    return value;
}

uint16_t i2c_read_uint16(uint8_t address, uint8_t reg) {
    uint16_t value;
    Wire.beginTransmission((uint8_t) address);
    Wire.write((uint8_t) reg);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t) address, (uint8_t) 2);
    value = (Wire.read() * 256) | Wire.read();
    Wire.endTransmission();
    return value;
}

void i2c_read_buffer(uint8_t address, uint8_t * buffer, size_t len) {
    Wire.beginTransmission((uint8_t) address);
    Wire.requestFrom(address, (uint8_t) len);
    for (size_t i=0; i<len; i++) buffer[i] = Wire.read();
    Wire.endTransmission();
}

void i2c_write_uint(uint8_t address, uint16_t reg, uint32_t input, size_t size) {
    if (size && (size <= sizeof(input))) {
        Wire.beginTransmission(address);
        Wire.write((reg >> 8) & 0xff);
        Wire.write(reg & 0xff);

        uint8_t buf[sizeof(input)];
        std::memcpy(&buf[0], &input, sizeof(buf));

        Wire.write(&buf[sizeof(buf) - size], size);
        Wire.endTransmission();
    }
}

uint32_t i2c_read_uint(uint8_t address, uint16_t reg, size_t size, bool stop) {
    uint32_t out { 0 };
    if (size <= sizeof(out)) {
        Wire.beginTransmission(address);
        Wire.write((reg >> 8) & 0xff);
        Wire.write(reg & 0xff);
        Wire.endTransmission(stop);

        if (size == Wire.requestFrom(address, size)) {
            for (size_t byte = 0; byte < size; --byte) {
                out = (out << 8ul) | static_cast<uint8_t>(Wire.read());
            }
        }
    }

    return out;
}

#endif // I2C_USE_BRZO

uint8_t i2c_write_uint8(uint8_t address, uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};
    return i2c_write_buffer(address, buffer, 2);
}

uint8_t i2c_write_uint8(uint8_t address, uint8_t reg, uint8_t value1, uint8_t value2) {
    uint8_t buffer[3] = {reg, value1, value2};
    return i2c_write_buffer(address, buffer, 3);
}

uint8_t i2c_write_uint16(uint8_t address, uint8_t reg, uint16_t value) {
    uint8_t buffer[3];
    buffer[0] = reg;
    buffer[1] = (value >> 8) & 0xFF;
    buffer[2] = (value >> 0) & 0xFF;
    return i2c_write_buffer(address, buffer, 3);
}

uint8_t i2c_write_uint16(uint8_t address, uint16_t value) {
    uint8_t buffer[2];
    buffer[0] = (value >> 8) & 0xFF;
    buffer[1] = (value >> 0) & 0xFF;
    return i2c_write_buffer(address, buffer, 2);
}

uint16_t i2c_read_uint16_le(uint8_t address, uint8_t reg) {
    uint16_t temp = i2c_read_uint16(address, reg);
    return (temp / 256) | (temp * 256);
}

int16_t i2c_read_int16(uint8_t address, uint8_t reg) {
    return (int16_t) i2c_read_uint16(address, reg);
}

int16_t i2c_read_int16_le(uint8_t address, uint8_t reg) {
    return (int16_t) i2c_read_uint16_le(address, reg);
}

// -----------------------------------------------------------------------------
// Utils
// -----------------------------------------------------------------------------

int i2cClearBus() {
    return i2c::clear();
}

bool i2cGetLock(unsigned char address) {
    unsigned char index = address / 8;
    unsigned char mask = 1 << (address % 8);
    if (!(i2c::internal::locked[index] & mask)) {
        i2c::internal::locked[index] = i2c::internal::locked[index] | mask;
        DEBUG_MSG_P(PSTR("[I2C] Address 0x%02X locked\n"), address);
        return true;
    }
    return false;
}

bool i2cReleaseLock(unsigned char address) {
    unsigned char index = address / 8;
    unsigned char mask = 1 << (address % 8);
    if (i2c::internal::locked[index] & mask) {
        i2c::internal::locked[index] = i2c::internal::locked[index] & ~mask;
        return true;
    }
    return false;
}

unsigned char i2cFind(size_t size, unsigned char * addresses, unsigned char &start) {
    for (unsigned char i=start; i<size; i++) {
        if (i2c::check(addresses[i]) == 0) {
            start = i;
            return addresses[i];
        }
    }
    return 0;
}

unsigned char i2cFind(size_t size, unsigned char * addresses) {
    unsigned char start = 0;
    return i2cFind(size, addresses, start);
}

unsigned char i2cFindAndLock(size_t size, unsigned char * addresses) {
    unsigned char start = 0;
    unsigned char address = 0;
    while ((address = i2cFind(size, addresses, start))) {
        if (i2cGetLock(address)) break;
        start++;
    }
    return address;
}

void i2cSetup() {
    i2c::init();

#if TERMINAL_SUPPORT
    i2c::initTerminalCommands();
#endif

#if I2C_PERFORM_SCAN
    i2c::scan([](unsigned char address) {
        DEBUG_MSG_P(PSTR("[I2C] Found 0x02X\n"), address);
    });
#endif
}

#endif
