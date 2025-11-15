/*

LED MODULE

Copyright (C) 2020-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include "utils.h"
#include "led_internal.h"
#include "led_pattern.ipp"

namespace espurna {
namespace led {
namespace {

using ParseDuration = espurna::duration::Milliseconds;

using DurationPair = espurna::duration::Pair;
using PairResult = espurna::duration::PairResult;

Duration native_duration(DurationPair pair) {
    using namespace espurna::duration;
    return to_chrono<Duration>(pair);
}

Duration native_duration(PairResult result) {
    return result.ok
        ? native_duration(result.value)
        : Duration::min();
}

PairResult parse_time(StringView view) {
    using namespace espurna::duration;
    return parse(view, ParseDuration::period{});
}

static constexpr auto RepeatsMax = size_t{ 255 };

// Scans input string with format
// '<on1>,<off1>,<repeats1> <on2>,<off2>,<repeats2> ...'
// And returns a list of Delay objects for the pattern

struct PatternResult {
    bool ok{ false };
    Pattern value;
};

PatternResult parse(StringView value) {
    PatternResult out;

    StringView tmp;

    const char* p1;
    const char* p2;
    const char* p3;

    const char* YYCURSOR { value.begin() };
    const char* YYLIMIT { value.end() };
    const char* YYMARKER;

/*!conditions:re2c:led_parse*/
    int c = yycinit;

/*!stags:re2c:led_parse format = 'const char *@@;'; */

loop:
/*!local:re2c:led_parse

      re2c:api:style = free-form;
      re2c:define:YYCTYPE = char;
      re2c:define:YYGETCONDITION = "c";
      re2c:define:YYSETCONDITION = "c = @@;";
      re2c:flags:tags = 1;
      re2c:yyfill:enable = 0;
      re2c:eof = 0;

      num = [0-9]+;
      sec = [s];

      wsp = [ \t\v\r\n]+;
      spec = num sec?;

      <init, parse> @p1 spec [,] @p2 spec ([,] @p3 num)? => separator {
          tmp = StringView(p1, p2 - p1 - 1);
          const auto on = parse_time(tmp);
          if (!on.ok) {
              goto return_err;
          }

          if (p3) {
              tmp = StringView(p2, p3 - p2 - 1);
          } else {
              tmp = StringView(p2, YYCURSOR);
          }

          const auto off = parse_time(tmp);
          if (!off.ok) {
              goto return_err;
          }

          size_t repeats_value;
          if (p3) {
              tmp = StringView(p3, YYCURSOR);
              const auto repeats = parseUnsigned(tmp, 10);
              if (!repeats.ok) {
                  goto return_err;
              }
              repeats_value = repeats.value;
          } else {
              repeats_value = 0;
          }

          repeats_value = std::min(repeats_value, RepeatsMax);

          out.value.add(
              native_duration(on),
              native_duration(off),
              repeats_value);
          out.ok = true;
          if (!repeats_value) {
              goto return_out;
          }

          goto loop;
      }

      <parse> [Rr] | "0,0" {
          if (out.value.size()) {
              out.value.add(Duration::zero(), Duration::zero(), 0);
          } else {
              goto return_err;
          }

          goto return_out;
      }

      <separator> wsp => parse {
          goto loop;
      }

      <*> $ { goto return_out; }
      <*> * { goto return_out; }
*/

return_err:
    out.ok = false;

return_out:
    return out;
}

} // namespace
} // namespace led
} // namespace espurna
