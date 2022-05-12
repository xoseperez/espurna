/*

I2C MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if I2C_SUPPORT

#if I2C_USE_BRZO
#include <brzo_i2c.h>
#else
#include <Wire.h>
#endif

#include "i2c.h"

#include <cstring>
#include <bitset>

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

namespace espurna {
namespace i2c {
namespace {

struct Bus {
    unsigned char sda { GPIO_NONE };
    unsigned char scl { GPIO_NONE };
#if I2C_USE_BRZO
    unsigned long frequency { 0 };
#endif
};

namespace internal {

Bus bus;

} // namespace internal

namespace lock {

std::bitset<128> storage{};

void reset(uint8_t address) {
    storage.reset(address);
}

bool get(uint8_t address) {
    return storage.test(address);
}

bool set(uint8_t address) {
    if (!get(address)) {
        storage.set(address);
        return true;
    }

    return false;
}

} // namespace lock

#if I2C_USE_BRZO
void brzo_i2c_start_transaction(uint8_t address) {
    ::brzo_i2c_start_transaction(address, internal::bus.frequency);
}
#endif

namespace build {

constexpr unsigned char sda() {
    return I2C_SDA_PIN;
}

constexpr unsigned char scl() {
    return I2C_SCL_PIN;
}

constexpr bool performScanOnBoot() {
    return I2C_PERFORM_SCAN == 1;
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

// make note that both APIs return integer status codes
// success is 0, everything else depends on the implementation
// for example, for our Wire it is:
// - 4 if line is busy
// - 2 if NACK happened when writing address
// - 3 if NACK happened when writing data
bool check(uint8_t address) {
#if I2C_USE_BRZO
    i2c::start_brzo_transaction(address);
    brzo_i2c_ACK_polling(1000);
    return 0 == brzo_i2c_end_transaction();
#else
    Wire.beginTransmission(address);
    return 0 == Wire.endTransmission();
#endif
}

template <typename Callback>
void scan(Callback&& callback) {
    static constexpr uint8_t AddressMin { 1 };
    static constexpr uint8_t AddressMax { 127 };

    for (auto address = AddressMin; address < AddressMax; ++address) {
        if (i2c::check(address)) {
            callback(address);
        }
    }
}

uint8_t check(const uint8_t* begin, const uint8_t* end) {
    for (const auto* it = begin; it != end; ++it) {
        if (check(*it)) {
            return *it;
        }
    }

    return 0;
}

void bootScan() {
    String addresses;
    scan([&](uint8_t address) {
        if (addresses.length()) {
            addresses += F(", ");
        }


        addresses += F("0x");
        addresses += hexEncode(address);
    });

    if (addresses.length()) {
        DEBUG_MSG_P(PSTR("[I2C] Found device(s): %s\n"), addresses.c_str());
    } else {
        DEBUG_MSG_P(PSTR("[I2C] No devices found\n"));
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
    bool scl_low = (digitalRead(scl) == LOW);
    if (scl_low) {
        return 1;
    }

    bool sda_low = (digitalRead(sda) == LOW);
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
    internal::bus.frequency = settings::sclFrequency();
    brzo_i2c_setup(internal::bus.sda, internal::bus.scl, settings::cst());
#else
    Wire.begin(internal::bus.sda, internal::bus.scl);
#endif

    DEBUG_MSG_P(PSTR("[I2C] Initialized SDA @ GPIO%hhu and SCL @ GPIO%hhu\n"),
            internal::bus.sda, internal::bus.scl);

#if I2C_CLEAR_BUS
    clear(internal::bus);
#endif
}

#if TERMINAL_SUPPORT

void initTerminalCommands() {
    terminalRegisterCommand(F("I2C.LOCKED"), [](::terminal::CommandContext&& ctx) {
        for (size_t address = 0; address < lock::storage.size(); ++address) {
            if (lock::storage.test(address)) {
                ctx.output.printf_P(PSTR("0x%02X\n"), address);
            }
        }

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("I2C.SCAN"), [](::terminal::CommandContext&& ctx) {
        size_t devices { 0 };
        i2c::scan([&](uint8_t address) {
            ++devices;
            ctx.output.printf_P(PSTR("0x%02X\n"), address);
        });

        if (devices) {
            ctx.output.printf_P(PSTR("found %zu device(s)\n"), devices);
            terminalOK(ctx);
            return;
        }

        terminalError(ctx, F("no devices found"));
    });

    terminalRegisterCommand(F("I2C.CLEAR"), [](::terminal::CommandContext&& ctx) {
        ctx.output.printf_P(PSTR("result %d\n"), i2c::clear());
        terminalOK(ctx);
    });
}

#endif // TERMINAL_SUPPORT

} // namespace
} // namespace i2c
} // namespace espurna

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
    Wire.requestFrom((uint8_t) address, (uint8_t) 1);
    value = Wire.read();
    return value;
}

uint8_t i2c_read_uint8(uint8_t address, uint8_t reg) {
    uint8_t value;
    Wire.beginTransmission((uint8_t) address);
    Wire.write((uint8_t) reg);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t) address, (uint8_t) 1);
    value = Wire.read();
    return value;
}

uint16_t i2c_read_uint16(uint8_t address) {
    uint16_t value;
    Wire.requestFrom((uint8_t) address, (uint8_t) 2);
    value = (Wire.read() * 256) | Wire.read();
    return value;
}

uint16_t i2c_read_uint16(uint8_t address, uint8_t reg) {
    uint16_t value;
    Wire.beginTransmission((uint8_t) address);
    Wire.write((uint8_t) reg);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t) address, (uint8_t) 2);
    value = (Wire.read() * 256) | Wire.read();
    return value;
}

void i2c_read_buffer(uint8_t address, uint8_t* buffer, size_t len) {
    Wire.requestFrom(address, (uint8_t) len);
    for (size_t i=0; i<len; ++i) {
        buffer[i] = Wire.read();
    }
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
    return espurna::i2c::clear();
}

bool i2cLock(uint8_t address) {
    return espurna::i2c::lock::set(address);
}

void i2cUnlock(uint8_t address) {
    espurna::i2c::lock::reset(address);
}

uint8_t i2cFind(uint8_t address) {
    if (espurna::i2c::check(address)) {
        return address;
    }

    return 0;
}

uint8_t i2cFind(const uint8_t* begin, const uint8_t* end) {
    const auto address = espurna::i2c::check(begin, end);
    if (address) {
        return address;
    }

    return 0;
}

uint8_t i2cFindAndLock(const uint8_t* begin, const uint8_t* end) {
    const auto address = i2cFind(begin, end);
    if (address && espurna::i2c::lock::set(address)) {
        return address;
    }

    return 0;
}

void i2cSetup() {
    espurna::i2c::init();

#if TERMINAL_SUPPORT
    espurna::i2c::initTerminalCommands();
#endif

    if (espurna::i2c::build::performScanOnBoot()) {
        espurna::i2c::bootScan();
    }
}

#endif
