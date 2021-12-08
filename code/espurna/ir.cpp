/*
IR MODULE

Copyright (C) 2018 by Alexander Kolesnikov (raw and MQTT implementation)
Copyright (C) 2017-2019 by François Déchery
Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

For the library, see:
https://github.com/crankyoldgit/IRremoteESP8266

To (re)create the string -> Payload decoder .ipp files, add `re2c` to the $PATH and 'run' the environment:
```
$ pio run -e ... \
    -t espurna/ir_parse_simple.re.ipp \
    -t espurna/ir_parse_state.re.ipp \
    -t espurna/ir_parse_raw.re.ipp
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

// TODO: current library version injects a bunch of stuff into the global scope:
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
namespace tx {

// Notice that IRremoteEsp8266 includes a *lot* of built-in protocols. the suggested way to build the library
// is to append `-D_IR_ENABLE_DEFAULT_=false` to the build flags and specify the individual `-DSEND_...`
// and `-DDECODE_...` *only* for the required one(s)
//
// `-DIR_TX_SUPPORT=0` disables considerable amount of stuff linked inside of the `IRsend` class (~35KiB), but for
// every category of payloads at the same time; simple, raw and state will all be gone. *It is possible* to introduce
// a more granular control, but idk if it's really worth it (...and likely result in an inextricable web of `#if`s and `#ifdef`s)

#if not IR_TX_SUPPORT

struct NoopSender {
    NoopSender(uint16_t, bool, bool) {
    }

    void begin() {
    }

    bool send(decode_type_t, const uint8_t*, uint16_t) {
        return false;
    }

    bool send(decode_type_t, uint64_t, uint16_t, uint16_t) {
        return false;
    }

    void sendRaw(const uint16_t*, uint16_t, uint16_t) {
    }
};

#define IRsend ::ir::tx::NoopSender
#endif

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

using PayloadSenderPtr = std::unique_ptr<PayloadSenderBase>;

namespace build {

// pin that the transmitter is attached to
constexpr unsigned char pin() {
    return IR_TX_PIN;
}

// (optional) whether the LED will turn ON when GPIO is LOW and OFF when it's HIGH
// (disabled by default)
constexpr bool inverted() {
    return IR_TX_INVERTED == 1;
}

// (optional) enable frequency modulation (ref. IRsend.h, enabled by default and assumes 50% duty cycle)
// (usage is 'hidden' by the protocol handlers, which use `::enableIROut(frequency, duty)`)
constexpr bool modulation() {
    return IR_TX_MODULATION == 1;
}

// (optional) number of times that the message will be sent immediately
// (i.e. when the [:<repeats>] is omitted from the MQTT payload)
constexpr uint16_t repeats() {
    return IR_TX_REPEATS;
}

// (optional) number of times that the message will be scheduled in the TX queue
// (i.e. when the [:<series>] is omitted from the MQTT payload)
constexpr uint8_t series() {
    return IR_TX_SERIES;
}

// (ms)
constexpr unsigned long delay() {
    return IR_TX_DELAY;
}

} // namespace build

namespace settings {

unsigned char pin() {
    return getSetting("irTx", build::pin());
}

bool inverted() {
    return getSetting("irTxInv", build::inverted());
}

bool modulation() {
    return getSetting("irTxMod", build::modulation());
}

uint16_t repeats() {
    return getSetting("rxTxRepeats", build::repeats());
}

uint8_t series() {
    return getSetting("rxTxSeries", build::series());
}

unsigned long delay() {
    return getSetting("irTxDelay", build::delay());
}

} // namespace settings

namespace internal {

uint16_t repeats { build::repeats() };
uint8_t series { build::series() };
unsigned long delay { build::delay() };

BasePinPtr pin;
std::unique_ptr<IRsend> instance;

std::queue<PayloadSenderPtr> queue;

} // namespace internal
} // namespace tx

namespace rx {

// `-DIR_RX_SUPPORT=0` disables everything related to the `IRrecv` class, ~20KiB
// (note that receiver objects are still techically there, just not doing anything)

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

    void enableIRIn(bool) const {
    }
};

#define IRrecv ::ir::rx::NoopReceiver
#endif

namespace build {

// pin that the receiver is attached to
constexpr unsigned char pin() {
    return IR_RX_PIN;
}

// library handles both the isr timer and the pinMode in the same method,
// can't be set externally and must be passed into the `enableIRIn(bool)`
constexpr bool pullup() {
    return IR_RX_PULLUP;
}

// internally, lib uses an u16[] of this size
constexpr uint16_t bufferSize() {
    return IR_RX_BUFFER_SIZE;
}

// to be isr-friendly, will allocate second u16[]
// that will be used as a storage when decode()'ing
constexpr bool bufferSave() {
    return true;
}

// allow unknown protocols to pass through to the processing
// (notice that this *will* cause RAW output to stop working)
constexpr bool unknown() {
    return IR_RX_UNKNOWN;
}

// (ms)
constexpr uint8_t timeout() {
    return IR_RX_TIMEOUT;
}

// (ms) minimal time in-between decode() calls
constexpr unsigned long delay() {
    return IR_RX_DELAY;
}

} // namespace build

namespace settings {

// specific to the IRrecv

unsigned char pin() {
    return getSetting("irRx", build::pin());
}

bool pullup() {
    return getSetting("irRxPullup", build::pullup());
}

uint16_t bufferSize() {
    return getSetting("irRxBufSize", build::bufferSize());
}

uint8_t timeout() {
    return getSetting("irRxTimeout", build::timeout());
}

// local settings

bool unknown() {
    return getSetting("irRxUnknown", build::unknown());
}

unsigned long delay() {
    return getSetting("irRxDelay", build::delay());
}

} // namespace settings

namespace internal {

bool pullup { build::pullup() };
bool unknown { build::unknown() };
unsigned long delay { build::delay() };

BasePinPtr pin;
std::unique_ptr<IRrecv> instance;

} // namespace internal
} // namespace rx

// TODO: some -std=c++11 things. *almost* direct ports of std::string_view and std::optional
//       (may be aliased via `using` and depend on the __cplusplus? string view is not 1-to-1 though)
//       obviously, missing most of constexpr init and std::optional optimization related to trivial ctors & dtors
//
// TODO: since the exceptions are disabled, and parsing failure is not really an 'exceptional' result anyway...
//       result struct may be in need of an additional struct describing the error, instead of just a boolean true or false
//       (something like std::expected - http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0323r8.html)
//       current implementation may be adjusted, but not the using DecodeResult = std::optional<T> mentioned above

struct StringView {
    StringView() = delete;
    ~StringView() = default;

    constexpr StringView(const StringView&) noexcept = default;
    constexpr StringView(StringView&&) noexcept = default;

#if __cplusplus > 201103L
    constexpr StringView& operator=(const StringView&) noexcept = default;
    constexpr StringView& operator=(StringView&&) noexcept = default;
#else
    StringView& operator=(const StringView&) noexcept = default;
    StringView& operator=(StringView&&) noexcept = default;
#endif

    constexpr StringView(std::nullptr_t) noexcept :
        _begin(nullptr),
        _length(0)
    {}

    constexpr StringView(const char* begin, size_t length) noexcept :
        _begin(begin),
        _length(length)
    {}

    constexpr StringView(const char* begin, const char* end) noexcept :
        StringView(begin, end - begin)
    {}

    template <size_t Size>
    constexpr StringView(const char (&string)[Size]) noexcept :
        StringView(&string[0], Size - 1)
    {}

    StringView& operator=(const String& string) noexcept {
        _begin = string.c_str();
        _length = string.length();
        return *this;
    }

    explicit StringView(const String& string) noexcept :
        StringView(string.c_str(), string.length())
    {}

    template <size_t Size>
    constexpr StringView& operator=(const char (&string)[Size]) noexcept {
        _begin = &string[0];
        _length = Size - 1;
        return *this;
    }

    const char* begin() const noexcept {
        return _begin;
    }

    const char* end() const noexcept {
        return _begin + _length;
    }

    constexpr size_t length() const noexcept {
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

    explicit operator bool() const noexcept {
        return _initialized;
    }

    T* operator->() {
        return &_result._value;
    }

    const T* operator->() const {
        return &_result._value;
    }

    bool has_value() const noexcept {
        return _initialized;
    }

    const T& value() const & {
        return _result._value;
    }

    T& value() & {
        return _result._value;
    }

    T&& value() && {
        return std::move(_result._value);
    }

    const T&& value() const && {
        return std::move(_result._value);
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

    // TODO: c++ std compliance may enforce weird optimizations if T contains const or reference members, ref.
    // - http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0532r0.pdf
    // - https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95349

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

// TODO: std::from_chars works directly with the view. not available with -std=c++11,
//       and needs some care in regards to the code size

template <typename T>
T sized(StringView view) {
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
unsigned long sized(StringView view) {
    String value(view);

    char* endp { nullptr };
    unsigned long result { std::strtoul(value.c_str(), &endp, 10) };
    if ((endp != value.c_str()) && (*endp == '\0')) {
        return result;
    }

    return 0;
}

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

// TODO: type is numeric based on the previous implementation. note that there are
// `::typeToString(decode_type_t)` and `::strToDecodeType(const char*)` (IRutils.h)
// And also see `const char kAllProtocolNames*`, which is a global from the IRtext header with
// \0-terminated chunks of stringivied decode_type_t (counting 'index' will deduce the type)
//
// (but, notice that str->type only works with C strings and *will* do a permissive
// `strToDecodeType(typeToString(static_cast<decode_type_t>(atoi(str))))` when the
// intial attempt fails)

namespace simple {

struct Payload {
    decode_type_t type;
    uint64_t value;
    uint16_t bits;
    uint16_t repeats;
    uint8_t series;
    unsigned long delay;
};

namespace value {

// TODO: endianness of input is always 'big', output is 'little'
//       all esp platforms and common build hosts are 'little'
//       but, actually make sure bswap is necessary?

// To convert from an existing decimal value, there is a python one-liner:
// >>> bytes(x for x in (123456789).to_bytes(8, 'big', signed=False) if x).hex()
// '075bcd15'
// (and also notice that old version *always* cast `u64` into `u32` which cut off part of the code)

uint64_t decode(StringView view) {
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

decode_type_t type(StringView view) {
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

uint64_t value(StringView view) {
    return ::ir::simple::value::decode(view);
}

uint16_t bits(StringView value) {
    return sized<uint16_t>(value);
}

uint16_t repeats(StringView value) {
    return sized<uint16_t>(value);
}

uint8_t series(StringView value) {
    return sized<uint8_t>(value);
}

unsigned long delay(StringView value) {
    return sized<unsigned long>(value);
}

template <typename T>
String encode(T& result) {
    String out;
    out.reserve(28);

    out += static_cast<int>(result.type());
    out += ':';

    out += value::encode(result.value());
    out += ':';

    out += static_cast<unsigned int>(result.bits());

    return out;
}

} // namespace payload

Payload prepare(StringView type, StringView value, StringView bits, StringView repeats, StringView series, StringView delay) {
    Payload result;
    result.type = payload::type(type);
    result.value = payload::value(value);
    result.bits = payload::bits(bits);

    if (repeats) {
        result.repeats = payload::repeats(repeats);
    } else {
        result.repeats = tx::internal::repeats;
    }

    if (series) {
        result.series = payload::series(series);
    } else {
        result.series = tx::internal::series;
    }

    if (delay) {
        result.delay = payload::delay(delay);
    } else {
        result.delay = tx::internal::delay;
    }

    return result;
}

#include "ir_parse_simple.re.ipp"

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

// Also see IRutils.h's `String resultToTimingInfo(decode_results*)` for all of timing info, with a nice table output
// Not really applicable here, though

namespace raw {

static_assert((DECODE_HASH), "");

struct Payload {
    uint16_t frequency;
    uint8_t series;
    unsigned long delay;
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

uint16_t frequency(StringView value) {
    return sized<uint16_t>(value);
}

uint8_t series(StringView value) {
    return sized<uint8_t>(value);
}

unsigned long delay(StringView value) {
    return sized<unsigned long>(value);
}

uint16_t time(StringView value) {
    return sized<uint16_t>(value);
}

template <typename T>
String encode(T& result) {
    auto raw = result.raw();
    if (raw) {
        return time::encode(raw.begin(), raw.end());
    }

    return F("0");
}

} // namespace payload

Payload prepare(StringView frequency, StringView series, StringView delay, decltype(Payload::time)&& time) {
    Payload result;
    result.frequency = payload::frequency(frequency);
    result.series = payload::series(series);
    result.delay = payload::delay(delay);
    result.time = std::move(time);

    return result;
}

#include "ir_parse_raw.re.ipp"

} // namespace raw

// TODO: current solution works directly with the internal 'u8 state[]', both for receiving and sending
// a more complex protocols for HVAC equipment *could* be handled by the IRacUtils (ref. IRac.h)
// where a generic 'IRac' class will convert certain common properties like temperature, fan speed,
// fan direction and power toggle (and some more, see 'stdAc::state_t'; or, the specific vendor class)
//
// Some problems with state_t, though:
// - not everything is 1-to-1 convertible with specific-protocol-AC-class to state_t
//   (or not directly, or with some unexpected limitations)
// - there's no apparent way to know which properties are supported by the protocol.
//   protocol-specific classes (e.g. MitsubishiAC) will convert to state_t by omitting certain fields,
//   and parse it by ignoring them. but, this is hidden in the implementation
// - some protocols require previous state as a reference for sending, and IRac already has an internal copy
//   if the state_t struct. but, notice that it is shared between protocols (as a generic way), so mixing
//   protocols becomes are bit of a headache
// - size of the payload is as wide as the largest one, so there's always a static blob of N
//   bytes reserved, both inside and with the proposed API of the library
//   saving state (to avoid always resetting to defaults on reboot) also becomes a problem,
//
// For a generic solution, supporting state_t would mean to allow to set *every* property declared by the struct
// Common examples and libraries wrapping IRac prefer JSON payload, and both IRac and IRutils contain helpers to convert
// each property to and from strings.
//
// But, preper to split HVAC into a different module, as none of the queueing or generic code facilities are actually useful.

namespace state {

// State messages transmit an arbitrary amount of bytes, by using the assosicated protocol method
// Repeats are intended to be handled via the respective PROTOCOL method automatically
// (and, there's no reliable way besides linking every type with it's method from our side)
//
// Transmitting:
//   Payload: <protocol>:<value>[:<series>][:<delay>]
//
//   Required parameters:
//     PROTOCOL - decimal ID, will be converted into a named 'decode_type_t'
//                (ref. IRremoteESP8266.h and it's protocol descriptions)
//     VALUE    - hexadecimal representation of the value that will be sent
//                (big endian, maximum depends on the protocol settings)
//
//   Optional payload parameters:
//     SERIES   - how many times the message will be scheduled for sending
//                (defaults to 1 aka once, [1...120))
//     DELAY    - minimum amount of time (ms) between queued messages
//                (defaults is IR_TX_DELAY, applies to every message in the series)
//
// Receiving:
//   Payload: 52:112233445566778899AABB (<protocol>:<value>)

static_assert(
    sizeof(decltype(decode_results::state)) >= sizeof(decltype(decode_results::value)),
    "Unsupported version of IRremoteESP8266");

using Value = std::vector<uint8_t>;

struct Payload {
    decode_type_t type;
    Value value;
    uint8_t series;
    unsigned long delay;
};

namespace value {

String encode(const uint8_t* begin, const uint8_t* end) {
    return hexEncode(begin, end);
}

template <typename T>
String encode(T&& range) {
    return hexEncode(range.begin(), range.end());
}

Value decode(StringView view) {
    Value out;
    if (!(view.length() % 2)) {
        out.resize(view.length() / 2, static_cast<uint8_t>(0));
        hexDecode(view.begin(), view.end(),
                out.data(), out.data() + out.size());
    }

    return out;
}

} // namespace value

namespace payload {

template <typename T>
String encode(T& result) {
    String out;
    out.reserve(4 + (result.bytes() * 2));

    out += static_cast<int>(result.type());
    out += ':';

    auto state = result.state();
    out += value::encode(state.begin(), state.end());

    return out;
}

} // namespace payload

Payload prepare(StringView type, StringView value, StringView series, StringView delay) {
    Payload result;
    result.type = ::ir::simple::payload::type(type);
    result.value = value::decode(value);

    if (series) {
        result.series = simple::payload::series(series);
    } else {
        result.series = tx::internal::series;
    }

    if (delay) {
        result.delay = simple::payload::delay(delay);
    } else {
        result.delay = tx::internal::delay;
    }

    return result;
}

#include "ir_parse_state.re.ipp"

} // namespace state

namespace rx {

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
            internal::instance->enableIRIn(internal::pullup);
        }
    }
};

void configure() {
    internal::delay = settings::delay();
    internal::pullup = settings::pullup();
    internal::unknown = settings::unknown();
}

void setup(BasePinPtr&& pin) {
    internal::pin = std::move(pin);
    internal::instance = std::make_unique<IRrecv>(
            internal::pin->pin(),
            settings::bufferSize(),
            settings::timeout(),
            build::bufferSave());
    internal::instance->enableIRIn(internal::pullup);
}

// Current implementation relies on the HEX-encoded 'value' (previously, decimal)
//
// XXX: when protocol is UNKNOWN, `value` is silently replaced with a fnv1 32bit hash.
//      can be disabled with `-DDECODE_HASH=0` in the build flags, but it will also
//      cause RAW output to stop working, as the `IRrecv::decode()` can never succeed :/
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
// Where `kStateSizeMax` is either:
// - deduced from the largest protocol from the `DECODE_AC` group, *if* any of the protocols is enabled
// - otherwise, it's just `sizeof(uint64_t)`
// (i.e. only extra data is lost, as union members always start at the beginning of the struct)

// Also see IRutils.h's `String resultToHumanReadableBasic(decode_results*);` for type + value as a single line

struct DecodeResult {
    template <typename T>
    struct Range {
        Range() = default;
        Range(const T* begin, const T* end) :
            _begin(begin),
            _end(end)
        {}

        const T* begin() const {
            return _begin;
        }

        const T* end() const {
            return _end;
        }

        explicit operator bool() const {
            return _begin && _end && (_begin < _end);
        }

    private:
        const T* _begin { nullptr };
        const T* _end { nullptr };
    };

    DecodeResult() = delete;
    explicit DecodeResult(::decode_results& result) :
        _result(result)
    {}

    decode_type_t type() const {
        return _result.decode_type;
    }

    explicit operator bool() const {
        return type() != decode_type_t::UNKNOWN;
    }

    uint16_t bits() const {
        return _result.bits;
    }

    uint64_t value() const {
        return _result.value;
    }

    // TODO: library examples (and some internal code, too) prefer this to be `bits() / 8`

    size_t bytes() const {
        const size_t Bits { bits() };
        size_t need { 0 };

        size_t out { 0 };
        while (need < Bits) {
            need += 8u;
            out += 1u;
        }

        return out;
    }

    using Raw = Range<uint16_t>;

    Raw raw() const {
        if (_result.rawlen > 1) {
            return Raw{
                const_cast<const uint16_t*>(&_result.rawbuf[1]),
                const_cast<const uint16_t*>(&_result.rawbuf[_result.rawlen])};
        }

        return {};
    }

    using State = Range<uint8_t>;

    State state() const {
        const size_t End { std::min(bytes(), sizeof(decltype(_result.state))) };
        return State{
            &_result.state[0],
            &_result.state[End]};
    }

private:
    const ::decode_results& _result;
};

} // namespace rx

namespace tx {

// TODO: variant instead of virtuals?

struct ReschedulablePayload : public PayloadSenderBase {
    static constexpr uint8_t SeriesMax { 120 };

    ReschedulablePayload() = delete;
    ~ReschedulablePayload() = default;

    ReschedulablePayload(const ReschedulablePayload&) = delete;
    ReschedulablePayload& operator=(const ReschedulablePayload&) = delete;

    ReschedulablePayload(ReschedulablePayload&&) = delete;
    ReschedulablePayload& operator=(ReschedulablePayload&&) = delete;

    ReschedulablePayload(uint8_t series, unsigned long delay) :
        _series(std::min(series, SeriesMax)),
        _delay(delay)
    {}

    bool reschedule() override {
        return _series && (--_series);
    }

    unsigned long delay() const override {
        return _delay;
    }

protected:
    size_t series() const {
        return _series;
    }

private:
    uint8_t _series;
    unsigned long _delay;
};

struct SimplePayloadSender : public ReschedulablePayload {
    SimplePayloadSender() = delete;
    explicit SimplePayloadSender(ir::simple::Payload&& payload) :
        ReschedulablePayload(payload.series, payload.delay),
        _payload(std::move(payload))
    {}

    bool send(IRsend& sender) const override {
        return series() && sender.send(_payload.type, _payload.value, _payload.bits, _payload.repeats);
    }

private:
    ir::simple::Payload _payload;
};

struct StatePayloadSender : public ReschedulablePayload {
    StatePayloadSender() = delete;
    explicit StatePayloadSender(ir::state::Payload&& payload) :
        ReschedulablePayload(
            (payload.value.size() ? payload.series : 0), payload.delay),
        _payload(std::move(payload))
    {}

    bool send(IRsend& sender) const override {
        return series() && sender.send(_payload.type, _payload.value.data(), _payload.value.size());
    }

private:
    ir::state::Payload _payload;
};

struct RawPayloadSender : public ReschedulablePayload {
    RawPayloadSender() = delete;
    explicit RawPayloadSender(ir::raw::Payload&& payload) :
        ReschedulablePayload(
            (payload.time.size() ? payload.series : 0), payload.delay),
        _payload(std::move(payload))
    {}

    bool send(IRsend& sender) const override {
        if (series()) {
            sender.sendRaw(_payload.time.data(), _payload.time.size(), _payload.frequency);
            return true;
        }

        return false;
    }

private:
    ir::raw::Payload _payload;
};

namespace internal {

PayloadSenderPtr make_sender(ir::simple::Payload&& payload) {
    return std::make_unique<SimplePayloadSender>(std::move(payload));
}

PayloadSenderPtr make_sender(ir::state::Payload&& payload) {
    return std::make_unique<StatePayloadSender>(std::move(payload));
}

PayloadSenderPtr make_sender(ir::raw::Payload&& payload) {
    return std::make_unique<RawPayloadSender>(std::move(payload));
}

void enqueue(PayloadSenderPtr&& sender) {
    queue.push(std::move(sender));
}

} // namespace internal

template <typename T>
bool enqueue(typename ir::ParseResult<T>&& result) {
    if (result) {
        internal::enqueue(internal::make_sender(std::move(result).value()));
        return true;
    }

    return false;
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

void configure() {
    internal::delay = settings::delay();
    internal::series = settings::series();
    internal::repeats = settings::repeats();
}

void setup(BasePinPtr&& pin) {
    internal::pin = std::move(pin);
    internal::instance = std::make_unique<IRsend>(
            internal::pin->pin(),
            settings::inverted(),
            settings::modulation());
    internal::instance->begin();
}

} // namespace tx

#if MQTT_SUPPORT
namespace mqtt {
namespace build {

// (optional) enables simple protocol MQTT rx output
constexpr bool rxSimple() {
    return IR_RX_SIMPLE_MQTT == 1;
}

// (optional) enables MQTT RAW rx output (i.e. time values that we received so far)
constexpr bool rxRaw() {
    return IR_RX_RAW_MQTT == 1;
}

// (optional) enables MQTT state rx output (commonly, HVAC remotes, or anything that has payload larger than 64bit)
// (*may need* increased timeout setting for the receiver, so it could buffer very large messages consistently and not lose some of the parts)
// (*requires* increase buffer size. but, depends on the protocol, so adjust accordingly)
constexpr bool rxState() {
    return IR_RX_STATE_MQTT == 1;
}

// {root}/{topic}

const char* topicRxSimple() {
    return IR_RX_SIMPLE_MQTT_TOPIC;
}

const char* topicTxSimple() {
    return IR_TX_SIMPLE_MQTT_TOPIC;
}

const char* topicRxRaw() {
    return IR_RX_RAW_MQTT_TOPIC;
}

const char* topicTxRaw() {
    return IR_TX_RAW_MQTT_TOPIC;
}

const char* topicRxState() {
    return IR_RX_STATE_MQTT_TOPIC;
}

const char* topicTxState() {
    return IR_TX_STATE_MQTT_TOPIC;
}

} // namespace build

namespace settings {

bool rxSimple() {
    return getSetting("irRxMqtt", build::rxSimple());
}

bool rxRaw() {
    return getSetting("irRxMqttRaw", build::rxRaw());
}

bool rxState() {
    return getSetting("irRxMqttState", build::rxState());
}

} // namespace settings

namespace internal {

bool publish_raw { build::rxRaw() };
bool publish_simple { build::rxSimple() };
bool publish_state { build::rxState() };

void callback(unsigned int type, const char* topic, char* payload) {
    switch (type) {

    case MQTT_CONNECT_EVENT:
        mqttSubscribe(build::topicTxSimple());
        mqttSubscribe(build::topicTxState());
        mqttSubscribe(build::topicTxRaw());
        break;

    case MQTT_MESSAGE_EVENT: {
        StringView view{payload, payload + strlen(payload)};

        String t = mqttMagnitude(topic);
        if (t.equals(build::topicTxSimple())) {
            ir::tx::enqueue(ir::simple::parse(view));
        } else if (t.equals(build::topicTxState())) {
            ir::tx::enqueue(ir::state::parse(view));
        } else if (t.equals(build::topicTxRaw())) {
            ir::tx::enqueue(ir::raw::parse(view));
        }

        break;
    }

    }
}

} // namespace internal

void process(rx::DecodeResult& result) {
    if (internal::publish_state && result && (result.bytes() > 8)) {
        ::mqttSend(build::topicRxState(), ::ir::state::payload::encode(result).c_str());
    } else if (internal::publish_simple) {
        ::mqttSend(build::topicRxSimple(), ::ir::simple::payload::encode(result).c_str());
    }

    if (internal::publish_raw) {
        ::mqttSend(build::topicRxRaw(), ::ir::raw::payload::encode(result).c_str());
    }
}

void configure() {
    internal::publish_raw = settings::rxRaw();
    internal::publish_simple = settings::rxSimple();
    internal::publish_state = settings::rxState();
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
//       but... this also means it *could* seamlessly handle state payloads just as simple values, just by changing the value retrieval function
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
    auto value = ir::simple::value::encode(result.value());

#if IR_RX_PRESET != 0
    auto preset = build::preset();
    if (preset.begin && preset.end && (preset.begin != preset.end)) {
        for (auto* it = preset.begin; it != preset.end; ++it) {
            String other((*it).value);
            if (other == value) {
                internal::inject((*it).command);
                return;
            }
        }
    }
#endif

    String key;
    key += F("irCmd");
    key += value;

    auto cmd = ::settings::internal::get(key);
    if (cmd) {
        internal::inject(cmd.ref());
    }
}

void setup() {
    terminalRegisterCommand(F("IR.SEND"), [](::terminal::CommandContext&& ctx) {
        if (ctx.argv.size() == 2) {
            auto view = StringView{ctx.argv[1]};

            auto simple = ir::simple::parse(view);
            if (ir::tx::enqueue(std::move(simple))) {
                terminalOK(ctx);
                return;
            }

            auto state = ir::state::parse(view);
            if (ir::tx::enqueue(std::move(state))) {
                terminalOK(ctx);
                return;
            }

            auto raw = ir::raw::parse(view);
            if (ir::tx::enqueue(std::move(raw))) {
                terminalOK(ctx);
                return;
            }

            terminalError(ctx, F("Invalid payload"));
            return;
        }

        terminalError(ctx, F("IR.SEND <PAYLOAD>"));
    });
}

} // namespace terminal
#endif

#if DEBUG_SUPPORT
namespace debug {

void log(rx::DecodeResult& result) {
    if (!result) {
        DEBUG_MSG_P(PSTR("[IR] IN unknown value %s\n"),
            ir::simple::value::encode(result.value()).c_str());
    } else if (result.bytes() > 8) {
        DEBUG_MSG_P(PSTR("[IR] IN protocol %d state %s\n"),
            static_cast<int>(result.type()), ir::state::value::encode(result.state()).c_str());
    } else {
        DEBUG_MSG_P(PSTR("[IR] IN protocol %d value %s bits %hu\n"),
            static_cast<int>(result.type()), ir::simple::value::encode(result.value()).c_str(), result.bits());
    }
}

} // namespace debug
#endif

namespace rx {

// TODO: rpnlib support like with rfbridge stringified callbacks?

void process(DecodeResult&& result) {
#if DEBUG_SUPPORT
    ir::debug::log(result);
#endif
#if TERMINAL_SUPPORT
    ir::terminal::process(result);
#endif
#if MQTT_SUPPORT
    ir::mqtt::process(result);
#endif
}

// IRrecv uses os timers to schedule things, isr and the system task do the actual processing
// Unless `bufferSave()` is set to `true`, raw value buffers will be shared with the ISR task.
// After `decode()` call, `result` object does not store the actual data though, but references
// the specific buffer that was allocated by the `instance` constructor.

void loop() {
    static ::decode_results result;
    if (internal::instance->decode(&result)) {
        if (result.overflow) {
            return;
        }

        if ((result.decode_type == decode_type_t::UNKNOWN) && !internal::unknown) {
            return;
        }

        static unsigned long last { millis() - internal::delay - 1ul };
        unsigned long ts { millis() };
        if (ts - last < internal::delay) {
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
    rx::configure();
    tx::configure();
#if MQTT_SUPPORT
    mqtt::configure();
#endif
}

void setup() {
    auto rxPin = gpioRegister(rx::settings::pin());
    if (rxPin) {
        DEBUG_MSG_P(PSTR("[IR] Receiver on GPIO%hhu\n"), rxPin->pin());
    } else {
        DEBUG_MSG_P(PSTR("[IR] No receiver configured\n"));
    }

    auto txPin = gpioRegister(tx::settings::pin());
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

struct NoopPayloadSender : public ir::tx::ReschedulablePayload {
    NoopPayloadSender(uint8_t series, unsigned long delay) :
        ir::tx::ReschedulablePayload(series, delay)
    {}

    bool send(IRsend&) const override {
        return series();
    }
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

// TODO: for parsers specifically, some fuzzing to randomize inputs and test order could be useful
//       (also, extending the current set of tests and / or having some helper macro that can fill the boilerplate)

// As a (temporary?) solution for right now, have these 4 macros that setup a Context object and a list of test runners.
// Each runner may call `IR_TEST(<something resolving to bool>)` to immediatly exit current block on failure and save report to the Context object.
// On destruction of the Context object, every report is printed to the debug output.

#define IR_TEST_SETUP_BEGIN() Context runner ## __FILE__ ## __LINE__ {
#define IR_TEST_SETUP_END() }

#define IR_TEST_RUNNER() [](Context::View& __context_view)

#define IR_TEST(EXPRESSION) {\
    if (!(EXPRESSION)) {\
        __context_view.report(__LINE__, F(#EXPRESSION));\
        return;\
    }\
}

void setup() {
    IR_TEST_SETUP_BEGIN() {
        IR_TEST_RUNNER() {
            IR_TEST(!ir::simple::parse(""));
        },
        IR_TEST_RUNNER() {
            IR_TEST(!ir::simple::parse(","));
        },
        IR_TEST_RUNNER() {
            IR_TEST(!ir::simple::parse("999::"));
        },
        IR_TEST_RUNNER() {
            IR_TEST(!ir::simple::parse("-5:doesntmatter"));
        },
        IR_TEST_RUNNER() {
            IR_TEST(!ir::simple::parse("2:0:31"));
        },
        IR_TEST_RUNNER() {
            IR_TEST(!ir::simple::parse("2:012:31"));
        },
        IR_TEST_RUNNER() {
            IR_TEST(!ir::simple::parse("2:112233445566778899AA:31"));
        },
        IR_TEST_RUNNER() {
            IR_TEST(::ir::simple::value::encode(0xffaabbccddee) == F("FFAABBCCDDEE"));
        },
        IR_TEST_RUNNER() {
            IR_TEST(::ir::simple::value::encode(0xfaabbccddee) == F("0FAABBCCDDEE"));
        },
        IR_TEST_RUNNER() {
            IR_TEST(::ir::simple::value::encode(0xee) == F("EE"));
        },
        IR_TEST_RUNNER() {
            IR_TEST(::ir::simple::value::encode(0) == F("00"));
        },
        IR_TEST_RUNNER() {
            auto result = ir::simple::parse("2:7FAABBCC:31");
            IR_TEST(result.has_value());

            auto& payload = result.value();
            IR_TEST(payload.type == decode_type_t::RC6);
            IR_TEST(payload.value == static_cast<uint64_t>(0x7faabbcc));
            IR_TEST(payload.bits == 31);
        },
        IR_TEST_RUNNER() {
            auto result = ir::simple::parse("15:AABBCCDD:25:3");
            IR_TEST(result.has_value());

            auto& payload = result.value();
            IR_TEST(payload.type == decode_type_t::COOLIX);
            IR_TEST(payload.value == static_cast<uint64_t>(0xaabbccdd));
            IR_TEST(payload.bits == 25);
            IR_TEST(payload.repeats == 3);
        },
        IR_TEST_RUNNER() {
            auto result = ir::simple::parse("10:0FEFEFEF:21:2:5:500");
            IR_TEST(result.has_value());

            auto& payload = result.value();
            IR_TEST(payload.type == decode_type_t::LG);
            IR_TEST(payload.value == static_cast<uint64_t>(0x0fefefef));
            IR_TEST(payload.bits == 21);
            IR_TEST(payload.repeats == 2);
            IR_TEST(payload.series == 5);
            IR_TEST(payload.delay == 500);
        },
        IR_TEST_RUNNER() {
            auto result = ir::simple::parse("20:1122AABBCCDDEEFF:64:2:3:1000");
            IR_TEST(result.has_value());

            auto ptr = std::make_unique<NoopPayloadSender>(
                    result->series, result->delay);
            IR_TEST(ptr->delay() == 1000);

            IRsend sender(GPIO_NONE);
            IR_TEST(ptr->send(sender));
            IR_TEST(ptr->reschedule());

            IR_TEST(ptr->send(sender));
            IR_TEST(ptr->reschedule());

            IR_TEST(ptr->send(sender));
            IR_TEST(!ptr->reschedule());
        },
        IR_TEST_RUNNER() {
            IR_TEST(!ir::state::parse(""));
        },
        IR_TEST_RUNNER() {
            IR_TEST(!ir::state::parse(":"));
        },
        IR_TEST_RUNNER() {
            IR_TEST(!ir::state::parse("-1100,100,150"));
        },
        IR_TEST_RUNNER() {
            IR_TEST(!ir::state::parse("25:"));
        },
        IR_TEST_RUNNER() {
            IR_TEST(!ir::state::parse("30:C"));
        },
        IR_TEST_RUNNER() {
            IR_TEST(ir::state::parse("45:CD"));
        },
        IR_TEST_RUNNER() {
            auto result = ir::state::parse("20:C7B7966A9B29CD3C5F2AC03B91B0B221");
            IR_TEST(result.has_value());

            auto& payload = result.value();
            IR_TEST(payload.type == decode_type_t::MITSUBISHI_AC);

            const uint8_t raw[] {
                0xc7, 0xb7, 0x96, 0x6a,
                0x9b, 0x29, 0xcd, 0x3c,
                0x5f, 0x2a, 0xc0, 0x3b,
                0x91, 0xb0, 0xb2, 0x21};

            IR_TEST(payload.value.size() == sizeof(raw));
            IR_TEST(std::equal(std::begin(payload.value), std::end(payload.value),
                        std::begin(raw)));
        },
        IR_TEST_RUNNER() {
            IR_TEST(!ir::raw::parse("-1:1:500:,200,150,250,50,100,100,150"));
        },
        IR_TEST_RUNNER() {
            auto result = ir::raw::parse("38:1:500:100,200,150,250,50,100,100,150");
            IR_TEST(result.has_value());

            auto& payload = result.value();
            IR_TEST(payload.frequency == 38);
            IR_TEST(payload.series == 1);
            IR_TEST(payload.delay == 500);

            decltype(ir::raw::Payload::time) expected_time {
                100, 200, 150, 250, 50, 100, 100, 150};
            IR_TEST(expected_time == payload.time);
        },
        IR_TEST_RUNNER() {
            const uint16_t raw[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
            IR_TEST(::ir::raw::time::encode(std::begin(raw), std::end(raw)) == F("2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32"));
        }
    }
    IR_TEST_SETUP_END();
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
