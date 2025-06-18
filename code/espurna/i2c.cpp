/*

I2C MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if I2C_SUPPORT

#include <Wire.h>

#include "compat.h"
#include "i2c.h"

#include <array>
#include <cstring>
#include <bitset>

#if __cplusplus >= 201806L
#include <bit>
#endif

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

namespace espurna {
namespace i2c {
namespace {

struct Bus {
    unsigned char sda { GPIO_NONE };
    unsigned char scl { GPIO_NONE };
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

} // namespace build

namespace settings {

unsigned char sda() {
    return getSetting("i2cSDA", build::sda());
}

unsigned char scl() {
    return getSetting("i2cSCL", build::scl());
}

} // namespace settings

// make note that both APIs return integer status codes
// success is 0, everything else depends on the implementation

uint8_t transmission(uint8_t address, bool stop) {
    Wire.beginTransmission(address);
    return Wire.endTransmission(stop);
}

uint8_t transmission(uint8_t address) {
    return transmission(address, true);
}

template <typename T>
uint8_t with_transmission(uint8_t address, bool stop, T&& callback) {
    Wire.beginTransmission(address);
    callback();    
    return Wire.endTransmission(stop);
}

// device address when found
// 0 when not found

uint8_t find(uint8_t address) {
    if (Ok == transmission(address)) {
        return address;
    }

    return 0;
}

template <typename T>
uint8_t find(const uint8_t* begin, const uint8_t* end, T&& filter) {
    for (const auto* it = begin; it != end; ++it) {
        if (filter(*it) && (Ok == transmission(*it))) {
            return *it;
        }
    }

    return 0;
}

uint8_t find(const uint8_t* begin, const uint8_t* end) {
    return find(begin, end, [](uint8_t) {
        return true;
    });
}

uint8_t findAndLock(const uint8_t* begin, const uint8_t* end) {
    const auto address = find(begin, end, [](uint8_t address) {
        return !lock::get(address);
    });

    if (address != 0) {
        lock::set(address);
    }

    return address;
}

template <typename T>
void scan(T&& callback) {
    static constexpr uint8_t Min { 0x8 };
    static constexpr uint8_t Max { 0x78 };
    for (auto address = Min; address < Max; ++address) {
        if (Ok == transmission(address)) {
            callback(address);
        }
    }
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

    Wire.begin(internal::bus.sda, internal::bus.scl);
    DEBUG_MSG_P(PSTR("[I2C] Initialized SDA @ GPIO%hhu and SCL @ GPIO%hhu\n"),
            internal::bus.sda, internal::bus.scl);

#if I2C_CLEAR_BUS
    clear(internal::bus);
#endif
}

#if TERMINAL_SUPPORT
namespace terminal {

PROGMEM_STRING(Locked, "I2C.LOCKED");

void locked(::terminal::CommandContext&& ctx) {
    for (size_t address = 0; address < lock::storage.size(); ++address) {
        if (lock::storage.test(address)) {
            ctx.output.printf_P(PSTR("0x%02X\n"), address);
        }
    }

    terminalOK(ctx);
}

PROGMEM_STRING(Scan, "I2C.SCAN");

void scan(::terminal::CommandContext&& ctx) {
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
}

PROGMEM_STRING(Clear, "I2C.CLEAR");

void clear(::terminal::CommandContext&& ctx) {
    ctx.output.printf_P(PSTR("result %d\n"), i2c::clear());
    terminalOK(ctx);
}

PROGMEM_STRING(Read, "I2C.READ");

void read(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() < 2) {
        terminalError(ctx, STRING_VIEW("<size> <addr> [<reg>]\n"));
        return;
    }

    const auto convert_size = ::espurna::settings::internal::convert<size_t>;
    size_t size = convert_size(ctx.argv[1]);
    if (!size) {
        terminalError(ctx, STRING_VIEW("<size> == 0"));
        return;
    }

    const auto convert_addr = ::espurna::settings::internal::convert<uint8_t>;
    uint8_t addr = convert_addr(ctx.argv[2]);

    uint8_t result = Busy;

    std::vector<uint8_t> out;
    out.resize(size, 0);

    const auto convert_regaddr = ::espurna::settings::internal::convert<uint32_t>;
    if (ctx.argv.size() == 4) {
        const auto regaddr = convert_regaddr(ctx.argv[3]);
        ctx.output.printf_P("read(%02x,%u,%zu)\n", addr, regaddr, out.size());
        result = i2c_read_buffer(addr, regaddr, out.data(), out.size());
    } else {
        ctx.output.printf_P("read(%02x,%zu)\n", addr, out.size());
        result = i2c_read_buffer(addr, out.data(), out.size());
    }

    if (result != out.size()) {
        terminalError(ctx, STRING_VIEW("unknown error")); // i2c readFrom wrapper always returns 0
        return;
    }

    String message;
    message.reserve(out.size() * 2);

    for (auto& value : out) {
        message += hexEncode(value);
    }

    ctx.output.printf("%s\n", message.c_str());
    terminalOK(ctx);
}

static constexpr ::terminal::Command Commands[] PROGMEM {
    {Locked, locked},
    {Scan, scan},
    {Clear, clear},
    {Read, read},
};

void setup() {
    espurna::terminal::add(Commands);
}

} // namespace terminal
#endif // TERMINAL_SUPPORT

std::array<uint8_t, 2> pack_uint16(uint16_t value) {
    std::array<uint8_t, 2> out;

    out[0] = static_cast<uint8_t>((value >> 8) & 0xff);
    out[1] = static_cast<uint8_t>(value & 0xff);

    return out;
}

uint16_t unpack_uint16(const std::array<uint8_t, 2>& value) {
    uint16_t out;

    out = static_cast<uint16_t>(value[0]) << 8;
    out |= static_cast<uint16_t>(value[1]);

    return out;
}

std::array<uint8_t, 4> pack_uint32(uint32_t value) {
    std::array<uint8_t, 4> out;

    out[0] = static_cast<uint8_t>((value >> 24) & 0xff);
    out[1] = static_cast<uint8_t>((value >> 16) & 0xff);
    out[2] = static_cast<uint8_t>((value >> 8) & 0xff);
    out[3] = static_cast<uint8_t>(value & 0xff);

    return out;
}

uint32_t unpack_uint32(const std::array<uint8_t, 4>& value) {
    uint32_t out;

    out = static_cast<uint32_t>(value[0]) << 24;
    out |= static_cast<uint32_t>(value[1]) << 16;
    out |= static_cast<uint32_t>(value[2]) << 8;
    out |= static_cast<uint32_t>(value[3]);

    return out;
}

} // namespace
} // namespace i2c
} // namespace espurna

// ---------------------------------------------------------------------
// I2C API
// ---------------------------------------------------------------------

using espurna::i2c::transmission;
using espurna::i2c::with_transmission;

using espurna::i2c::pack_uint16;
using espurna::i2c::unpack_uint16;

using espurna::i2c::pack_uint32;
using espurna::i2c::unpack_uint32;

uint8_t i2c_wakeup(uint8_t address) {
    return transmission(address, true);
}

// api below split into two variants
// - ..._append_... - only issues Wire.write()
// - ..._write_... - starts with 'begin()' & ends with 'end()' of transmission

// attempt to write 1..4bytes from the value
static uint8_t i2c_append_least(uint32_t value) {
    uint8_t out{};

    const auto buf = pack_uint32(value);
    const auto begin = buf.cbegin();

    const auto end = buf.cend();
    const auto before_end = end - 1;

    for (auto it = begin; it != end; ++it) {
        if (*it || (it == before_end)) {
            out = Wire.write(it, std::distance(it, end));
            break;
        }
    }

    return out;
}

// attempt a transmission request of 1..4bytes of the given value
static uint8_t i2c_write_least(uint8_t address, uint32_t value, bool stop) {
    return with_transmission(address, stop,
        [&]() {
            i2c_append_least(value);
        });
}

static uint8_t i2c_append_buffer_impl(const uint8_t* buffer, size_t len) {
    return Wire.write(buffer, len);
}

uint8_t i2c_write_buffer(uint8_t address, const uint8_t* buffer, size_t len, bool stop) {
    return with_transmission(address, stop,
        [&]() {
            i2c_append_buffer_impl(buffer, len);
        });
}

uint8_t i2c_write_buffer(uint8_t address, const uint8_t* buffer, size_t len) {
    return i2c_write_buffer(address, buffer, len, true);
}

uint8_t i2c_write_buffer(uint8_t address, uint32_t reg, const uint8_t* buffer, size_t len, bool stop) {
    return with_transmission(address, stop,
        [&]() {
            i2c_append_least(reg);
            i2c_append_buffer_impl(buffer, len);
        });
}

uint8_t i2c_write_buffer(uint8_t address, uint32_t reg, const uint8_t* buffer, size_t len) {
    return i2c_write_buffer(address, reg, buffer, len, true);
}

static uint8_t i2c_append_uint8_impl(uint8_t value) {
    return Wire.write(value);
}

uint8_t i2c_write_uint8(uint8_t address, uint8_t value) {
    return with_transmission(address, true,
        [&]() {
            i2c_append_uint8_impl(value);
        });
}

uint8_t i2c_write_uint8(uint8_t address, uint32_t reg, uint8_t value) {
    return with_transmission(address, true,
        [&]() {
            i2c_append_least(reg);
            i2c_append_uint8_impl(value);
        });
}

static uint8_t i2c_append_uint16_impl(uint16_t value) {
    const auto prepared = pack_uint16(value);
    return Wire.write(prepared.data(), prepared.size());
}

uint8_t i2c_write_uint16(uint8_t address, uint16_t value) {
    return with_transmission(address, true,
        [&]() {
            i2c_append_uint16_impl(value);
        });
}

uint8_t i2c_write_uint16(uint8_t address, uint32_t reg, uint16_t value) {
    return with_transmission(address, true,
        [&]() {
            i2c_append_least(reg);
            i2c_append_uint16_impl(value);
        });
}

static uint8_t i2c_append_uint32_impl(uint32_t value) {
    const auto prepared = pack_uint32(value);
    return Wire.write(prepared.data(), prepared.size());
}

uint8_t i2c_write_uint32(uint8_t address, uint32_t value) {
    return with_transmission(address, true,
        [&]() {
            i2c_append_uint32_impl(value);
        });
}

uint8_t i2c_write_uint32(uint8_t address, uint32_t reg, uint32_t value) {
    return with_transmission(address, true,
        [&]() {
            i2c_append_least(reg);
            i2c_append_uint32_impl(value);
        });
}

uint8_t i2c_write_most(uint8_t address, uint32_t reg, uint32_t value, size_t len, bool stop) {
    const auto buf = pack_uint32(value);

    const auto most = std::clamp(len, size_t{ 1 }, buf.size());
    const auto* it = buf.data() + buf.size() - most;

    return i2c_write_buffer(address, reg, it, most, stop);
}

uint8_t i2c_write_most(uint8_t address, uint32_t reg, uint32_t value, size_t len) {
    return i2c_write_most(address, reg, value, len, true);
}

uint8_t i2c_read_buffer(uint8_t address, uint8_t* buffer, size_t len) {
    const auto out = Wire.requestFrom(address, static_cast<uint8_t>(len));
    for (size_t i = 0; i < out; ++i) {
        buffer[i] = Wire.read();
    }

    return out;
}

uint8_t i2c_read_buffer(uint8_t address, uint32_t reg, uint8_t* buffer, size_t len, bool stop) {
    i2c_write_least(address, reg, stop);
    return i2c_read_buffer(address, buffer, len);
}

uint8_t i2c_read_buffer(uint8_t address, uint32_t reg, uint8_t* buffer, size_t len) {
    return i2c_read_buffer(address, reg, buffer, len, true);
}

uint8_t i2c_read_uint8(uint8_t address) {
    uint8_t value[1]{};
    i2c_read_buffer(address, &value[0], sizeof(value));
    return value[0];
}

uint8_t i2c_read_uint8(uint8_t address, uint32_t reg, bool stop) {
    i2c_write_least(address, reg, stop);
    return i2c_read_uint8(address);
}

uint8_t i2c_read_uint8(uint8_t address, uint32_t reg) {
    return i2c_read_uint8(address, reg, true);
}

uint16_t i2c_read_uint16(uint8_t address) {
    std::array<uint8_t, 2> buf{};
    i2c_read_buffer(address, buf.data(), buf.size());

    return unpack_uint16(buf);
}

uint16_t i2c_read_uint16(uint8_t address, uint32_t reg, bool stop) {
    i2c_write_least(address, reg, stop);
    return i2c_read_uint16(address);
}

uint16_t i2c_read_uint16(uint8_t address, uint32_t reg) {
    return i2c_read_uint16(address, reg, true);
}

uint16_t i2c_read_uint16_le(uint8_t address, uint32_t reg, bool stop) {
    return __builtin_bswap16(i2c_read_uint16(address, reg, stop));
}

uint16_t i2c_read_uint16_le(uint8_t address, uint32_t reg) {
    return i2c_read_uint16_le(address, reg, true);
}

int16_t i2c_read_int16(uint8_t address) {
    return std::bit_cast<int16_t>(i2c_read_uint16(address));
}

int16_t i2c_read_int16(uint8_t address, uint32_t reg, bool stop) {
    return std::bit_cast<int16_t>(i2c_read_uint16(address, reg, stop));
}

int16_t i2c_read_int16(uint8_t address, uint32_t reg) {
    return i2c_read_int16(address, reg, true);
}

int16_t i2c_read_int16_le(uint8_t address, uint32_t reg, bool stop) {
    return std::bit_cast<int16_t>(i2c_read_uint16_le(address, reg, stop));
}

int16_t i2c_read_int16_le(uint8_t address, uint32_t reg) {
    return i2c_read_int16_le(address, reg, true);
}

uint32_t i2c_read_uint32(uint8_t address) {
    std::array<uint8_t, 4> buf{};
    i2c_read_buffer(address, buf.data(), buf.size());

    return unpack_uint32(buf);
}

uint32_t i2c_read_uint32(uint8_t address, uint32_t reg, bool stop) {
    i2c_write_least(address, reg, stop);
    return i2c_read_uint32(address);
}

uint32_t i2c_read_uint32(uint8_t address, uint32_t reg) {
    return i2c_read_uint32(address, reg, true);
}

uint32_t i2c_read_uint32_le(uint8_t address, uint32_t reg, bool stop) {
    return __builtin_bswap32(i2c_read_uint32(address, reg, stop));
}

uint32_t i2c_read_uint32_le(uint8_t address, uint32_t reg) {
    return i2c_read_uint32_le(address, reg, true);
}

int32_t i2c_read_int32(uint8_t address, uint32_t reg, bool stop) {
    return std::bit_cast<int32_t>(i2c_read_uint32(address, reg, stop));
}

int32_t i2c_read_int32(uint8_t address, uint32_t reg) {
    return i2c_read_int32(address, reg, true);
}

int32_t i2c_read_int32_le(uint8_t address, uint32_t reg, bool stop) {
    return std::bit_cast<int32_t>(i2c_read_uint32_le(address, reg, stop));
}

int32_t i2c_read_int32_le(uint8_t address, uint32_t reg) {
    return i2c_read_int32_le(address, reg, true);
}

uint32_t i2c_read_most(uint8_t address, uint32_t reg, size_t len, bool stop) {
    uint8_t buf[4]{};

    const auto most = std::clamp(len, size_t{ 1 }, sizeof(buf));
    i2c_read_buffer(address, reg, &buf[0], most, stop);

    uint32_t out{};
    for (size_t byte = 0; byte < most; ++byte) {
        out = (out << 8ul) | static_cast<uint32_t>(buf[byte]);
    }

    return out;
}

uint32_t i2c_read_most(uint8_t address, uint32_t reg, size_t len) {
    return i2c_read_most(address, reg, len, true);
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
    return espurna::i2c::find(address);
}

uint8_t i2cFind(const uint8_t* begin, const uint8_t* end) {
    return espurna::i2c::find(begin, end);
}

uint8_t i2cFindAndLock(const uint8_t* begin, const uint8_t* end) {
    return espurna::i2c::findAndLock(begin, end);
}

void i2cSetup() {
    espurna::i2c::init();

#if TERMINAL_SUPPORT
    espurna::i2c::terminal::setup();
#endif

    if (espurna::i2c::build::performScanOnBoot()) {
        espurna::i2c::bootScan();
    }
}

#endif
