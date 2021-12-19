/*

Part of the RELAY MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <cstdint>

namespace espurna {
namespace relay {
namespace pulse {
namespace {

struct Result {
    Result() = default;
    explicit Result(Duration duration) :
        _duration(duration)
    {}

    template <typename T>
    Result& operator+=(T duration) {
        _result = true;
        _duration += std::chrono::duration_cast<Duration>(duration);
        return *this;
    }

    explicit operator bool() const {
        return _result;
    }

    void reset() {
        _result = false;
        _duration = Duration::min();
    }

    Duration duration() const {
        return _duration;
    }

    Duration::rep count() const {
        return _duration.count();
    }

private:
    bool _result { false };
    Duration _duration { Duration::min() };
};

namespace internal {

enum class Type {
    Unknown,
    Seconds,
    Minutes,
    Hours
};

bool validNextType(Type lhs, Type rhs) {
    switch (lhs) {
    case Type::Unknown:
        return true;
    case Type::Hours:
        return (rhs == Type::Minutes) || (rhs == Type::Seconds);
    case Type::Minutes:
        return (rhs == Type::Seconds);
    case Type::Seconds:
        break;
    }

    return false;
}

Result parse(const char* begin, const char* end) {
    Result out;

    String token;
    Type last { Type::Unknown };
    Type type { Type::Unknown };

    const char* ptr { begin };
    if (!begin || !end || (begin == end)) {
        goto output;
    }

loop:
    while (ptr != end) {
        switch (*ptr) {
        case '\0':
            if (last == Type::Unknown) {
                goto update_floating;
            }
            goto output;
        case '0'...'9':
            token += (*ptr);
            ++ptr;
            break;
        case 'h':
            if (validNextType(last, Type::Hours)) {
                type = Type::Hours;
                goto update_decimal;
            }
            goto reset;
        case 'm':
            if (validNextType(last, Type::Minutes)) {
                type = Type::Minutes;
                goto update_decimal;
            }
            goto reset;
        case 's':
            if (validNextType(last, Type::Seconds)) {
                type = Type::Seconds;
                goto update_decimal;
            }
            goto reset;
        case ',':
        case '.':
            if (out) {
                goto reset;
            }
            goto read_floating;
        }
    }

    if (token.length()) {
        goto update_floating;
    }

    goto output;

update_floating:
    {
        char* endp { nullptr };
        auto value = strtod(token.c_str(), &endp);
        if (endp && (endp != token.c_str()) && endp[0] == '\0') {
            out += Seconds(value);
            goto output;
        }

        goto reset;
    }

update_decimal:
    last = type;
    ++ptr;

    if (type != Type::Unknown) {
        char* endp { nullptr };
        uint32_t value = strtoul(token.c_str(), &endp, 10);

        if (endp && (endp != token.c_str()) && endp[0] == '\0') {
            switch (type) {
            case Type::Hours: {
                out += ::espurna::duration::Hours { value };
                break;
            }
            case Type::Minutes: {
                out += ::espurna::duration::Minutes { value };
                break;
            }
            case Type::Seconds: {
                out += ::espurna::duration::Seconds { value };
                break;
            }
            case Type::Unknown:
                goto reset;
            }

            type = Type::Unknown;
            token = "";

            goto loop;
        }
    }

    goto reset;

read_floating:
    switch (*ptr) {
    case ',':
    case '.':
        token += '.';
        ++ptr;
        break;
    default:
        goto reset;
    }

    while (ptr != end) {
        switch (*ptr) {
        case '\0':
            goto update_floating;
        case '0'...'9':
            token += (*ptr);
            break;
        case 'e':
        case 'E':
            token += (*ptr);
            ++ptr;

            while (ptr != end) {
                switch (*ptr) {
                case '\0':
                    goto reset;
                case '-':
                case '+':
                    token += (*ptr);
                    ++ptr;
                    goto read_floating_exponent;
                case '0'...'9':
                    goto read_floating_exponent;
                }
            }

            goto reset;
        case ',':
        case '.':
            goto reset;
        }

        ++ptr;
    }

    goto update_floating;

read_floating_exponent:
    while (ptr != end) {
        switch (*ptr) {
        case '0'...'9':
            token += *(ptr);
            ++ptr;
            break;
        }

        goto reset;
    }

    goto update_floating;

reset:
    out.reset();

output:
    return out;
}

} // namespace internal

Result parse(const String& value) {
    return internal::parse(value.begin(), value.end());
}

Result parse(const char* value) {
    return internal::parse(value, value + strlen(value));
}

#if 0
void test() {
    auto report = [](const String& value) {
        const auto result = parse(value);
        DEBUG_MSG_P(PSTR(":\"%s\" is #%c -> %u (ms)\n"),
            value.c_str(),
            static_cast<bool>(result) ? 't' : 'f',
            result.count());
    };

    report("5h");
    report("7h6h");
    report("15m");
    report("19m1h");
    report("12345");
    report("1.5");
}
#endif

} // namespace
} // namespace pulse
} // namespace relay
} // namespace espurna
