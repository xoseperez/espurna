/*

LED MODULE

Copyright (C) 2020-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

// Scans input string with format
// '<on1>,<off1>,<repeats1> <on2>,<off2>,<repeats2> ...'
// And returns a list of Delay objects for the pattern

Pattern::Pattern(espurna::StringView value) {
    const char* on1;
    const char* on2;

    const char* off1;
    const char* off2;

    const char* repeat1;
    const char* repeat2;

    const char* YYCURSOR { value.begin() };
    const char* YYLIMIT { value.end() };
    const char* YYMARKER;

loop:
/*!stags:re2c format = 'const char *@@;'; */
/*!re2c
        re2c:define:YYCTYPE = char;
        re2c:flags:tags = 1;
        re2c:yyfill:enable   = 0;
        re2c:yych:conversion = 1;
        re2c:indent:top      = 1;
        re2c:eof = 0;

        end = "\x00";
        wsp = [ \t]+;
        num = [0-9]+;

        $ { return; }
        * { return; }

        wsp { goto loop; }

        @on1 num @on2 [,] @off1 num @off2 [,] @repeat1 num @repeat2 {
            const auto on = parseUnsigned(StringView(on1, on2), 10);
            if (!on.ok) {
                return;
            }

            const auto off = parseUnsigned(StringView(off1, off2), 10);
            if (!off.ok) {
                return;
            }

            using Repeats = Delay::Repeats;
            constexpr Repeats RepeatsMax { Delay::RepeatsMax };
            const auto repeats = parseUnsigned(StringView(repeat1, repeat2), 10);
            if (!repeats.ok) {
                return;
            }

            _delays.emplace_back(
                std::min(duration::Milliseconds(on.value), Delay::MillisecondsMax),
                std::min(duration::Milliseconds(off.value), Delay::MillisecondsMax),
                std::min(repeats.value, RepeatsMax));
            if (repeats.value) {
                goto loop;
            }
        }
*/
}
