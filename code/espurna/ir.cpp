/*
IR MODULE

Copyright (C) 2018 by Alexander Kolesnikov (raw and MQTT implementation)
Copyright (C) 2017-2019 by François Déchery
Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

For the library, see:
https://github.com/crankyoldgit/IRremoteESP8266

To (re)create the string -> Payload decoder .inc files, add `re2c` to the $PATH and 'run' the environment:
```
$ pio run -e ... \
    -t espurna/ir_parse_simple.re.cpp.inc \
    -t espurna/ir_parse_raw.re.cpp.inc
```
(see scripts/pio_pre.py and scripts/espurna_utils/build.py for more info)

*/

#include "espurna.h"

#if IR_SUPPORT

#include "ir.h"
#include "mqtt.h"
#include "relay.h"
#include "terminal.h"

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>

#include <cstdint>
#include <cstring>
#include <queue>
#include <vector>

// TODO: note that IRremoteEsp8266 includes a *lot* of built-in protocols. the suggested way to build the library
// is to append `-D_IR_ENABLE_DEFAULT_=false` to the build flags and specify the `-DSEND_...` and `-DDECODE_...` *only* for the required one(s)
//
// TODO: current IRremoteEsp8266 injects a bunch of stuff into the global scope
// - `__STDC_LIMIT_MACROS`, forcing some c99 macros related to integer limits
// - `enum decode_type_t` with `UNKNOWN` and `UNUSED` symbols in it
// - various `const T k*` defined in the headers (...that are replacing preprocessor tokens :/)
// - various `ENABLE_...`, `SEND_...`, `DECODE_...`, and etc. preprocessor names
//   (like `SEND_RAW` and `DECODE_HASH` for internal settings, or `DPRINT` when `DEBUG` is defined)
// ref. IRremoteESP8266.h, IRrecv.h and IRsend.h
//
// One solution is to patch upstream to have an optional `namespace irremoteesp8266 { ... }` wrapping everything related to the lib through a build flag, possibly versioned as well
// And, getting rid of accidentally exported C stuff in favour of C++ alternatives.

namespace ir {
namespace {

// TODO: some internal-only code instead of std::string_view and std::optional
//       currently, b/c of the -std=c++11. and might behave slightly different
//       from the stl alternatives. (like string literals with '\0' embedded,
//       and an obvious lack of constexpr methods)
//       may be aliased via `using` and depend on the __cplusplus?

struct StringView {
    StringView() = delete;
    ~StringView() = default;

    StringView(const StringView&) = default;
    StringView& operator=(const StringView&) = default;

    StringView(StringView&&) = default;
    StringView& operator=(StringView&&) = default;

    StringView(const char* begin, size_t length) :
        _begin(begin),
        _length(length)
    {}

    StringView(const char* begin, const char* end) :
        StringView(begin, end - begin)
    {}

    template <size_t Size>
    StringView(const char (&string)[Size]) :
        StringView(&string[0], Size - 1)
    {}

    StringView& operator=(const String& string) {
        _begin = string.c_str();
        _length = string.length();
        return *this;
    }

    explicit StringView(const String& string) :
        StringView(string.c_str(), string.length())
    {}

    template <size_t Size>
    StringView& operator=(const char (&string)[Size]) {
        _begin = &string[0];
        _length = Size - 1;
        return *this;
    }

    const char* begin() const {
        return _begin;
    }

    const char* end() const {
        return _begin + _length;
    }

    size_t length() const {
        return _length;
    }

    explicit operator bool() const {
        return _begin && _length;
    }

    explicit operator String() const {
        String out;
        out.concat(_begin, _length);
        return out;
    }

private:
    const char* _begin;
    size_t _length;
};

template <typename T>
struct ParseResult {
    ParseResult() = default;
    ParseResult(ParseResult&& other) noexcept {
        if (other._initialized) {
            init(std::move(other._result._value));
        }
    }

    explicit ParseResult(T&& value) noexcept {
        init(std::move(value));
    }

    ~ParseResult() {
        if (_initialized) {
            _result._value.~T();
        }
    }

    ParseResult(const ParseResult&) = delete;
    ParseResult& operator=(const T& value) = delete;

    ParseResult& operator=(T&& value) noexcept {
        init(std::move(value));
        return *this;
    }

    explicit operator bool() const {
        return _initialized;
    }

    bool ok() const {
        return _initialized;
    }

    const T& get() const {
        return _result._value;
    }

    T move() && {
        auto out = std::move(_result._value);
        return out;
    }

private:
    struct Empty {
    };

    union Result {
        Result() :
            _empty()
        {}

        ~Result() {
        }

        Result(const Result&) = delete;
        Result(Result&&) = delete;

        Result& operator=(const Result&) = delete;
        Result& operator=(Result&&) = delete;

        template <typename... Args>
        Result(Args&&... args) :
            _value(std::forward<Args>(args)...)
        {}

        template <typename... Args>
        void update(Args&&... args) {
            _value = T(std::forward<Args>(args)...);
        }

        Empty _empty;
        T _value;
    };

    void reset() {
        if (_initialized) {
            _result._value.~T();
        }
    }

    template <typename... Args>
    void init(Args&&... args) {
        if (!_initialized) {
            ::new (&_result) Result(std::forward<Args>(args)...);
            _initialized = true;
        } else {
            _result.update(std::forward<Args>(args)...);
        }
    }

    bool _initialized { false };
    Result _result;
};

// TODO: std::from_chars / re2c parser to convert numbers

template <typename T>
T sized(const StringView& view) {
    String value(view);

    char* endp { nullptr };
    unsigned long result { std::strtoul(value.c_str(), &endp, 10) };
    if ((endp != value.c_str()) && (*endp == '\0')) {
        constexpr unsigned long Boundary { 1ul << (sizeof(T) * 8) };
        if (result < Boundary) {
            return result;
        }
    }

    return 0;
}

template <>
unsigned long sized(const StringView& view) {
    String value(view);

    char* endp { nullptr };
    unsigned long result { std::strtoul(value.c_str(), &endp, 10) };
    if ((endp != value.c_str()) && (*endp == '\0')) {
        return result;
    }

    return 0;
}

namespace build {

// gpio settings create two different objects
// one for transmitting
constexpr unsigned char tx() {
    return IR_TX_PIN;
}

// and one for receiving
constexpr unsigned char rx() {
    return IR_TX_PIN;
}

// internally, lib uses an u16[] of this size
constexpr uint16_t rxBufferSize() {
    return IR_RX_BUFFER_SIZE;
}

// to be isr-friendly, will allocate second u16[]
// that will be used as a storage when decode()'ing
constexpr bool rxBufferSave() {
    return true;
}

// (optional) number of times that the message will be scheduled in the TX queue
// (i.e. when the [:<series>] is omitted from the MQTT payload)
constexpr uint8_t txSeries() {
    return IR_TX_SERIES;
}

// (ms)
constexpr unsigned txDelay() {
    return IR_TX_DELAY;
}

// (ms)
constexpr uint8_t rxTimeout() {
    return IR_RX_TIMEOUT;
}

// (ms) minimal time in-between decode() calls
constexpr unsigned long rxDelay() {
    return IR_RX_DELAY;
}

} // namespace build

namespace settings {

unsigned char tx() {
    return getSetting("irTx", build::tx());
}

unsigned char rx() {
    return getSetting("irRx", build::rx());
}

} // namespace settings

// Simple messages that transmit the numeric 'value' (up to 8 bytes)
//
// Transmitting:
//   Payload: <protocol>:<value>:<bits>[:<repeats>][:<delay>][:<times>]
//
//   Required parameters:
//     PROTOCOL - decimal ID, will be converted into a named 'decode_type_t'
//                (ref. IRremoteESP8266.h and it's protocol descriptions)
//     VALUE    - hexadecimal representation of the value that will be sent
//                (big endian, maximum 8bytes / 64bit. byte is always zero-padded)
//     BITS     - number of bits associated with the protocol
//                (ref. IRremoteESP8266.h and it's protocol descriptions)
//
//   Optional payload parameters:
//     REPEATS  - how many times the message will be sent immediatly
//                (defaults to 0 or the value set by the PROTOCOL type)
//     SERIES   - how many times the message will be scheduled for sending
//                (defaults to 1 aka once, [1...120))
//     DELAY    - minimum amount of time (ms) between queued messages
//                (defaults is IR_TX_DELAY, applies to every message in the series)
//
// Receiving:
//   Payload: 2:AABBCCDD:32 (<protocol>:<value>:<bits>)

// TODO: type as text both ways? see
// - IRutils.h::strToDecodeType(const char*);
// - IRutils.h::typeToString(decode_type_t);
// (i.e. additional path for [0-9A-Za-z]?)

namespace simple {

struct Payload {
    decode_type_t type { decode_type_t::UNKNOWN };
    uint64_t value { 0 };
    uint16_t bits { 0 };
    uint16_t repeats { 0 };
    uint8_t series { build::txSeries() };
    unsigned long delay { build::txDelay() };
};

namespace value {

// TODO: endianness of input is always 'big', output is 'little'
//       all esp platforms and common build hosts are 'little'
//       but, actually make sure bswap is necessary?

// To convert from an existing decimal value, there is a python one-liner:
// >>> bytes(x for x in (123456789).to_bytes(8, 'big', signed=False) if x).hex()
// '075bcd15'
// (and also notice that old version *always* cast `u64` into `u32` which cut off part of the code)

uint64_t decode(const StringView& view) {
    constexpr size_t RawSize { sizeof(uint64_t) };
    constexpr size_t BufferSize { (RawSize * 2) + 1 };

    if (!(view.length() % 2) && (view.length() < BufferSize)) {
        char buffer[BufferSize] {0};

        constexpr size_t BufferLen { BufferSize - 1 };
        char* ZerolessOffset { std::begin(buffer) + BufferLen - view.length() };
        std::fill(std::begin(buffer), ZerolessOffset, '0');
        std::copy(view.begin(), view.end(), ZerolessOffset);

        uint8_t raw[RawSize] {0};
        if (::hexDecode(buffer, BufferLen, raw, sizeof(raw))) {
            uint64_t output{0};
            std::memcpy(&output, &raw, sizeof(raw));
            return __builtin_bswap64(output);
        }
    }

    return 0;
}

String encode(uint64_t input) {
    String out;

    if (input) {
        const uint64_t Value { __builtin_bswap64(input) };

        uint8_t raw[sizeof(Value)] {0};
        std::memcpy(&raw, &Value, sizeof(raw));

        uint8_t* begin { std::begin(raw) };
        while (!(*begin)) {
            ++begin;
        }

        out = hexEncode(begin, std::end(raw));
    } else {
        out.concat(F("00"));
    }

    return out;
}

} // namespace value

namespace payload {

decode_type_t type(const StringView& view) {
    static_assert(std::is_same<int, std::underlying_type<decode_type_t>::type>::value, "");
    constexpr int First { -1 };
    constexpr int Last { static_cast<int>(decode_type_t::kLastDecodeType) };

    String value(view);

    int result { value.toInt() };
    if ((First < result) && (result < Last)) {
        return static_cast<decode_type_t>(result);
    }

    return decode_type_t::UNKNOWN;
}

uint64_t value(const StringView& view) {
    return ::ir::simple::value::decode(view);
}

uint16_t bits(const StringView& value) {
    return sized<uint16_t>(value);
}

uint16_t repeats(const StringView& value) {
    return sized<uint16_t>(value);
}

uint8_t series(const StringView& value) {
    return sized<uint8_t>(value);
}

unsigned long delay(const StringView& value) {
    return sized<unsigned long>(value);
}

} // namespace payload

#include "ir_parse_simple.re.cpp.inc"

} // namespace simple

// Transmitting:
//   Payload: <frequency>:<series>:<delay>:<μs>,<μs>,<μs>,<μs>,...
//            |         Options          | |       Message       |
//
//   FREQUENCY  - modulation frequency, either in kHz (<1000) or Hz (>=1000)
//   SERIES     - how many times the message will be scheduled for sending
//                [1...120)
//   DELAY      - minimum amount of time (ms) between queued messages
//
// Receiving:
//   Payload: <μs>,<μs>,<μs>,<μs>,...
//
// The message is encoded as time in microseconds for the IR LED to be in a certain state.
// First one is always ON, and the second one - OFF.

namespace raw {

constexpr uint16_t DefaultFrequency { 38 }; // kHz

struct Payload {
    uint16_t frequency { DefaultFrequency };
    uint8_t series { build::txSeries() };
    unsigned long delay { build::txDelay() };
    std::vector<uint16_t> time;
};

namespace time {

// TODO: compress / decompress with https://tasmota.github.io/docs/IRSend-RAW-Encoding/?
//
// Each rawbuf TIME value is:
// - multiplied by the TICK (old RAWTICK, currently kRawTick in a *global scope*)
// - rounded to the closest multiple of 5 (e.g. 299 becomes 300)
// - assigned an english alphabet letter ID (...or not, when exhausted all of 26 letters)
// Resulting payload contains TIME(μs) alternating between ON and OFF, starting with ON:
// - when first seen, output time directly prefixed with either '+' (ON) or '-' (OFF)
// - on further appearences, replace the time value with a letter that is uppercase for ON and lowercase for OFF
//
// For example, current implementation:
// > 100,200,100,200,200,300,300,300
//   |A| |B| |A| |B| |B| |C| |C| |C|
// Becomes:
// > +100-200AbB-300Cc
//    |A| |B|    |C|

String encode(const uint16_t* begin, const uint16_t* end) {
    static_assert((kRawTick == 2), "");

    String out;
    out.reserve((end - begin) * 5);

    for (const uint16_t* it = begin; it != end; ++it) {
        if (out.length()) {
            out += ',';
        }
        out += String((*it) * kRawTick, 10);
    }

    return out;
}

} // namespace time

namespace payload {

uint16_t frequency(const StringView& value) {
    return sized<uint16_t>(value);
}

uint8_t series(const StringView& value) {
    return sized<uint8_t>(value);
}

unsigned long delay(const StringView& value) {
    return sized<unsigned long>(value);
}

uint16_t time(const StringView& value) {
    return sized<uint16_t>(value);
}

} // namespace payload

#include "ir_parse_raw.re.cpp.inc"

} // namespace raw

namespace rx {

#if not IR_RX_SUPPORT

struct NoopReceiver {
    NoopReceiver(uint16_t, uint16_t, uint8_t, bool) {
    }

    bool decode(decode_results*) const {
        return false;
    }

    void disableIRIn() const {
    }

    void enableIRIn() const {
    }
};

#define IRrecv NoopReceiver
#endif

namespace internal {

BasePinPtr pin;
std::unique_ptr<IRrecv> instance;

} // namespace internal

struct Lock {
    Lock(const Lock&) = delete;
    Lock(Lock&&) = delete;

    Lock& operator=(const Lock&) = delete;
    Lock& operator=(Lock&&) = delete;

    Lock() {
        if (internal::instance) {
            internal::instance->disableIRIn();
        }
    }

    ~Lock() {
        if (internal::instance) {
            internal::instance->enableIRIn();
        }
    }
};

void setup(BasePinPtr&& pin) {
    internal::pin = std::move(pin);
    internal::instance = std::make_unique<IRrecv>(
            internal::pin->pin(),
            build::rxBufferSize(),
            build::rxTimeout(),
            build::rxBufferSave());
    internal::instance->enableIRIn();
}

// TODO: complex protocols used by HVAC *could* be handled by the IRacUtils (ref. IRac.h)

// Current implementation relies on the HEX-encoded 'value' (previously, decimal)
//
// XXX: when protocol is UNKNOWN, `value` is silently replaced with a fnv1 32bit hash
//      can be disabled with `-DDECODE_HASH=0` in the build flags
//
// XXX: library utilizes union as a way to store the data, making this an interesting case
//      of two-way aliasing inside of the struct. (and sometimes unexpected size requirements)
//
// At the time of writing, it is:
// > union {
// >   struct {
// >     uint64_t value;    // Decoded value
// >     uint32_t address;  // Decoded device address.
// >     uint32_t command;  // Decoded command.
// >   };
// >   uint8_t state[kStateSizeMax];  // Multi-byte results.
// > };
//
// From our side, trying to use `state` would mean:
// - always check `bits` in case decoder replaced it with a shorter value
//   default state size is 53, based on the largest payload supported by the lib
// - no `-DDECODE_AC=0` is in the build flags, as it will become u8[0].
//   *is* a case of undefined behaviour, and will also trigger both `-Wpedantic` and `-Warray-bounds`

// Also see:
// - `String resultToHumanReadableBasic(decode_results*);` for type + value as a single line
// - `String resultToTimingInfo(decode_results*)` for all of timing info, with a nice table output
// Not applicable here, though

struct DecodeResult {
    DecodeResult() = delete;
    explicit DecodeResult(::decode_results& result) :
        _result(result)
    {}

    decode_type_t type() const {
        return _result.decode_type;
    }

    uint16_t bits() const {
        return _result.bits;
    }

    const String& asSimplePayload() {
        if (!_payload.length()) {
            _payload.reserve(28);

            _payload += static_cast<int>(_result.decode_type);
            _payload += ':';

            _payload += asValue();
            _payload += ':';

            _payload += static_cast<unsigned int>(_result.bits);
        }

        return _payload;
    }

    const String& asValue() {
        static_assert(std::is_same<decltype(decode_results::value), uint64_t>::value, "");

        if (!_value.length()) {
            _value = ::ir::simple::value::encode(_result.value);
        }

        return _value;
    }

    const String& asRawTime() {
        if (!_time.length() && (_result.rawlen > 1)) {
            _time = ::ir::raw::time::encode(
                const_cast<const uint16_t*>(&_result.rawbuf[1]),
                const_cast<const uint16_t*>(&_result.rawbuf[_result.rawlen]));
        }

        return _time;
    }

private:
    const ::decode_results& _result;
    String _payload;
    String _value;
    String _time;
};

} // namespace rx

namespace tx {

#if not IR_TX_SUPPORT

struct NoopSender {
    NoopSender(uint16_t) {
    }

    void begin() {
    }

    bool send(decode_type_t, uint64_t, uint16_t, uint16_t) {
        return false;
    }

    void sendRaw(const uint16_t*, uint16_t, uint16_t) {
    }
};

#define IRsend NoopSender
#endif

// TODO: variant instead of virtuals?

struct PayloadSenderBase {
    PayloadSenderBase() = default;
    virtual ~PayloadSenderBase() = default;

    PayloadSenderBase(const PayloadSenderBase&) = delete;
    PayloadSenderBase& operator=(const PayloadSenderBase&) = delete;

    PayloadSenderBase(PayloadSenderBase&&) = delete;
    PayloadSenderBase& operator=(PayloadSenderBase&&) = delete;

    virtual unsigned long delay() const = 0;
    virtual bool send(IRsend& sender) const = 0;
    virtual bool reschedule() = 0;
};

struct SimplePayloadSender : public PayloadSenderBase {
    SimplePayloadSender() = delete;
    explicit SimplePayloadSender(ir::simple::Payload&& payload) :
        _payload(std::move(payload)),
        _series(_payload.series)
    {}

    bool send(IRsend& sender) const override {
        return _series && sender.send(_payload.type, _payload.value, _payload.bits, _payload.repeats);
    }

    bool reschedule() override {
        return _series && (--_series);
    }

    unsigned long delay() const override {
        return _payload.delay;
    }

private:
    ir::simple::Payload _payload;
    size_t _series;
};

struct RawPayloadSender : public PayloadSenderBase {
    RawPayloadSender() = delete;
    explicit RawPayloadSender(ir::raw::Payload&& payload) :
        _payload(std::move(payload)),
        _series(_payload.time.size() ? _payload.series : 0)
    {}

    bool reschedule() override {
        return _series && (--_series);
    }

    bool send(IRsend& sender) const override {
        if (_series) {
            sender.sendRaw(_payload.time.data(), _payload.time.size(), _payload.frequency);
            return true;
        }

        return false;
    }

    unsigned long delay() const override {
        return _payload.delay;
    }

private:
    ir::raw::Payload _payload;
    size_t _series;
};

using PayloadSenderPtr = std::unique_ptr<PayloadSenderBase>;

namespace internal {

BasePinPtr pin;
std::unique_ptr<IRsend> instance;

std::queue<PayloadSenderPtr> queue;

} // namespace internal

void enqueue(ir::simple::Payload&& payload) {
    internal::queue.push(std::make_unique<SimplePayloadSender>(std::move(payload)));
}

void enqueue(ir::ParseResult<ir::simple::Payload>&& result) {
    if (result) {
        enqueue(std::move(result).move());
    }
}

void enqueue(ir::raw::Payload&& payload) {
    internal::queue.push(std::make_unique<RawPayloadSender>(std::move(payload)));
}

void enqueue(ir::ParseResult<ir::raw::Payload>&& result) {
    if (result) {
        enqueue(std::move(result).move());
    }
}

void loop() {
    if (internal::queue.empty()) {
        return;
    }

    auto& payload = internal::queue.front();
    static unsigned long last { millis() - payload->delay() - 1ul };

    const unsigned long timestamp { millis() };
    if (timestamp - last < payload->delay()) {
        return;
    }

    last = timestamp;

    rx::Lock lock;
    if (!payload->send(*internal::instance)) {
        internal::queue.pop();
        return;
    }

    if (!payload->reschedule()) {
        internal::queue.pop();
    }
}

void setup(BasePinPtr&& pin) {
    internal::pin = std::move(pin);
    internal::instance = std::make_unique<IRsend>(internal::pin->pin());
    internal::instance->begin();
}

} // namespace tx

#if MQTT_SUPPORT
namespace mqtt {
namespace build {

// (optional) enables MQTT rx output
constexpr bool rx() {
    return IR_RX_MQTT == 1;
}

// (optional) enables MQTT RAW rx output (aka raw payload's rawbuf TIME * TICK)
constexpr bool rxRaw() {
    return IR_RX_RAW_MQTT == 1;
}

const char* topicRx() {
    return IR_RX_MQTT_TOPIC;
}

const char* topicTx() {
    return IR_TX_MQTT_TOPIC;
}

const char* topicRxRaw() {
    return IR_RX_RAW_MQTT_TOPIC;
}

const char* topicTxRaw() {
    return IR_TX_RAW_MQTT_TOPIC;
}

} // namespace build

namespace settings {

bool rx() {
    return getSetting("irRxMqtt", build::rx());
}

bool rxRaw() {
    return getSetting("irRxMqttRaw", build::rxRaw());
}

} // namespace settings

namespace internal {

bool rx { build::rx() };
bool rxRaw { build::rxRaw() };

void callback(unsigned int type, const char* topic, char* payload) {
    switch (type) {

    case MQTT_CONNECT_EVENT:
        mqttSubscribe(build::topicTx());
        mqttSubscribe(build::topicTxRaw());
        break;

    case MQTT_MESSAGE_EVENT: {
        StringView view{payload, payload + strlen(payload)};

        String t = mqttMagnitude(topic);
        if (t.equals(build::topicTx())) {
            ir::tx::enqueue(ir::simple::parse(view));
        } else if (t.equals(build::topicTxRaw())) {
            ir::tx::enqueue(ir::raw::parse(view));
        }

        break;
    }

    }
}

} // namespace internal

void process(rx::DecodeResult& result) {
    if (internal::rx) {
        ::mqttSend(build::topicRx(), result.asSimplePayload().c_str());
    }

    if (internal::rxRaw) {
        mqttSend(build::topicRxRaw(), result.asRawTime().c_str());
    }
}

void configure() {
    internal::rx = settings::rx();
    internal::rxRaw = settings::rxRaw();
}

void setup() {
    mqttRegister(internal::callback);
}

} // namespace mqtt
#endif

#if TERMINAL_SUPPORT
namespace terminal {

struct ValueCommand {
    const __FlashStringHelper* value;
    const __FlashStringHelper* command;
};

struct Preset {
    const ValueCommand* const begin;
    const ValueCommand* const end;
};

namespace build {

// TODO: optimize the array itself via PROGMEM? can't be static though, b/c F(...) will be resolved later and the memory is empty in the flash
//       also note of the alignment requirements that don't always get applied to a simple PROGMEM'ed array (unless explicitly set, or the contained value is aligned)
//       strings vs. number for values do have a slight overhead (x2 pointers, byte-by-byte cmp instead of a 2byte memcmp), but it seems to be easier to handle here
// TODO: have an actual name for presets (remote, device, etc.)?
// TODO: user-defined presets?
// TODO: pub-sub through terminal?

// Replaced old ir_button.h IR_BUTTON_ACTION_... with an appropriate terminal command
// Unlike the RFbridge implementation, does not depend on the RELAY_SUPPORT and it's indexes

#if IR_RX_PRESET != 0

Preset preset() {
#if IR_RX_PRESET == 1
// For the original Remote shipped with the controller
// +------+------+------+------+
// |  UP  | Down | OFF  |  ON  |
// +------+------+------+------+
// |  R   |  G   |  B   |  W   |
// +------+------+------+------+
// |  1   |  2   |  3   |FLASH |
// +------+------+------+------+
// |  4   |  5   |  6   |STROBE|
// +------+------+------+------+
// |  7   |  8   |  9   | FADE |
// +------+------+------+------+
// |  10  |  11  |  12  |SMOOTH|
// +------+------+------+------+
    static const std::array<ValueCommand, 20> instance {
        {{F("FF906F"), F("brightness +10")},
        {F("FFB847"), F("brightness -10")},
        {F("FFF807"), F("light off")},
        {F("FFB04F"), F("light on")},

        {F("FF9867"), F("rgb #FF0000")},
        {F("FFD827"), F("rgb #00FF00")},
        {F("FF8877"), F("rgb #0000FF")},
        {F("FFA857"), F("rgb #FFFFFF")},

        {F("FFE817"), F("rgb #D13A01")},
        {F("FF48B7"), F("rgb #00E644")},
        {F("FF6897"), F("rgb #0040A7")},
        //{F("FFB24D"), F("effect flash")},

        {F("FF02FD"), F("rgb #E96F2A")},
        {F("FF32CD"), F("rgb #00BEBF")},
        {F("FF20DF"), F("rgb #56406F")},
        //{F("FF00FF"), F("effect strobe")},

        {F("FF50AF"), F("rgb #EE9819")},
        {F("FF7887"), F("rgb #00799A")},
        {F("FF708F"), F("rgb #944E80")},
        //{F("FF58A7"), F("effect fade")},

        {F("FF38C7"), F("rgb #FFFF00")},
        {F("FF28D7"), F("rgb #0060A1")},
        {F("FFF00F"), F("rgb #EF45AD")}}
        //{F("FF30CF"), F("effect smooth")}
    };
#elif IR_RX_PRESET == 2
// Another identical IR Remote shipped with another controller
//  +------+------+------+------+
//  |  UP  | Down | OFF  |  ON  |
//  +------+------+------+------+
//  |  R   |  G   |  B   |  W   |
//  +------+------+------+------+
//  |  1   |  2   |  3   |FLASH |
//  +------+------+------+------+
//  |  4   |  5   |  6   |STROBE|
//  +------+------+------+------+
//  |  7   |  8   |  9   | FADE |
//  +------+------+------+------+
//  |  10  |  11  |  12  |SMOOTH|
//  +------+------+------+------+
    static const std::array<ValueCommand, 20> instance {
        {{F("FF00FF"), F("brightness +10")},
        {F("FF807F"), F("brightness -10")},
        {F("FF40BF"), F("light off")},
        {F("FFC03F"), F("light on")},

        {F("FF20DF"), F("rgb #FF0000")},
        {F("FFA05F"), F("rgb #00FF00")},
        {F("FF609F"), F("rgb #0000FF")},
        {F("FFE01F"), F("rgb #FFFFFF")},

        {F("FF10EF"), F("rgb #D13A01")},
        {F("FF906F"), F("rgb #00E644")},
        {F("FF50AF"), F("rgb #0040A7")},
        //{F("FFD02F"), F("effect flash")},

        {F("FF30CF"), F("rgb #E96F2A")},
        {F("FFB04F"), F("rgb #00BEBF")},
        {F("FF708F"), F("rgb #56406F")},
        //{F("FFF00F"), F("effect strobe")},

        {F("FF08F7"), F("rgb #EE9819")},
        {F("FF8877"), F("rgb #00799A")},
        {F("FF48B7"), F("rgb #944E80")},
        //{F("FFC837"), F("effect fade")},

        {F("FF28D7"), F("rgb #FFFF00")},
        {F("FFA857"), F("rgb #0060A1")},
        {F("FF6897"), F("rgb #EF45AD")}}
        //{F("FFE817"), F("effect smooth")}
    };
#elif IR_RX_PRESET == 3
// Samsung AA59-00608A for a generic 8CH module
//  +------+------+------+
//  |  1   |  2   |  3   |
//  +------+------+------+
//  |  4   |  5   |  6   |
//  +------+------+------+
//  |  7   |  8   |  9   |
//  +------+------+------+
//  |      |  0   |      |
//  +------+------+------+
    static const std::array<ValueCommand, 8> instance {
        {{F("E0E020DF"), F("relay 0 toggle")},
        {F("E0E0A05F"), F("relay 1 toggle")},
        {F("E0E0609F"), F("relay 2 toggle")},
        {F("E0E010EF"), F("relay 3 toggle")},
        {F("E0E0906F"), F("relay 4 toggle")},
        {F("E0E050AF"), F("relay 5 toggle")},
        {F("E0E030CF"), F("relay 6 toggle")},
        {F("E0E0B04F"), F("relay 7 toggle")}}
    };
    // Plus, 2 extra buttons (TODO: on each side of 0?)
    // - E0E0708F
    // - E0E08877
#elif IR_RX_PRESET == 4
//  +------+------+------+
//  | OFF  | SRC  | MUTE |
//  +------+------+------+
//  ...
//  +------+------+------+
//  TODO: ...but what's the rest?
    static const std::array<ValueCommand, 1> instance {
        {F("FFB24D"), F("relay 0 toggle")}
    };
#elif IR_RX_PRESET == 5
// Another identical IR Remote shipped with another controller as SET 1 and 2
// +------+------+------+------+
// |  UP  | Down | OFF  |  ON  |
// +------+------+------+------+
// |  R   |  G   |  B   |  W   |
// +------+------+------+------+
// |  1   |  2   |  3   |FLASH |
// +------+------+------+------+
// |  4   |  5   |  6   |STROBE|
// +------+------+------+------+
// |  7   |  8   |  9   | FADE |
// +------+------+------+------+
// |  10  |  11  |  12  |SMOOTH|
// +------+------+------+------+
    static const std::array<ValueCommand, 20> instance {
        {{F("F700FF"), F("brightness +10")},
        {F("F7807F"), F("brightness -10")},
        {F("F740BF"), F("light off")},
        {F("F7C03F"), F("light on")},

        {F("F720DF"), F("rgb #FF0000")},
        {F("F7A05F"), F("rgb #00FF00")},
        {F("F7609F"), F("rgb #0000FF")},
        {F("F7E01F"), F("rgb #FFFFFF")},

        {F("F710EF"), F("rgb #D13A01")},
        {F("F7906F"), F("rgb #00E644")},
        {F("F750AF"), F("rgb #0040A7")},
        //{F("F7D02F"), F("effect flash")},

        {F("F730CF"), F("rgb #E96F2A")},
        {F("F7B04F"), F("rgb #00BEBF")},
        {F("F7708F"), F("rgb #56406F")},
        //{F("F7F00F"), F("effect strobe")},

        {F("F708F7"), F("rgb #EE9819")},
        {F("F78877"), F("rgb #00799A")},
        {F("F748B7"), F("rgb #944E80")},
        //{F("F7C837"), F("effect fade")},

        {F("F728D7"), F("rgb #FFFF00")},
        {F("F7A857"), F("rgb #0060A1")},
        {F("F76897"), F("rgb #EF45AD")}}
        //{F("F7E817"), F("effect smooth")}
    };
#else
#error "Preset is not handled"
#endif

    return {std::begin(instance), std::end(instance)};
}

#endif

} // namespace build

namespace internal {

void inject(String command) {
    terminalInject(command.c_str(), command.length());
    if (!command.endsWith("\r\n") && !command.endsWith("\n")) {
        terminalInject('\n');
    }
}

} // namespace internal

void process(rx::DecodeResult& result) {
#if IR_RX_PRESET != 0
    auto preset = build::preset();
    if (preset.begin && preset.end && (preset.begin != preset.end)) {
        for (auto* it = preset.begin; it != preset.end; ++it) {
            String other((*it).value);
            if (other == result.asValue()) {
                internal::inject((*it).command);
                return;
            }
        }
    }
#endif

    String key;
    key += F("irCmd");
    key += result.asValue();

    auto cmd = ::settings::internal::get(key);
    if (cmd) {
        internal::inject(cmd.ref());
    }
}

void setup() {
    terminalRegisterCommand(F("IR.SEND"), [](const ::terminal::CommandContext& ctx) {
        if (ctx.argv.size() == 2) {
            auto decoded = ir::simple::parse(StringView{ctx.argv[1]});
            if (decoded.ok()) {
                ir::tx::enqueue(std::move(decoded).move());
                terminalOK(ctx);
                return;
            }

            terminalError(ctx, F("Invalid payload"));
            return;
        }

        terminalError(ctx, F("IR.SEND <SIMPLE PAYLOAD>"));
    });
}

} // namespace terminal
#endif

namespace rx {

// TODO: rpnlib support like with rfbridge stringified callbacks?
// TODO: receiver uses os timers to schedule things, isr and the system task do the actual processing

void process(DecodeResult&& result) {
#if DEBUG_SUPPORT
    DEBUG_MSG_P(PSTR("[IR] IN: %.*s\n"),
        result.asSimplePayload().length(),
        result.asSimplePayload().c_str());
#endif
#if TERMINAL_SUPPORT
    ir::terminal::process(result);
#endif
#if MQTT_SUPPORT
    ir::mqtt::process(result);
#endif
}

void loop() {
    static ::decode_results result;
    if (internal::instance->decode(&result)) {
        if (result.overflow) {
            return;
        }

        static unsigned long last { millis() - build::rxDelay() - 1ul };
        unsigned long ts { millis() };
        if (ts - last < build::rxDelay()) {
            return;
        }

        last = ts;
        process(DecodeResult(result));
    }
}

} // namespace rx

#if RELAY_SUPPORT
namespace relay {
namespace settings {

String relayOn(size_t id) {
    return getSetting({"irRelayOn", id});
}

String relayOff(size_t id) {
    return getSetting({"irRelayOff", id});
}

} // namespace settings

namespace internal {

void callback(size_t id, bool status) {
    auto cmd = status
        ? settings::relayOn(id)
        : settings::relayOff(id);
    if (!cmd.length()) {
        return;
    }

    StringView view{cmd};
    ir::tx::enqueue(ir::simple::parse(view));
}

} // namespace internal

void setup() {
    ::relayOnStatusNotify(internal::callback);
    ::relayOnStatusChange(internal::callback);
}

} // namespace relay
#endif

void configure() {
#if MQTT_SUPPORT
    mqtt::configure();
#endif
}

void setup() {
    auto rxPin = gpioRegister(settings::rx());
    if (rxPin) {
        DEBUG_MSG_P(PSTR("[IR] Receiver on GPIO%hhu\n"), rxPin->pin());
    } else {
        DEBUG_MSG_P(PSTR("[IR] No receiver configured\n"));
    }

    auto txPin = gpioRegister(settings::tx());
    if (txPin) {
        DEBUG_MSG_P(PSTR("[IR] Transmitter on GPIO%hhu\n"), txPin->pin());
    } else {
        DEBUG_MSG_P(PSTR("[IR] No transmitter configured\n"));
    }

    if (!rxPin && !txPin) {
        return;
    }

    espurnaRegisterReload(configure);
    configure();

    if (rxPin && txPin) {
        ::espurnaRegisterLoop([]() {
            ir::rx::loop();
            ir::tx::loop();
        });
    } else if (rxPin) {
        ::espurnaRegisterLoop([]() {
            ir::rx::loop();
        });
    } else if (txPin) {
        ::espurnaRegisterLoop([]() {
            ir::tx::loop();
        });
    }

    if (txPin) {
#if MQTT_SUPPORT
        ir::mqtt::setup();
#endif
#if RELAY_SUPPORT
        ir::relay::setup();
#endif
#if TERMINAL_SUPPORT
        ir::terminal::setup();
#endif
    }

    if (txPin) {
        tx::setup(std::move(txPin));
    }

    if (rxPin) {
        rx::setup(std::move(rxPin));
    }
}

} // namespace
} // namespace ir

#if IR_TEST_SUPPORT
namespace ir {
namespace {
namespace test {

// TODO: may be useful if struct and values comparison error dump happens. but, not really nice looking for structs b/c of the length and no field highlight

#if 0
String serialize(const ::ir::simple::Payload& payload) {
    String out;
    out.reserve(128);

    out += F("{ .type=decode_type_t::");
    out += typeToString(payload.type);
    out += F(", ");

    out += F(".value=");
    out += ::ir::simple::value::encode(payload.value);
    out += F(", ");

    out += F(".bits=");
    out += String(payload.bits, 10);
    out += F(", ");

    out += F(".repeats=");
    out += String(payload.repeats, 10);
    out += F(", ");

    out += F(".series=");
    out += String(payload.series, 10);
    out += F(", ");

    out += F(".delay=");
    out += String(payload.delay, 10);
    out += F(" }");

    return out;
}

String serialize(const ::ir::raw::Payload& payload) {
    String out;
    out.reserve(128);

    out += F("{ .frequency=");
    out += String(payload.frequency, 10);
    out += F(", ");

    out += F(".series=");
    out += String(payload.series, 10);
    out += F(", ");

    out += F(".delay=");
    out += String(payload.delay, 10);
    out += F(", ");

    out += F(".time[");
    out += String(payload.time.size(), 10);
    out += F("]={");

    bool comma { false };
    for (auto& value : payload.time) {
        if (comma) {
            out += F(", ");
        }
        out += String(value, 10);
        comma = true;
    }

    out += F("} }");

    return out;
}

#endif

struct Report {
    Report(int line, String&& repr) :
        _line(line),
        _repr(std::move(repr))
    {}

    int line() const {
        return _line;
    }

    const String& repr() const {
        return _repr;
    }

private:
    int _line;
    String _repr;
};

using Reports = std::vector<Report>;

struct Context {
    struct View {
        explicit View(Context& context) :
            _context(context)
        {}

        template <typename... Args>
        void report(Args&&... args) {
            _context.report(std::forward<Args>(args)...);
        }

    private:
        Context& _context;
    };

    using Runner = void(*)(View&);
    using Runners = std::initializer_list<Runner>;

    Context(Runners runners) :
        _begin(std::begin(runners)),
        _end(std::end(runners))
    {
        run();
    }

#if DEBUG_SUPPORT
    ~Context() {
        DEBUG_MSG_P(PSTR("[IR TEST] %s\n"),
                _reports.size() ? "FAILED" : "SUCCESS");
        for (auto& report : _reports) {
            DEBUG_MSG_P(PSTR("[IR TEST] " __FILE__ ":%d '%.*s'\n"),
                    report.line(), report.repr().length(), report.repr().c_str());
        }
    }
#endif

    template <typename... Args>
    void report(Args&&... args) {
        _reports.emplace_back(std::forward<Args>(args)...);
    }

private:
    void run() {
        View view(*this);
        for (auto* it = _begin; it != _end; ++it) {
            (*it)(view);
        }
    }

    const Runner* _begin;
    const Runner* _end;

    Reports _reports;
};

// TODO: unity and pio-test? would need to:
// - use `test_build_project_src = yes` in the .ini
// - disable `DEBUG_SERIAL_SUPPORT` in case it's on `Serial` or anything else allowing output to the `Serial`
//   (some code gets automatically generated when `pio test` is called that contains setUp(), tearDown(), etc.)
// - have more preprocessor-wrapped chunks
// - not depend on destructors, since unity uses setjmp and longjmp
//   (or use `-DUNITY_EXCLUDE_SETJMP_H`)

// TODO: anything else header-only? may be a problem though with c++ approach, as most popular frameworks depend on std::ostream

// TODO: for parsers specifically, some fuzzing generating random strings and test order randomization could be useful
//       (also, extending the current set of tests and / or having some helper macro that can fill the boilerplate)

// As a (temporary?) current solution, have these 4 macros that setup a Context object and a list of test runners.
// Each runner may call `IR_CHECK(<something resolving to bool>)` to immediatly exit current block on failure and save report to the Context object.
// On destruction of the Context object, every report is printed to the debug output.

#define IR_CHECK_SETUP_BEGIN() Context runner ## __FILE__ ## __LINE__ {
#define IR_CHECK_SETUP_END() }

#define IR_CHECK_RUNNER() [](Context::View& __context_view)

#define IR_CHECK(EXPRESSION) {\
    if (!(EXPRESSION)) {\
        __context_view.report(__LINE__, F(#EXPRESSION));\
        return;\
    }\
}

void setup() {
    IR_CHECK_SETUP_BEGIN() {
        IR_CHECK_RUNNER() {
            IR_CHECK(!ir::simple::parse("").ok());
        },
        IR_CHECK_RUNNER() {
            IR_CHECK(!ir::simple::parse(",").ok());
        },
        IR_CHECK_RUNNER() {
            IR_CHECK(!ir::simple::parse("999::").ok());
        },
        IR_CHECK_RUNNER() {
            IR_CHECK(!ir::simple::parse("-5:doesntmatter").ok());
        },
        IR_CHECK_RUNNER() {
            IR_CHECK(!ir::simple::parse("2:0:31").ok());
        },
        IR_CHECK_RUNNER() {
            IR_CHECK(!ir::simple::parse("2:112233445566778899AA:31").ok());
        },
        IR_CHECK_RUNNER() {
            auto result = ir::simple::parse("2:7FAABBCC:31");
            IR_CHECK(result.ok());

            auto& payload = result.get();
            IR_CHECK(payload.type == decode_type_t::RC6);
            IR_CHECK(payload.value == static_cast<uint64_t>(0x7faabbcc));
            IR_CHECK(payload.bits == 31);
        },
        IR_CHECK_RUNNER() {
            auto result = ir::simple::parse("15:AABBCCDD:25:3");
            IR_CHECK(result.ok());

            auto& payload = result.get();
            IR_CHECK(payload.type == decode_type_t::COOLIX);
            IR_CHECK(payload.value == static_cast<uint64_t>(0xaabbccdd));
            IR_CHECK(payload.bits == 25);
            IR_CHECK(payload.repeats == 3);
        },
        IR_CHECK_RUNNER() {
            auto result = ir::simple::parse("10:0FEFEFEF:21:2:5:500");
            IR_CHECK(result.ok());

            auto& payload = result.get();
            IR_CHECK(payload.type == decode_type_t::LG);
            IR_CHECK(payload.value == static_cast<uint64_t>(0x0fefefef));
            IR_CHECK(payload.bits == 21);
            IR_CHECK(payload.repeats == 2);
            IR_CHECK(payload.series == 5);
            IR_CHECK(payload.delay == 500);
        },
        IR_CHECK_RUNNER() {
            IR_CHECK(!ir::raw::parse("-1:1:500:,200,150,250,50,100,100,150").ok());
        },
        IR_CHECK_RUNNER() {
            auto result = ir::raw::parse("38:1:500:100,200,150,250,50,100,100,150");
            IR_CHECK(result.ok());

            auto& payload = result.get();
            IR_CHECK(payload.frequency == 38);
            IR_CHECK(payload.series == 1);
            IR_CHECK(payload.delay == 500);

            decltype(ir::raw::Payload::time) expected_time {
                100, 200, 150, 250, 50, 100, 100, 150};
            IR_CHECK(expected_time == payload.time);
        },
        IR_CHECK_RUNNER() {
            IR_CHECK(::ir::simple::value::encode(0xffaabbccddee) == F("FFAABBCCDDEE"));
        },
        IR_CHECK_RUNNER() {
            IR_CHECK(::ir::simple::value::encode(0xfaabbccddee) == F("0FAABBCCDDEE"));
        },
        IR_CHECK_RUNNER() {
            IR_CHECK(::ir::simple::value::encode(0xee) == F("EE"));
        },
        IR_CHECK_RUNNER() {
            IR_CHECK(::ir::simple::value::encode(0) == F("00"));
        },
        IR_CHECK_RUNNER() {
            const uint16_t raw[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
            IR_CHECK(::ir::raw::time::encode(std::begin(raw), std::end(raw)) == F("2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32"));
        }
    }
    IR_CHECK_SETUP_END();
}

} // namespace test
} // namespace
} // namespace ir
#endif

void irSetup() {
#if IR_TEST_SUPPORT
    ir::test::setup();
#endif
    ir::setup();
}

#endif
