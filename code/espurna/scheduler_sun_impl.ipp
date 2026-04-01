/*

Part of SCHEDULER MODULE

Copyright (C) 2017 by faina09
Adapted by Xose Pérez <xose dot perez at gmail dot com>

Copyright (C) 2019-2024 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

Sunrise & Sundown algorithm implementation by Nathan Osman
ref. https://github.com/nathan-osman/go-sunrise

Altitude correction by Jérémy Rabasco
ref. https://github.com/nathan-osman/go-sunrise/pull/11

Original licence follows:

The MIT License (MIT)

Copyright (c) 2017 Nathan Osman

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

std::remainder implementation for Cores <3.x.x

Orinal copyright info follows:

Copyright 2010 The Go Authors. All rights reserved.
Use of this source code is governed by a BSD-style
license that can be found in the LICENSE file.

The original C code and the comment below are from
FreeBSD's /usr/src/lib/msun/src/e_remainder.c and came
with this notice. The go code is a simplified version of
the original C.

====================================================
Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.

Developed at SunPro, a Sun Microsystems, Inc. business.
Permission to use, copy, modify, and distribute this
software is freely granted, provided that this notice
is preserved.
====================================================

*/

#pragma once

#include "libs/fs_math.h"
#include "datetime.h"

#include <cmath>
#include <ctime>
#include <limits>

#include "scheduler_common.ipp"

#if defined(ESP8266) && !defined(HOST_MOCK)
#ifndef SCHEDULER_SUN_EMBEDDED_CMATH
#define SCHEDULER_SUN_EMBEDDED_CMATH 1
#endif
#endif

namespace espurna {
namespace scheduler {
namespace {

namespace sun {

// Result is unset by default, should check if it is >0
struct SunriseSunset {
    datetime::Clock::time_point sunrise { event::DefaultSeconds };
    datetime::Clock::time_point sunset { event::DefaultSeconds };
};

// Details provided by the user
struct Location {
    double latitude;
    double longitude;
    double altitude;
};

constexpr double Pi { M_PI };
constexpr double Pi2 { M_PI_2 };

constexpr double NaN { std::numeric_limits<double>::quiet_NaN() };
constexpr double PositiveNaN { NaN };
constexpr double NegativeNaN { -NaN };

namespace math {

#if SCHEDULER_SUN_EMBEDDED_CMATH
// ref. https://dsp.stackexchange.com/a/17276 by Robert Bristow-Johnson
// effectively a float algorithm, but more than good enough for this case
float atan(float x) {
    float offset = 0.0;

    float xSquared = x * x;
    float xPower = xSquared;

    float accumulator = 1.0;
    accumulator += 0.33288950512027 * xPower;

    xPower *= xSquared;
    accumulator += -0.08467922817644 * xPower;

    xPower *= xSquared;
    accumulator += 0.03252232640125 * xPower;

    xPower *= xSquared;
    accumulator += -0.00749305860992 * xPower;

    return offset + x / accumulator;
}

// both arcsine and arccosine implemented through atan. adding some exceptions
// based on Go impl to remove special 'x' handling above. returns early when input is out-of-range
// note that newlib obviously implements both, should the need arise just swap these with std::...
double asin(double x) {
    if (x == 0) {
        return x;
    }

    bool sign { false };
    if (x < 0) {
        sign = true;
        x = -x;
    }

    if (x > 1) {
        return NaN;
    }

    double out = std::sqrt(1.0 - x * x);
    if (x > 0.7) {
        out = Pi2 - atan(out / x);
    } else {
        out = atan(x / out);
    }

    if (sign) {
        out = -out;
    }

    return out;
}

double acos(double x) {
    return Pi2 - asin(x);
}

double cos(double x) {
    return fs_cos(x);
}

double sqrt(double x) {
    return fs_sqrt(x);
}

double fmod(double x, double y) {
    return fs_fmod(x, y);
}

#else

// Everything from stdlib calc below needs

double asin(double x) {
    return std::asin(x);
}

double acos(double x) {
    return std::acos(x);
}

double cos(double x) {
    return std::cos(x);
}

double sqrt(double x) {
    return std::sqrt(x);
}

double fmod(double x, double y) {
    return std::fmod(x, y);
}

#endif

double remainder(double x, double y) {
// need local implementation b/c __ieee754_remainder missing from libc shipped with 2.7.4
#if __cplusplus < 201411L
    constexpr double TinyValue { 4.45014771701440276618e-308 }; // 0x0020000000000000
    constexpr double HalfMax { std::numeric_limits<double>::max() / 2.0 };

    if (std::isnan(x) || std::isnan(y) || std::isinf(x) || y == 0.0) {
        return NaN;
    }

    if (std::isinf(y)) {
        return x;
    }

    bool sign { false };
    if (x < 0) {
        sign = true;
        x = -x;
    }

    if (y < 0) {
        y = -y;
    }

    if (x == y) {
        return sign ? -0.0 : 0.0;
    }

    if (y <= HalfMax) {
        x = fmod(x, y + y); // now x < 2y
    }

    if (y < TinyValue) {
        if ((x + x) > y) {
            x -= y;
            if ((x + x) >= y) {
                x -= y;
            }
        }
    } else {
        auto half = y / 2.0;
        if (x > half) {
            x -= y;
            if (x >= half) {
                x -= y;
            }
        }
    }

    if (sign) {
        x = -x;
    }

    return x;
#else
    return std::remainder(x, y);
#endif
}

double sin(double x) {
    return std::sin(x);
}

} // namespace math

using math::acos;
using math::asin;
using math::cos;
using math::fmod;
using math::remainder;
using math::sin;
using math::sqrt;

// most calculations done with radians
constexpr double Degree { Pi / 180.0L };

// readability func missing from original implementation
constexpr double to_radians(double degrees) {
    return degrees * Degree;
}

// readability func missing from original implementation
constexpr double to_degrees(double radians) {
    return radians / Degree;
}

// julian calendar consts
constexpr double JD2000 { 2451545.0 }; // aka 2000-01-01 12:00 (also note it is not .25 / 18:00)
constexpr double UnixJD { 2440587.5 }; // aka 1970-01-01 00:00

// TODO chrono? ::day, ::year ratios below
// TODO more strict types for time_point / durations?
constexpr double SecondsInDay { 86400.0 };
constexpr double Y { 36525.0 };

constexpr double posixToJulianDay(time_t timestamp) {
    return double(timestamp) / SecondsInDay + UnixJD;
}

constexpr time_t julianDayToPosix(double d) {
    return time_t(int64_t((d - UnixJD) * SecondsInDay));
}

constexpr datetime::Seconds julianDayToDatetimeSeconds(double d) {
    return datetime::Seconds{ julianDayToPosix(d) };
}

constexpr datetime::Clock::time_point julianDayToTimePoint(double d) {
    return datetime::Clock::time_point(julianDayToDatetimeSeconds(d));
}

// Argument of periapsis for the earth on the given Julian day
constexpr double argument_of_perihelion(double d) {
	return 102.93005 + 0.3179526 * (d - JD2000) / Y;
}

// Angular distance of the earth along the ecliptic
double ecliptic_longitude(double solarAnomaly, double equationOfCenter, double d) {
	return fmod(solarAnomaly + equationOfCenter + 180.0 + argument_of_perihelion(d), 360.0);
}

// Julian data for the local true solar transit.
double solar_transit(double d, double solarAnomaly, double eclipticLongitude) {
    double equationOfTime;
    equationOfTime = 0.0053 * std::sin(to_radians(solarAnomaly));
    equationOfTime -= 0.0069 * std::sin(to_radians(2.0 * eclipticLongitude));

    return d + equationOfTime;
}

// Angle of the sun in degrees relative to the earth for the specified Julian day
double solar_mean_anomaly(double d) {
    double out = remainder(357.5291 + 0.98560028 * (d - JD2000), 360.0);
    if (out < 0.0) {
        out += 360.0;
    }

    return out;
}

// Angular difference between the position of the earth in its elliptical orbit
// and the position it would occupy in a circular orbit for the given mean anomaly
double equation_of_center(double solarAnomaly) {
    const double inRadians = to_radians(solarAnomaly);

    const double x = sin(inRadians);
    const double x2 = sin(2.0 * inRadians);
    const double x3 = sin(3.0 * inRadians);

    return 1.9148 * x + 0.0200 * x2 + 0.0003 * x3;
}

// One of the two angles required to locate a point on the celestial sphere in the
// equatorial coordinate system. The ecliptic longitude parameter must be in degrees.
double ecliptic_longitude_declination(double eclipticLongitude) {
	return to_degrees(asin(sin(to_radians(eclipticLongitude)) * 0.39779));
}

double altitude_radian_correctiion(double altitude) {
	return ((-2.076 * sqrt(altitude) / 60.0) / 360.0) * Pi * 2.0;
}

// Second of the two angles required to locate a point on the celestial sphere in the
// equatorial coordinate system while correcting for the observer's altitude (in meters)
// nb. nan return values replace corresponding float64 max() and min().
// the only use in code is right here (or, in case this func is called elsewhere)
double hour_angle_altitude(double latitude, double declination, double altitude) {
    double latitudeRad = to_radians(latitude);
	double declinationRad = to_radians(declination);

    // constexpr double numRad = to_radians(-0.833);
	constexpr double numRad = -0.0145385927;

    double altitude_correction = altitude_radian_correctiion(altitude);
	double numerator = sin(numRad + altitude_correction)
        - sin(latitudeRad) * sin(declinationRad);
	double denominator = cos(latitudeRad) * cos(declinationRad);

    double division = numerator / denominator;

	// Sun never rises
	if (division > 1.0) {
		return PositiveNaN;
	}

	// Sun never sets
	if (division < -1.0) {
		return NegativeNaN;
	}

	return to_degrees(acos(division));
}

// Second of the two angles required to locate a point on the celestial sphere in the equatorial coordinate system.
inline double hour_angle(double latitude, double declination) {
	return hour_angle_altitude(latitude, declination, 0);
}

// MeanSolarNoon calculates the time at which the sun is at its highest
// altitude. The returned time is in Julian days.
double mean_solar_noon(double longitude, datetime::Days days) {
    // adjust for middle-of-the-day, optimistic case that handles UTC-12..UTC+12
    auto seconds = std::chrono::duration_cast<datetime::Seconds>(days);
    seconds += std::chrono::hours(12);

	return posixToJulianDay(seconds.count()) - longitude / 360.0;
}

// When the sun will rise and when it will set on the given day at the specified location and altitude.
// Returns negative numbers when sun does not rise or set.
SunriseSunset sunrise_sunset(const Location& location, datetime::Clock::time_point time_point) {
    // TODO separate struct, debug log for intermediate calculations?
    const auto d = mean_solar_noon(location.longitude,
            datetime::floor<datetime::Days>(time_point.time_since_epoch()));
	const auto solarAnomaly = solar_mean_anomaly(d);
	const auto equationOfCenter  = equation_of_center(solarAnomaly);
    const auto eclipticLongitude = ecliptic_longitude(solarAnomaly, equationOfCenter, d);
	const auto solarTransit = solar_transit(d, solarAnomaly, eclipticLongitude);
	const auto declination = ecliptic_longitude_declination(eclipticLongitude);
	const auto hourAngle = hour_angle_altitude(location.latitude, declination, location.altitude);

    SunriseSunset out;

	// Special case for no sunrise, no sunset
	if (std::isnan(std::abs(hourAngle))) {
		return out;
	}

	const auto frac = hourAngle / 360.0;
	const auto sunrise = solarTransit - frac;
	const auto sunset = solarTransit + frac;

    out.sunrise = julianDayToTimePoint(sunrise);
    out.sunset = julianDayToTimePoint(sunset);

    return out;
}

} // namespace sun

} // namespace
} // namespace scheduler
} // namespace espurna
