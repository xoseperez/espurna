/*

Part of SCHEDULER MODULE

Copyright (C) 2017 by faina09
Adapted by Xose Pérez <xose dot perez at gmail dot com>

Copyright (C) 2019-2024 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include "datetime.h"
#include "types.h"
#include "utils.h"

#include "libs/Delimiter.h"

#include "scheduler_common.ipp"

namespace espurna {
namespace scheduler {
namespace {

// Heavily inspired by systemd.time OnCalendar= syntax
// https://www.freedesktop.org/software/systemd/man/latest/systemd.time.html#Calendar%20Events
//
// Most specs should work, but there are some differences
// - there are no year ranges here (pending a better idea of range/repetetion algo)
// - '-L' instead of '~' for last-day-of-month
// - '-W' syntax for weeks aka 'day-index'
// - ranges *can* loop around - Fri..Tue 10:55..05 is valid, but still means the same hour and not 11:00..05

namespace parse {

// special out-of-bounds day for parsed value validation
constexpr auto InvalidDay = datetime::Weekday{ 7 };

// date or time ANY value
constexpr bool is_any(StringView view) {
    return view.length() == 1 && '*' == view[0];
}

// date day-of-week N counted backwards
constexpr bool is_last(StringView view) {
    return view.length() == 1 && ('l' == view[0] || 'L' == view[0]);
}

// Nth week of the month
constexpr bool is_weekday_index(StringView view) {
    return view.length() == 2 && ('W' == view[0] || 'w' == view[0]);
}

constexpr int dec_char2int(char c) {
    return (('0' <= c) && (c <= '9'))
        ? (c - '0') : -1;
}

bool fill_bit_range(bits::Range& range, StringView view) {
    const char* YYCURSOR { view.begin() };
    const char* YYLIMIT { view.end() };

    bool done { false };
    bool out { false };

    auto last = -1;
    auto range_last = -1;

    ParseUnsignedResult result;
    int repeat { -1 };

    const char* p { nullptr };

loop:
    /*!local:re2c:fill_bit_range

      re2c:api:style = free-form;
      re2c:define:YYCTYPE = char;
      re2c:yyfill:enable = 0;
      re2c:eof = 0;

      dec = [0-9]+;

      [0-9] {
        goto take_last;
      }

      [/][0-9]+ {
        p = YYCURSOR - 1;
        while (*p != '/') {
          --p;
        }
        ++p;

        goto take_repeat;
      }

      [.][.] {
        goto take_range;
      }

      [,] {
        goto consume_last;
      }

      $ {
        done = true;
        goto consume_last;
      }

      * {
        out = false;
        goto return_out;
      }

    */

// update {last} with the latest digit
take_last:
    if (last == -1) {
      last = 0;
    }

    last = (last * 10) + (*(YYCURSOR - 1) - '0');
    goto loop;

// map every nth bit, based on repeat value
take_repeat:
    if (last == -1) {
        out = false;
        goto return_out;
    }

    if (repeat != -1) {
        out = false;
        goto return_out;
    }

    result = parseUnsigned(StringView(p, YYCURSOR), 10);
    if (!result.ok) {
        out = false;
        goto return_out;
    }

    repeat = result.value;

    goto consume_last;

// expect {last} to be set, bail otherwise
take_range:
    if (last == -1) {
        out = false;
        goto return_out;
    }

    range_last = last;
    last = -1;

    goto loop;

consume_last:
    // validate w/ range
    if ((last != -1) && !range.valid(last)) {
        out = false;
        goto return_out;
    }

    // in case repeat is set for a single value, set {last} to the last possible bit
    if ((range_last == -1) && (repeat > 0)) {
        range_last = last;
        last = range.end();
    }

    // fill output with {range_last}..{last} within range boundaries
    // bitset should support roll over for when {last} is less than {range_last}
    // meaning, 45..15 <=> 0..15,45..63. otherwise, support the usual 0..63
    // all bits between x and y, also including x and y, are set (i.e. range is inclusive)
    if ((last != -1) && (range_last != -1)) {
        range.fill(range_last, last, (repeat > 0) ? repeat : 1);
        out = true;
    // return immediately when there was no {last} after {range_last} was set
    } else if (range_last != -1) {
        done = true;
        out = false;
    // everything is ok, set and consume {last} bit
    } else if (last != -1) {
        range.set(last);
        out = true;
    // no need to continue, return current state
    } else {
        done = true;
    }

    if (done) {
        goto return_out;
    }

    last = range_last = -1;
    repeat = -1;

    goto loop;

return_out:
    return out;
}

// monday,wednesday / mon...fri / 1,2,3 / mon,2
// numeric value is 1..7, starting from monday
// both short and full forms are allowed
// (which can be either in upper or lower case, or mixed)
bool parse_weekdays(WeekdayMatch& match, StringView view) {

    const char* YYCURSOR { view.begin() };
    const char* YYLIMIT { view.end() };
    const char* YYMARKER;

    bool done { false };
    bool out { false };

    auto current = InvalidDay;

    auto last = InvalidDay;
    auto range_last = InvalidDay;

loop:
    /*!local:re2c:parse_weekdays

      re2c:api:style = free-form;
      re2c:define:YYCTYPE = char;
      re2c:flags:case-insensitive = 1;
      re2c:yyfill:enable = 0;
      re2c:eof = 0;

      mon =  "mon" | "monday"    | "1";
      tue =  "tue" | "tuesday"   | "2";
      wed =  "wed" | "wednesday" | "3";
      thu =  "thu" | "thursday"  | "4";
      fri =  "fri" | "friday"    | "5";
      sat =  "sat" | "saturday"  | "6";
      sun =  "sun" | "sunday"    | "7";

      mon {
        current = datetime::Monday;
        goto take_last;
      }

      tue {
        current = datetime::Tuesday;
        goto take_last;
      }

      wed {
        current = datetime::Wednesday;
        goto take_last;
      }

      thu {
        current = datetime::Thursday;
        goto take_last;
      }

      fri {
        current = datetime::Friday;
        goto take_last;
      }

      sat {
        current = datetime::Saturday;
        goto take_last;
      }

      sun {
        current = datetime::Sunday;
        goto take_last;
      }

      [.][.] {
        out = false;
        goto take_range;
      }

      [,] {
        out = false;
        goto consume_last;
      }

      $ {
        done = true;
        goto consume_last;
      }

      * {
        out = false;
        goto return_out;
      }

    */

// process pending token, bail when seeing duplicates or errors
take_last:
    if ((current == InvalidDay) || (last != InvalidDay)) {
        out = false;
        goto return_out;
    }

    last = current;
    current = InvalidDay;

    goto loop;

// expect {last} to be set, bail otherwise
take_range:
    if (last != InvalidDay) {
        range_last = last;
        last = InvalidDay;
        goto loop;
    }

    goto return_out;

consume_last:
    // fill output with {range_last}..{last}
    // similar to generic range parser, support inverse fill
    // no /{repeat}, though, but this is the shortest range
    if ((last != InvalidDay) && (range_last != InvalidDay)) {
        match |= fill_match(range_last, last);
        out = true;
    // return immediately in case no {range_last} was provided
    } else if (range_last != InvalidDay) {
        done = true;
        out = false;
    // otherwise, just set the previouly updated {last}
    } else if (last != InvalidDay) {
        match |= last;
        out = true;
    // return the results gathered so far
    } else {
        done = true;
    }

    if (done) {
        goto return_out;
    }

    last = range_last = InvalidDay;

    goto loop;

return_out:
    return out && (YYCURSOR == YYLIMIT);
}

bool update_year(DateMatch& match, StringView view) {
    if (is_any(view)) {
        match.year = 0;
        return true;
    }

    auto result = parseUnsigned(view, 10);
    if (result.ok && (result.value >= 1970)) {
        match.year = result.value;
        return true;
    }

    return false;
}

bool update_month(DateMatch& match, StringView view) {
    if (is_any(view)) {
        match.month.set();
        return true;
    }

    auto range = bits::Range{1, 12};
    if (fill_bit_range(range, view)) {
        match.month = range.to_u32() >> 1;
        return true;
    }

    return false;
}

bool update_day_of_week(DateMatch& match, StringView view) {
    if (is_any(view)) {
        match.day.set();
        match.day[0] = false;
        return true;
    }

    auto range = bits::Range{1, 31};
    if (fill_bit_range(range, view)) {
        match.day = range.to_u32();
        return true;
    }

    return false;
}

bool update_weekday_index(DateMatch& match, StringView view) {
    if ((view[1] == 'L') || (view[1] == 'l')) {
        match.day_index[0] = true;
        return true;
    }

    auto index = dec_char2int(view[1]);
    if ((index >= 1) && (index <= 5)) {
        match.day_index[index] = true;
        return true;
    }

    return false;
}

bool update_weekday_last(DateMatch& match, StringView view) {
    match.day[0] = true;

    if (view.length() == 1) {
        return true;
    }

    view = StringView(view.begin() + 1, view.end());

    auto range = bits::Range{1, 31};
    if (fill_bit_range(range, view)) {
        match.day |= range.to_u32();
        return true;
    }

    return false;
}

bool parse_date(DateMatch& match, StringView view) {
    const char* YYCURSOR { view.begin() };
    const char* YYLIMIT { view.end() };
    const char* YYMARKER;

    StringView tmp;
    bool out { false };

    const char *p;

    /*!conditions:re2c:date*/
    int c = yycinit;

    /*!stags:re2c:date format = 'const char *@@;'; */

    // {yyyy} - single number for year (todo? u64 mask should cover 2024..2088)
    // {mm} - month. single number, comma-separated numbers or numeric range
    // {dow} - day-of-week. single number, comma-separated numbers or numeric range
    // {wdi} - weekday-index. single number for Nth weekday in the month.
    // {wdl} - weekday-last. same as {dow}, but starting from the end of the month

    // using common format of {yyyy}-{mm}-{dd}, with optional {yyyy}
    // with {wdl}, L{N} used instead of replacing hyphen with ~{N} like systemd calendar does

    // note the ambigous [*]-MM-DD / [*]-DD match, using lookahead to work around that

    /*!local:re2c:date

      re2c:api:style = free-form;
      re2c:define:YYGETCONDITION = "c";
      re2c:define:YYSETCONDITION = "c = @@;";
      re2c:define:YYCTYPE = char;
      re2c:flags:case-insensitive = 1;
      re2c:flags:tags = 1;
      re2c:yyfill:enable = 0;
      re2c:eof = 0;

      range_or_dec = [0-9,./]+;

      yyyy = [*] | [0-9]{4};
      mm   = [*] | range_or_dec;

      dow  = [*] | range_or_dec;
      wdi  = [Ww][1-5Ll];
      wdl  = [Ll] range_or_dec?;

      last = (dow | wdi | wdl);

      <init> @p yyyy / [-] mm [-] last => yyyy_mm_dd {
        tmp = StringView(p, YYCURSOR - p);
        if (!update_year(match, tmp)) {
          goto return_out;
        }

        ++YYCURSOR;

        goto yyc_yyyy_mm_dd;
      }

      <init,yyyy_mm_dd> @p mm / [-] last => mm_dd {
        tmp = StringView(p, YYCURSOR - p);
        if (!update_month(match, tmp)) {
          goto return_out;
        }

        ++YYCURSOR;

        goto yyc_mm_dd;
      }

      <mm_dd> @p dow {
        tmp = StringView(p, YYCURSOR - p);
        out = update_day_of_week(match, tmp);
        goto return_out;
      }

      <mm_dd> @p wdi {
        tmp = StringView(p, YYCURSOR - p);
        out = update_weekday_index(match, tmp);
        goto return_out;
      }

      <mm_dd> @p wdl {
        tmp = StringView(p, YYCURSOR - p);
        out = update_weekday_last(match, tmp);
        goto return_out;
      }

      <*> * {
        out = false;
        goto return_out;
      }

      <*> $ {
        goto return_out;
      }

    */

return_out:
    return out && (YYCURSOR == YYLIMIT);
}

bool update_hour(TimeMatch& match, StringView view) {
    if (is_any(view)) {
        match.hour.set();
        return true;
    }

    auto range = bits::Range{0, 23};
    if (fill_bit_range(range, view)) {
        match.hour = range.to_u32();
        return true;
    }

    return false;
}

bool update_minute(TimeMatch& match, StringView view) {
    if (is_any(view)) {
        match.minute.set();
        return true;
    }

    auto range = bits::Range{0, 60};
    if (fill_bit_range(range, view)) {
        match.minute = range.to_u64();
        return true;
    }

    return false;
}

// *:15 - every 15th minute of an hour
// 15:* - every minute of 15th hour
// *:*  - every minute
//
// 0/5:00  - 00:00, 05:00, 10:00, 15:00, 20:00
// *:0/30  - 00:30, 01:00, 01:30, etc.
bool parse_time(TimeMatch& match, StringView view) {
    const char* YYCURSOR { view.begin() };
    const char* YYLIMIT { view.end() };
    const char* YYMARKER;

    StringView tmp;
    bool out { false };

    const char *h0;
    const char *h1;

    const char *m0;

    /*!stags:re2c:timeofday format = 'const char *@@;'; */

loop:
    /*!local:re2c:timeofday

      re2c:api:style = free-form;
      re2c:define:YYCTYPE = char;
      re2c:define:YYSTAGP = "@@ = YYCURSOR;";
      re2c:define:YYSTAGN = "@@ = nullptr;";
      re2c:flags:tags = 1;
      re2c:yyfill:enable = 0;
      re2c:eof = 0;

      range_or_dec = [0-9,./]+;

      hh = [*] | range_or_dec;
      mm = [*] | range_or_dec;

      @h0 hh @h1 [:] @m0 mm {
        tmp = StringView(h0, h1 - h0);
        if (!update_hour(match, tmp)) {
          goto return_out;
        }

        tmp = StringView(m0, YYCURSOR - m0);
        if (!update_minute(match, tmp)) {
          goto return_out;
        }

        out = true;

        goto loop;
      }

      * {
        out = false;
        goto return_out;
      }

      $ {
        goto return_out;
      }
    */

return_out:
    return out && (YYCURSOR == YYLIMIT);
}

// Extra conditions, generally set through keywords
bool parse_time_keyword(TimeMatch& match, StringView view) {
    const char* YYCURSOR { view.begin() };
    const char* YYLIMIT { view.end() };
    const char* YYMARKER;

    bool out { false };

loop:
    /*!local:re2c:flags

      re2c:api:style = free-form;
      re2c:define:YYCTYPE = char;
      re2c:flags:case-insensitive = 1;
      re2c:yyfill:enable = 0;
      re2c:eof = 0;

      utc = "UTC";
      sunrise = "Sunrise";
      sunset = "Sunset";

      sunrise {
        match.flags |= FlagSunrise;
        out = true;
        goto loop;
      }

      sunset {
        match.flags |= FlagSunset;
        out = true;
        goto loop;
      }

      utc {
        match.flags |= FlagUtc;
        out = true;
        goto loop;
      }

      * {
        out = false;
        goto return_out;
      }

      $ {
        goto return_out;
      }
    */

return_out:
    return out && (YYCURSOR == YYLIMIT);
}

// Relative time triggers, "before" or "after" specific 'target' event happens.
// Offset can be 0, causing action to immediately trigger.
// Can depend on either fixed-time events like sunrise & sunset, or other calendar schedules.

bool parse_relative_offset(datetime::Minutes& out, StringView view) {
    if (!view.length()) {
        return false;
    }

    if (!view.length()) {
        return false;
    }

    auto result = duration::parse(view, datetime::Minutes::period{});
    if (result.ok) {
        out = duration::to_chrono<datetime::Minutes>(result.value);
        return true;
    }

    return false;
}

bool parse_relative_keyword(Relative& out, StringView view) {
    bool result { false };

    if (STRING_VIEW("before").equalsIgnoreCase(view)) {
        out.order = relative::Order::Before;
        result = true;
    } else if (STRING_VIEW("after").equalsIgnoreCase(view)) {
        out.order = relative::Order::After;
        result = true;
    }

    return result;
}

bool update_relative_id(Relative& out, StringView view) {
    auto result = parseUnsigned(view, 10);

    if (result.ok && result.value <= 255) {
        out.data = result.value;
        return true;
    }

    return false;
}

bool parse_relative_target(Relative& value, StringView view) {
    const char* YYCURSOR { view.begin() };
    const char* YYLIMIT { view.end() };
    const char* YYMARKER;

    StringView tmp;
    bool out { false };

    const char *p;

    /*!stags:re2c:relative_target format = 'const char *@@;'; */

    /*!local:re2c:relative_target

      re2c:api:style = free-form;
      re2c:define:YYCTYPE = char;
      re2c:flags:case-insensitive = 1;
      re2c:flags:tags = 1;
      re2c:yyfill:enable = 0;
      re2c:eof = 0;

      calendar = "Cal" | "Calendar";

      sunrise = "Sunrise";
      sunset = "Sunset";

      calendar [#] @p [0-9]+ {
        value.type = relative::Type::Calendar;

        tmp = StringView(p, YYCURSOR - p);
        out = update_relative_id(value, tmp);

        goto return_out;
      }

      ["] @p [A-Za-z0-9]+ ["] {
        value.type = relative::Type::Named;

        tmp = StringView(p, YYCURSOR - p - 1);
        value.name = tmp.toString();

        out = true;

        goto return_out;
      }


      sunrise {
        value.type = relative::Type::Sunrise;
        out = true;

        goto return_out;
      }

      sunset {
        value.type = relative::Type::Sunset;
        out = true;

        goto return_out;
      }

      * {
        out = false;
        goto return_out;
      }

      $ {
        goto return_out;
      }
    */

return_out:
    return out && (YYCURSOR == YYLIMIT);
}

// 2024-08-24T19:18:25+00:00
// 2024-08-24T19:18:25Z
// 2024-08-25T07:18:25
// 2024-08-25T07:18:25+12:00

namespace iso8601 {

constexpr StringView year(StringView view) {
    return StringView(&view[0], 4);
}

constexpr StringView month(StringView view) {
    return StringView(&view[5], 2);
}

constexpr StringView day(StringView view) {
    return StringView(&view[8], 2);
}

constexpr StringView hour(StringView view) {
    return StringView(&view[11], 2);
}

constexpr StringView minute(StringView view) {
    return StringView(&view[14], 2);
}

constexpr StringView second(StringView view) {
    return StringView(&view[17], 2);
}

constexpr StringView tz(StringView view) {
    return (view.length() == 19)
        ? StringView{}
        : StringView(&view[20], &view[view.length()]);
}

constexpr int from_one_digit(char value) {
    return ((value >= '0') && (value <= '9'))
        ? (value - '0')
        : 0;
}

constexpr int from_two_digits(StringView view) {
    return (from_one_digit(view.data()[0]) * 10)
         + from_one_digit(view.data()[1]);
}

constexpr int from_four_digits(StringView view) {
    return (from_one_digit(view.data()[0]) * 1000)
         + (from_one_digit(view.data()[1]) * 100)
         + (from_one_digit(view.data()[2]) * 10)
         + from_one_digit(view.data()[3]);
}

constexpr bool is_valid(const datetime::DateHhMmSs& datetime) {
    return (datetime.month < 13) && (datetime.month > 0)
        && (datetime.day < 32) && (datetime.day > 0)
        && (datetime.hours < 24)
        && (datetime.minutes < 60)
        && (datetime.seconds < 60);
}

constexpr datetime::DateHhMmSs make_datetime(StringView view) {
    return datetime::DateHhMmSs{
        .year = from_four_digits(year(view)),
        .month = from_two_digits(month(view)),
        .day = from_two_digits(day(view)),
        .hours = from_two_digits(hour(view)),
        .minutes = from_two_digits(minute(view)),
        .seconds = from_two_digits(second(view)),
    };
}

} // namespace iso8601

bool parse_simple_iso8601(datetime::DateHhMmSs& datetime, bool& utc, StringView view) {
    const char* YYCURSOR { view.begin() };
    const char* YYLIMIT { view.end() };
    const char* YYMARKER;

    StringView tmp;
    bool out { false };

    const char *p;

    /*!conditions:re2c:simple_iso8601*/
    int c = yycinit;

    /*!stags:re2c:simple_iso8601 format = 'const char *@@;'; */

loop:
    /*!local:re2c:simple_iso8601

      re2c:api:style = free-form;
      re2c:define:YYGETCONDITION = "c";
      re2c:define:YYSETCONDITION = "c = @@;";
      re2c:define:YYCTYPE = char;
      re2c:flags:case-insensitive = 0;
      re2c:flags:tags = 1;
      re2c:yyfill:enable = 0;
      re2c:eof = 0;

      YYYY = [0-9]{4};
      MM = [0-9]{2};
      DD = [0-9]{2};

      hh = [0-9]{2};
      mm = [0-9]{2};
      ss = [0-9]{2};

      zero = "+00:00";
      zulu = "Z";

      <init> @p YYYY [-] MM [-] DD [T] hh [:] mm [:] ss => tz {
        tmp = StringView{p, YYCURSOR};

        datetime = iso8601::make_datetime(tmp);
        if (!iso8601::is_valid(datetime)) {
          goto return_out;
        }

        goto loop;
      }

      <tz> zulu | zero {
        out = true;
        utc = true;
        goto return_out;
      }

      <tz> $ {
        out = true;
        utc = false;
        goto return_out;
      }

      <*> * {
        out = false;
        goto return_out;
      }

      <*> $ {
        out = false;
        goto return_out;
      }
    */

return_out:
    return out && (YYCURSOR == YYLIMIT);

}

} // namespace parse

using parse::parse_date;
using parse::parse_weekdays;
using parse::parse_time;
using parse::parse_time_keyword;
using parse::parse_relative_keyword;
using parse::parse_relative_offset;
using parse::parse_relative_target;
using parse::parse_simple_iso8601;

Schedule parse_schedule(StringView view) {
    Schedule out;

    // DATE " " WDS " " TIME " " KW
    const auto spaces = std::count(view.begin(), view.end(), ' ');
    if (spaces > 3) {
        return out;
    }

    auto split = StatefulSplitView(view);

    bool parsed_date { false };
    bool parsed_weekdays { false };
    bool parsed_time { false };
    bool parsed_keyword { false };

    int parsed { 0 };

    while (split.next()) {
        auto elem = split.current();

        // most expected order, starting with date
        if (!parsed_date && ((parsed_date = parse_date(out.date, elem)))) {
            ++parsed;
            continue;
        }

        // then weekdays
        if (!parsed_weekdays && ((parsed_weekdays = parse_weekdays(out.weekdays, elem)))) {
            ++parsed;
            continue;
        }

        // then time
        if (!parsed_time && ((parsed_time = parse_time(out.time, elem)))) {
            ++parsed;
            continue;
        }

        // and keyword is always at the end. forcibly stop the parsing, regardless of the state
        if ((parsed_keyword = parse_time_keyword(out.time, elem))) {
            ++parsed;
            break;
        }
    }

    // expect one of each element, plus optional keyword
    if (parsed != (1 + spaces)) {
        return out;
    }

    // do not want both time and sun{rise,set}
    if (want_sunrise_sunset(out.time) && parsed_time) {
        return out;
    }

    out.ok = parsed_date
        || parsed_weekdays
        || parsed_time
        || parsed_keyword;

    if (out.ok && !parsed_time && !want_sunrise_sunset(out.time)) {
        out.time.hour = 0b1;
        out.time.minute = 0b1;
    }

    return out;
}

Relative parse_relative(StringView view) {
    Relative out;

    // OFFSET " " KEYWORD " " TARGET
    const auto spaces = std::count(view.begin(), view.end(), ' ');
    if (spaces > 2) {
        return out;
    }

    auto split = StatefulSplitView(view);

    bool parsed_offset { false };
    bool parsed_keyword { false };
    bool parsed_target { false };

    int found { 0 };

    while ((found < 3) && split.next()) {
        auto elem = split.current();
        ++found;

        if (!parsed_offset && (parsed_offset = parse_relative_offset(out.offset, elem))) {
            continue;
        }

        if (!parsed_keyword && (parsed_keyword = parse_relative_keyword(out, elem))) {
            continue;
        }

        if (!parsed_target && (parsed_target = parse_relative_target(out, elem))) {
            continue;
        }
    }

    if (!parsed_keyword || !parsed_target) {
        out.type = relative::Type::None;
    }

    if (!parsed_offset && out.type != relative::Type::None) {
        out.offset = datetime::Minutes{ 1 };
    }

    return out;
}

} // namespace
} // namespace scheduler
} // namespace espurna
