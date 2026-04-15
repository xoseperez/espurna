/*

Part of SCHEDULER MODULE

Copyright (C) 2017 by faina09
Adapted by Xose Pérez <xose dot perez at gmail dot com>

Copyright (C) 2019-2024 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include "bits.h"
#include "datetime.h"
#include "types.h"
#include "utils.h"

#include "scheduler_common.ipp"
#include "scheduler_sun_impl.ipp"

namespace espurna {
namespace scheduler {
namespace {

namespace sun {

// Generic event fields plus date & time pair, to be used in scheduler matching code
struct EventMatch : public Event {
    datetime::Date date;
    TimeMatch time;
};

// Sunrise & Sunset state for the runtime
struct Match {
    EventMatch sunrise;
    EventMatch sunset;
};

tm make_utc_date_time(datetime::Seconds seconds) {
    tm out{};

    time_t timestamp{ seconds.count() };
    gmtime_r(&timestamp, &out);

    return out;
}

TimeMatch make_time_match(const tm& date_time) {
    TimeMatch out;

    out.hour[date_time.tm_hour] = true;
    out.minute[date_time.tm_min] = true;
    out.flags = FlagUtc;

    return out;
}

DateMatch make_date_match(const EventMatch& match) {
    DateMatch out;

    out.year = match.date.year;
    out.month[match.date.month - 1] = true;
    out.day[match.date.day] = true;

    return out;
}

void update_event_match_date_time(EventMatch& match, datetime::Clock::time_point time_point) {
    const auto duration = time_point.time_since_epoch();
    const auto date_time = make_utc_date_time(duration);
    match.date = datetime::make_date(date_time);
    match.time = make_time_match(date_time);
}

bool update_event_match(EventMatch& match, datetime::Clock::time_point time_point) {
    if (match.next != time_point) {
        if (event::is_valid(match.next)) {
            match.last = match.next;
        }

        if (event::is_valid(time_point)) {
            update_event_match_date_time(match, time_point);
            match.next = time_point;
        } else {
            match.next = event::DefaultTimePoint;
        }

        return true;
    }

    return false;
}

// updates match with the sunrise & sunset from the default algorithm outputs
bool update(Match& match, const Location& location, datetime::Clock::time_point time_point) {
    const auto result = sun::sunrise_sunset(location, time_point);

    const auto sunrise_updated = update_event_match(match.sunrise, result.sunrise);
    const auto sunset_updated = update_event_match(match.sunset, result.sunset);

    return sunrise_updated || sunset_updated;
}

// updates match with the sunrise & sunset compared against the givent time point
// should result in time points only in the past or only in the future (ref. T::check)
template <typename T, int DaysNum = T::negative_days ? -1 : 1>
bool update(Match& match, const Location& location, datetime::Clock::time_point time_point) {
    auto result = sun::sunrise_sunset(location, time_point);

    const auto reset_sunrise =
        !event::is_valid(result.sunrise) || T::check(time_point, result.sunrise);

    const auto reset_sunset =
        !event::is_valid(result.sunset) || T::check(time_point, result.sunset);

    if (reset_sunrise || reset_sunset) {
        tm tmp;
        datetime::delta_utc(tmp, time_point.time_since_epoch(), datetime::Days{ DaysNum });

        const auto other_time_point = datetime::make_time_point(datetime::to_seconds(tmp));
        const auto other = sun::sunrise_sunset(location, other_time_point);

        if (reset_sunrise && event::is_valid(other.sunrise)) {
            result.sunrise = other.sunrise;
        }

        if (reset_sunset && event::is_valid(other.sunset)) {
            result.sunset = other.sunset;
        }
    }

    const auto sunrise_updated = update_event_match(match.sunrise, result.sunrise);
    const auto sunset_updated = update_event_match(match.sunset, result.sunset);

    return sunrise_updated || sunset_updated;
}

template <typename T>
bool update(Match& match, const Location& location, time_t timestamp, const tm& today, T&& compare) {
    return update(match, location, datetime::make_time_point(timestamp), today, std::forward<T>(compare));
}

// check() needs current or future events, discard timestamps in the past
// round to minutes when doing so as well, since std::greater<> would compare seconds
// note that time point values usually go as lhs, causing 'or-equals' to include sun{rise,set} point
struct CompareForward {
    static constexpr bool negative_days = false;
    static constexpr bool check(const event::time_point& lhs, const event::time_point& rhs) {
        return event::greater(lhs, rhs);
    }
};

constexpr bool CompareForward::negative_days;

template
bool update<CompareForward>(Match&, const Location&, datetime::Clock::time_point);

// relative events need current or past time point
struct CompareBackward {
    static constexpr bool negative_days = true;
    static constexpr bool check(const event::time_point& lhs, const event::time_point& rhs) {
        return event::less(lhs, rhs);
    }
};

constexpr bool CompareBackward::negative_days;

template
bool update<CompareBackward>(Match&, const Location&, datetime::Clock::time_point);

// try to schedule update at the earliest. making sure it happens *after* current time point
// fallback to user provided value otherwise (as callback for postponed calc)
template <typename T>
datetime::Clock::time_point update_next_time_point(
        std::initializer_list<datetime::Clock::time_point> candidates,
        datetime::Clock::time_point time_point,
        T&& fallback)
{
    auto out = event::DefaultTimePoint;

    for (const auto& value : candidates) {
        if (event::is_valid(value) && (value > time_point)) {
            if (event::is_valid(out)) {
                out = std::min(out, value);
            } else {
                out = value;
            }
        }
    }

    if (!event::is_valid(out)) {
        out = fallback();
    }

    return out;
}

template <typename T>
datetime::Clock::time_point update_next_time_point(
        const Match& match,
        datetime::Clock::time_point time_point,
        T&& fallback)
{
        return update_next_time_point(
                {match.sunrise.next, match.sunset.next},
                time_point,
                std::forward<T>(fallback));
}

} // namespace
} // namespace sun
} // namespace scheduler
} // namespace espurna
