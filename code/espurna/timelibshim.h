// Shim original TimeLib functions

#pragma once

constexpr time_t daysPerWeek = 7;

constexpr time_t secondsPerMinute = 60;
constexpr time_t secondsPerHour = 3600;
constexpr time_t secondsPerDay = secondsPerHour * 24;
constexpr time_t secondsPerWeek = daysPerWeek * secondsPerDay;

constexpr time_t secondsPerYear = secondsPerWeek * 52;
constexpr time_t secondsY2K = 946684800; // the time at the start of y2k

// wall clock values
constexpr const time_t numberOfSeconds(uint32_t ts) {
    return (ts % secondsPerMinute);
}  

constexpr const time_t numberOfMinutes(uint32_t ts) {
    return ((ts / secondsPerMinute) % secondsPerMinute);
}

constexpr const time_t numberOfHours(uint32_t ts) {
    return ((ts % secondsPerDay) / secondsPerHour);
}

// week starts with sunday as number 1, monday as 2 etc.
constexpr const time_t dayOfWeek(time_t ts) {
    return ((ts / secondsPerDay + 4) % daysPerWeek) + 1;
}

// the number of days since 0 (Jan 1 1970 in case of time_t values)
constexpr const time_t elapsedDays(uint32_t ts) {
    return (ts / secondsPerDay);
}

// the number of seconds since last midnight 
constexpr const time_t elapsedSecsToday(uint32_t ts) {
    return (ts % secondsPerDay);
}

// The following methods are used in calculating alarms and assume the clock is set to a date later than Jan 1 1971
// Always set the correct time before settting alarms

// time at the start of the given day
constexpr const time_t previousMidnight(time_t ts) {
    return ((ts / secondsPerDay) * secondsPerDay);
}

// time at the end of the given day 
constexpr const time_t nextMidnight(time_t ts) {
    return previousMidnight(ts) + secondsPerDay;
}

// note that week starts on day 1
constexpr const time_t elapsedSecsThisWeek(uint32_t ts) {
    return elapsedSecsToday(ts) + ((dayOfWeek(ts) - 1) * secondsPerDay);
}

// time at the start of the week for the given time
constexpr const time_t previousSunday(time_t ts) {
    return ts - elapsedSecsThisWeek(ts);
}

// time at the end of the week for the given time
constexpr const time_t nextSunday(time_t ts) {
    return previousSunday(ts) + secondsPerWeek;
}

        year(t), month(t), day(t), hour(t), minute(t), second(t)

static time_t _ntp_ts = 0;
static tm _ntp_tm;

void _ntpTmCache(time_t ts) {
    if (_ntp_tm != ts) {
        _ntp_ts = ts;
        localtime_r(_ntp_ts, _ntp_tm);
    }
}

int hour(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm.tm_hour;
}

int minute(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm.tm_min;
}

int second(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm.tm_sec;
}

int day(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm.tm_day;
}

int weekday(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm.tm_wday;
}

int month(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm.tm_mon;
}

int year(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm.tm_year;
}

time_t now() {
    return time(nullptr);
}
