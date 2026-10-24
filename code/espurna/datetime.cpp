/*

DATETIME MODULE

Copyright (C) 2024 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "datetime.h"

namespace espurna {
namespace datetime {
namespace {

constexpr Seconds start_of_day_delta(Seconds seconds, Days delta) {
    return seconds - (seconds - std::chrono::duration_cast<Days>(seconds)) + delta;
}

time_t delta_local_impl(tm& out, Days days) {
    out.tm_mday += days.count();
    out.tm_hour = 0;
    out.tm_min = 0;
    out.tm_sec = 0;

    out.tm_isdst = -1;

    return mktime(&out);
}

time_t delta_utc_impl(tm& out, Seconds seconds, Days days) {
    const auto timestamp = start_of_day_delta(seconds, days);

    time_t tmp { timestamp.count() };
    gmtime_r(&tmp, &out);

    return tmp;
}

String tz_offset_string(Seconds offset) {
    String out;

    auto hours = std::chrono::duration_cast<Hours>(offset);
    offset -= hours;

    out = (offset >= offset.zero()) ? '+' : '-';
    if (hours < Hours{9}) {
        out += '0';
    }

    out += String(hours.count(), 10);
    out += ':';

    auto minutes = std::chrono::duration_cast<Minutes>(offset);
    if (minutes < Minutes{9}) {
        out += '0';
    }

    out += String(minutes.count(), 10);

    return out;
}

} // namespace

// In case of newlib, there is no `tm::tm_gmtoff` and this offset has to be calculated manually.
// Although there is sort-of standard POSIX `_timezone` global, it only tracks non-DST time.
Seconds tz_offset(const Context& ctx) {
    return to_seconds(ctx.local) - to_seconds(ctx.utc);
}

// Days since 1970/01/01.
// Proposition 6.2 of Neri and Schneider,
// "Euclidean Affine Functions and Applications to Calendar Algorithms".
// https://arxiv.org/abs/2102.06959
Days to_days(const Date& date) noexcept {
    auto constexpr __z2    = static_cast<uint32_t>(-1468000);
    auto constexpr __r2_e3 = static_cast<uint32_t>(536895458);

    const auto __y1 = static_cast<uint32_t>(date.year) - __z2;
    const auto __m1 = static_cast<uint32_t>(
            static_cast<unsigned>(date.month));
    const auto __d1 = static_cast<uint32_t>(
            static_cast<unsigned>(date.day));

    const auto __j  = static_cast<uint32_t>(__m1 < 3);
    const auto __y0 = __y1 - __j;
    const auto __m0 = __j ? __m1 + 12 : __m1;
    const auto __d0 = __d1 - 1;

    const auto __q1 = __y0 / 100;
    const auto __yc = 1461 * __y0 / 4 - __q1 + __q1 / 4;
    const auto __mc = (979 *__m0 - 2919) / 32;
    const auto __dc = __d0;

    return Days{
        static_cast<int32_t>(__yc + __mc + __dc - __r2_e3)};
}

Days to_days(const tm& t) noexcept {
    return to_days(make_date(t));
}

// Construct from days since 1970/01/01.
// Proposition 6.3 of Neri and Schneider,
// "Euclidean Affine Functions and Applications to Calendar Algorithms".
// https://arxiv.org/abs/2102.06959
Date from_days(Days days) noexcept {
    constexpr auto __z2    = static_cast<uint32_t>(-1468000);
    constexpr auto __r2_e3 = static_cast<uint32_t>(536895458);

    const auto __r0 = static_cast<uint32_t>(days.count()) + __r2_e3;

    const auto __n1 = 4 * __r0 + 3;
    const auto __q1 = __n1 / 146097;
    const auto __r1 = __n1 % 146097 / 4;

    constexpr auto __p32 = static_cast<uint64_t>(1) << 32;
    const auto __n2 = 4 * __r1 + 3;
    const auto __u2 = static_cast<uint64_t>(2939745) * __n2;
    const auto __q2 = static_cast<uint32_t>(__u2 / __p32);
    const auto __r2 = static_cast<uint32_t>(__u2 % __p32) / 2939745 / 4;

    constexpr auto __p16 = static_cast<uint32_t>(1) << 16;
    const auto __n3 = 2141 * __r2 + 197913;
    const auto __q3 = __n3 / __p16;
    const auto __r3 = __n3 % __p16 / 2141;

    const auto __y0 = 100 * __q1 + __q2;
    const auto __m0 = __q3;
    const auto __d0 = __r3;

    const auto __j  = __r2 >= 306;
    const auto __y1 = __y0 + __j;
    const auto __m1 = __j ? __m0 - 12 : __m0;
    const auto __d1 = __d0 + 1;

    return Date{
        .year = static_cast<int>(__y1 + __z2),
        .month = static_cast<uint8_t>(__m1),
        .day = static_cast<uint8_t>(__d1)};
}

// Seconds since 1970/01/01.
// Generalize convertion func to be used instead of mktime, where only the return value matters
Seconds to_seconds(const Date& date, const HhMmSs& hh_mm_ss) noexcept {
    auto out = std::chrono::duration_cast<Seconds>(to_days(date));

    out += Hours(hh_mm_ss.hours);
    out += Minutes(hh_mm_ss.minutes);
    out += Seconds(hh_mm_ss.seconds);

    return out;
}

tm DateHhMmSs::c_value() const noexcept {
    tm out{};
    out.tm_isdst = -1;

    out.tm_year = year - 1900;
    out.tm_mon = month - 1;
    out.tm_mday = day;

    out.tm_hour = hours;
    out.tm_min = minutes;
    out.tm_sec = seconds;

    return out;
}

Seconds to_seconds(const tm& t) noexcept {
    return to_seconds(make_date(t), make_hh_mm_ss(t));
}

Seconds to_seconds(const DateHhMmSs& datetime, bool utc) noexcept {
    if (utc) {
        return to_seconds(
            make_date(datetime),
            make_hh_mm_ss(datetime));
    }

    auto c_value = datetime.c_value();
    return Seconds{ mktime(&c_value) };
}

Clock::time_point make_time_point(const DateHhMmSs& datetime, bool utc) noexcept {
    return Clock::time_point(to_seconds(datetime, utc));
}

Context make_context(Seconds seconds) {
    return make_context(seconds.count());
}

Context make_context(Clock::time_point time_point) {
    return make_context(time_point.time_since_epoch());
}

Context make_context(time_t timestamp) {
    Context out;
    out.timestamp = timestamp;

    localtime_r(&timestamp, &out.local);
    gmtime_r(&timestamp, &out.utc);

    return out;
}

time_t delta_local(tm& out, Days days) {
    return delta_local_impl(out, days);
}

time_t delta_utc(tm& out, Seconds seconds, Days days) {
    return delta_utc_impl(out, seconds, days);
}

Context delta(const datetime::Context& ctx, Days days) {
    Context out = ctx;

    const auto local = delta_local_impl(out.local, days);
    if (local < 0) {
        out.timestamp = -1;
        return out;
    }

    out.timestamp = delta_utc_impl(out.utc,
        Seconds{ out.timestamp }, days);

    return out;
}

// iso8601 time string, without timezone info
String format(const tm& t) {
    char buffer[32];
    snprintf_P(buffer, sizeof(buffer),
        PSTR("%04d-%02d-%02dT%02d:%02d:%02d"),
        std::clamp(t.tm_year + 1900, 1970, 9999),
        std::clamp(t.tm_mon + 1, 1, 12),
        std::clamp(t.tm_mday, 1, 31),
        std::clamp(t.tm_hour, 0, 24),
        std::clamp(t.tm_min, 0, 60),
        std::clamp(t.tm_sec, 0, 60));

    return String(buffer);
}

// retrieve local time struct from timestamp and format it
String format_local(time_t timestamp) {
    tm tmp;
    localtime_r(&timestamp, &tmp);
    return format(tmp);
}

String format_local(Clock::time_point time_point) {
    return format_local(time_point.time_since_epoch().count());
}

// retrieve utc time struct from timestamp and format it
String format_utc(time_t timestamp) {
    tm tmp;
    gmtime_r(&timestamp, &tmp);
    return format(tmp) + 'Z';
}

String format_utc(Clock::time_point time_point) {
    return format_utc(time_point.time_since_epoch().count());
}

// time string plus offset from UTC
// could be the same as format_utc when offset is zero
String format_local_tz(const Context& ctx) {
    const auto offset = tz_offset(ctx);
    if (offset == offset.zero()) {
        return format(ctx.local) + 'Z';
    }

    return format(ctx.local) + tz_offset_string(offset);
}

String format_local_tz(time_t timestamp) {
    return format_local_tz(make_context(timestamp));
}

String format_local_tz(Clock::time_point time_point) {
    return format_local_tz(make_context(time_point));
}

// aka "Zulu time" or "Zulu meridian", shorter version of +00:00
String format_utc_tz(const tm& t) {
    return format(t) + 'Z';
}

String format_utc_tz(const Context& ctx) {
    return format_utc_tz(ctx.utc);
}

} // namespace datetime
} // namespace espurna

