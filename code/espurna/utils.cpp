/*

UTILS MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#include "ntp.h"

#include <limits>
#include <random>

// We can only return small values (max 'z' aka 122)
static constexpr uint8_t InvalidByte { 255u };

static uint8_t bin_char2byte(char c) {
    switch (c) {
    case '0'...'1':
        return (c - '0');
    }

    return InvalidByte;
}

static uint8_t oct_char2byte(char c) {
    switch (c) {
    case '0'...'7':
        return (c - '0');
    }

    return InvalidByte;
}

static uint8_t dec_char2byte(char c) {
    switch (c) {
    case '0'...'9':
        return (c - '0');
    }

    return InvalidByte;
}

static uint8_t hex_char2byte(char c) {
    switch (c) {
    case '0'...'9':
        return (c - '0');
    case 'a'...'f':
        return 10 + (c - 'a');
    case 'A'...'F':
        return 10 + (c - 'A');
    }

    return InvalidByte;
}

static ParseUnsignedResult parseUnsignedImpl(espurna::StringView value, int base) {
    auto out = ParseUnsignedResult{
        .ok = false,
        .value = 0,
    };

    using Char2Byte = uint8_t(*)(char);
    Char2Byte char2byte = nullptr;

    switch (base) {
    case 2:
        char2byte = bin_char2byte;
        break;
    case 8:
        char2byte = oct_char2byte;
        break;
    case 10:
        char2byte = dec_char2byte;
        break;
    case 16:
        char2byte = hex_char2byte;
        break;
    }

    if (!char2byte) {
        return out;
    }

    for (auto it = value.begin(); it != value.end(); ++it) {
        const auto digit = char2byte(*it);
        if (digit == InvalidByte) {
            out.ok = false;
            goto err;
        }

        const auto value = out.value;
        out.value = (out.value * uint32_t(base)) + uint32_t(digit);
        // TODO explicitly set the output bit width?
        if (value > out.value) {
            out.ok = false;
            goto err;
        }

        out.ok = true;
    }

err:
    return out;
}

bool tryParseId(espurna::StringView value, size_t limit, size_t& out) {
    using T = std::remove_cvref<decltype(out)>::type;
    static_assert(std::is_same<T, size_t>::value, "");

    if (value.length()) {
        const auto result = parseUnsignedImpl(value, 10);
        if (result.ok && (result.value < limit)) {
            out = result.value;
            return true;
        }
    }

    return false;
}

bool tryParseIdPath(espurna::StringView value, size_t limit, size_t& out) {
    if (value.length()) {
        const auto before_begin = value.begin() - 1;
        for (auto it = value.end() - 1; it != before_begin; --it) {
            if ((*it) == '/') {
                return tryParseId(
                    espurna::StringView(it + 1, value.end()),
                    limit, out);
            }
        }
    }

    return false;
}

String prettyDuration(espurna::duration::Seconds seconds) {
    time_t timestamp = static_cast<time_t>(seconds.count());
    tm spec;
    gmtime_r(&timestamp, &spec);

    char buffer[64];
    sprintf_P(buffer, PSTR("%02dy %02dd %02dh %02dm %02ds"),
        (spec.tm_year - 70), spec.tm_yday, spec.tm_hour,
        spec.tm_min, spec.tm_sec);

    return String(buffer);
}

// -----------------------------------------------------------------------------
// SSL
// -----------------------------------------------------------------------------

bool sslCheckFingerPrint(const char * fingerprint) {
    return (strlen(fingerprint) == 59);
}

bool sslFingerPrintArray(const char * fingerprint, unsigned char * bytearray) {

    // check length (20 2-character digits ':' or ' ' separated => 20*2+19 = 59)
    if (!sslCheckFingerPrint(fingerprint)) return false;

    // walk the fingerprint
    for (unsigned int i=0; i<20; i++) {
        bytearray[i] = strtol(fingerprint + 3*i, NULL, 16);
    }

    return true;

}

bool sslFingerPrintChar(const char * fingerprint, char * destination) {

    // check length (20 2-character digits ':' or ' ' separated => 20*2+19 = 59)
    if (!sslCheckFingerPrint(fingerprint)) return false;

    // copy it
    strncpy(destination, fingerprint, 59);

    // walk the fingerprint replacing ':' for ' '
    for (unsigned char i = 0; i<59; i++) {
        if (destination[i] == ':') destination[i] = ' ';
    }

    return true;

}

// -----------------------------------------------------------------------------
// Helper functions
// -----------------------------------------------------------------------------

// using 'random device' as-is, while most common implementations
// would've used it as a seed for some generator func
// TODO notice that stdlib std::mt19937 struct needs ~2KiB for it's internal
// `result_type state[std::mt19937::state_size]` (ref. sizeof())
uint32_t randomNumber(uint32_t minimum, uint32_t maximum) {
    using Device = espurna::system::RandomDevice;
    using Type = Device::result_type;

    static Device random;
    auto distribution = std::uniform_int_distribution<Type>(minimum, maximum);

    return distribution(random);
}

uint32_t randomNumber() {
    return (espurna::system::RandomDevice{})();
}

double roundTo(double num, unsigned char positions) {
    double multiplier = 1;
    while (positions-- > 0) multiplier *= 10;
    return round(num * multiplier) / multiplier;
}

// ref. https://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
// the machine epsilon has to be scaled to the magnitude of the values used
// and multiplied by the desired precision in ULPs (units in the last place)
// unless the result is subnormal
bool almostEqual(double lhs, double rhs, int ulp) {
    return __builtin_fabs(lhs - rhs) <= std::numeric_limits<double>::epsilon() * __builtin_fabs(lhs + rhs) * ulp
        || __builtin_fabs(lhs - rhs) < std::numeric_limits<double>::min();
}

bool almostEqual(double lhs, double rhs) {
    return almostEqual(lhs, rhs, 3);
}

espurna::StringView stripNewline(espurna::StringView value) {
    if ((value.length() >= 2)
     && (*(value.end() - 1) == '\n')
     && (*(value.end() - 2) == '\r')) {
        value = espurna::StringView(value.begin(), value.end() - 2);
    } else if ((value.length() >= 1) && (*(value.end() - 1) == '\n')) {
        value = espurna::StringView(value.begin(), value.end() - 1);
    }

    return value;
}

bool isNumber(const char* begin, const char* end) {
    bool dot { false };
    bool digit { false };

    for (auto ptr = begin; ptr != end; ++ptr) {
        switch (*ptr) {
        case '\0':
            break;
        case '-':
        case '+':
            if (ptr != begin) {
                return false;
            }
            break;
        case '.':
            if (dot) {
                return false;
            }
            dot = true;
            break;
        case '0' ... '9':
            digit = true;
            break;
        case 'a' ... 'z':
        case 'A' ... 'Z':
            return false;
        }
    }

    return digit;
}

bool isNumber(const String& value) {
    if (value.length()) {
        return isNumber(value.begin(), value.end());
    }

    return false;
}

// ref: lwip2 lwip_strnstr with strnlen
char* strnstr(const char* buffer, const char* token, size_t n) {
  const auto token_len = strnlen_P(token, n);
  if (!token_len) {
      return const_cast<char*>(buffer);
  }

  const auto first = pgm_read_byte(token);
  for (const char* p = buffer; *p && (p + token_len <= buffer + n); p++) {
      if ((*p == first) && (strncmp_P(p, token, token_len) == 0)) {
          return const_cast<char*>(p);
      }
  }

  return nullptr;
}

ParseUnsignedResult parseUnsigned(espurna::StringView value, int base) {
    return parseUnsignedImpl(value, base);
}

static constexpr int base_from_char(char c) {
    return (c == 'b') ? 2 :
        (c == 'o') ? 8 :
        (c == 'x') ? 16 : 0;
}

ParseUnsignedResult parseUnsigned(espurna::StringView value) {
    int base = 10;

    if (value.length() && (value.length() > 2)) {
        const auto from_base = base_from_char(value[1]);
        if ((value[0] == '0') && (from_base != 0)) {
            base = from_base;
            value = espurna::StringView(
                value.begin() + 2, value.end());
        }
    }

    return parseUnsignedImpl(value, base);
}

String formatUnsigned(uint32_t value, int base) {
    constexpr size_t BufferSize { 8 * sizeof(decltype(value)) };

    String result;
    if (base == 2) {
        result += "0b";
    } else if (base == 8) {
        result += "0o";
    } else if (base == 16) {
        result += "0x";
    }

    char buffer[BufferSize + 1] = {0};
    ultoa(value, buffer, base);
    result += buffer;

    return result;
}

namespace {

// From a byte array to an hexa char array ("A220EE...", double the size)
template <typename T>
const uint8_t* hexEncodeImpl(const uint8_t* in_begin, const uint8_t* in_end, T&& callback) {
    static const char base16[] = "0123456789ABCDEF";

    constexpr uint8_t Left { 0xf0 };
    constexpr uint8_t Right { 0xf };
    constexpr uint8_t Shift { 4 };

    auto* in_ptr = in_begin;
    for (; in_ptr != in_end; ++in_ptr) {
        const char buf[2] {
            base16[((*in_ptr) & Left) >> Shift],
            base16[(*in_ptr) & Right]
        };
        if (!callback(buf)) {
            break;
        }
    }

    return in_ptr;
}

} // namespace

char* hexEncode(const uint8_t* in_begin, const uint8_t* in_end, char* out_begin, char* out_end) {
    char* out_ptr { out_begin };

    hexEncodeImpl(in_begin, in_end, [&](const char (&byte)[2]) {
        *(out_ptr) = byte[0];
        ++out_ptr;

        *(out_ptr) = byte[1];
        ++out_ptr;

        return out_ptr != out_end;
    });

    return out_ptr;
}

String hexEncode(const uint8_t* in_begin, const uint8_t* in_end) {
    String out;
    out.reserve(in_end - in_begin);

    hexEncodeImpl(in_begin, in_end, [&](const char (&byte)[2]) {
        out.concat(byte, 2);
        return true;
    });

    return out;
}

size_t hexEncode(const uint8_t* in, size_t in_size, char* out, size_t out_size) {
    if (out_size >= ((in_size * 2) + 1)) {
        char* out_ptr = hexEncode(in, in + in_size, out, out + out_size);
        *out_ptr = '\0';
        ++out_ptr;
        return out_ptr - out;
    }

    return 0;
}

// From an hexa char array ("A220EE...") to a byte array (half the size)
uint8_t* hexDecode(const char* in_begin, const char* in_end, uint8_t* out_begin, uint8_t* out_end) {
    constexpr uint8_t Shift { 4 };

    const char* in_ptr { in_begin };
    uint8_t* out_ptr { out_begin };
    while ((in_ptr != in_end) && (out_ptr != out_end)) {
        uint8_t lhs = hex_char2byte(*in_ptr);
        if (lhs == InvalidByte) {
            break;
        }
        ++in_ptr;

        uint8_t rhs = hex_char2byte(*in_ptr);
        if (rhs == InvalidByte) {
            break;
        }
        ++in_ptr;

        (*out_ptr) = (lhs << Shift) | rhs;
        ++out_ptr;
    }

    return out_ptr;
}

size_t hexDecode(const char* in, size_t in_size, uint8_t* out, size_t out_size) {
    if ((in_size & 1) || (out_size < (in_size / 2))) {
        return 0;
    }

    uint8_t* out_ptr { hexDecode(in, in + in_size, out, out + out_size) };
    return out_ptr - out;
}

size_t consumeAvailable(Stream& stream) {
    const auto result = stream.available();
    if (result <= 0) {
        return 0;
    }

    const auto available = static_cast<size_t>(result);
    size_t size = 0;
    uint8_t buf[64];
    do {
        const auto chunk = std::min(available, std::size(buf));
        stream.readBytes(&buf[0], chunk);
        size += chunk;
    } while (size != available);

    return size;
}
