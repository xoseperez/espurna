/*

Part of SENSOR MODULE

*/

#include "sensor.h"
#include "libs/fs_math.h"

namespace espurna {
namespace sensor {

// Generic storage. Most of the time we init this on boot with both members or start at 0 and increment with watt-second

Energy::Energy(Energy::Pair pair) :
    _kwh(pair.kwh),
    _ws(pair.ws)
{}

Energy::Energy(WattSeconds ws) {
    _ws.value = ws.value;
    while (_ws.value >= WattSecondsMax) {
        _ws.value -= WattSecondsMax;
        ++_kwh.value;
    }
}

Energy::Energy(WattHours other) :
    Energy(static_cast<double>(other.value) / 1000.0)
{}

Energy::Energy(double kwh) {
    double lhs;
    double rhs = fs_modf(kwh, &lhs);

    _kwh.value = lhs;
    _ws.value = rhs * static_cast<double>(KilowattHours::Ratio::num);
}

Energy& Energy::operator+=(WattSeconds other) {
    return *this += Energy(other);
}

Energy Energy::operator+(WattSeconds other) {
    Energy result(*this);
    result += other;

    return result;
}

Energy& Energy::operator+=(const Energy& other) {
    _kwh.value += other._kwh.value;

    const auto left = WattSecondsMax - _ws.value;
    if (other._ws.value >= left) {
        _kwh.value += 1;
        _ws.value = (other._ws.value - left);
    } else {
        _ws.value += other._ws.value;
    }

    return *this;
}

Energy::operator bool() const {
    return (_kwh.value > 0) && (_ws.value > 0);
}

WattSeconds Energy::asWattSeconds() const {
    using Type = WattSeconds::Type;

    static constexpr auto TypeMax = std::numeric_limits<Type>::max();
    static constexpr Type KwhMax { TypeMax / WattSecondsMax };

    auto kwh = _kwh.value;
    while (kwh >= KwhMax) {
        kwh -= KwhMax;
    }

    WattSeconds out;
    out.value += _ws.value;
    out.value += kwh * WattSecondsMax;

    return out;
}

double Energy::asDouble() const {
    return static_cast<double>(_kwh.value)
        + static_cast<double>(_ws.value)
        / static_cast<double>(WattSecondsMax);
}

String Energy::asString() const {
    String out;

    // Value without `+` is treated as just `<kWh>`
    out += String(_kwh.value, 10);
    if (_ws.value) {
        out += '+';
        out += String(_ws.value, 10);
    }

    return out;
}

void Energy::reset() {
    *this = Energy{};
}

} // namespace sensor
} // namespace espurna
