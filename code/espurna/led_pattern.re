/*

LED MODULE

Copyright (C) 2020-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

// Scans input string with format
// '<on1>,<off1>,<repeats1> <on2>,<off2>,<repeats2> ...'
// And returns a list of Delay objects for the pattern

Pattern::Pattern(const char* begin, const char* end) {
    char buffer[16];

    const char* on1;
    const char* on2;

    const char* off1;
    const char* off2;

    const char* repeat1;
    const char* repeat2;

    const char* YYCURSOR { begin };
    const char* YYLIMIT { end };
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
            memcpy(buffer, on1, on2 - on1);
            buffer[on2 - on1] = '\0';
            espurna::duration::Milliseconds::rep on { strtoul(buffer, nullptr, 10) };

            memcpy(buffer, off1, off2 - off1);
            buffer[off2 - off1] = '\0';
            espurna::duration::Milliseconds::rep off { strtoul(buffer, nullptr, 10) };

            memcpy(buffer, repeat1, repeat2 - repeat1);
            buffer[repeat2 - repeat1] = '\0';

            using Repeats = Delay::Repeats;
            Repeats repeats { strtoul(buffer, nullptr, 10) };

            constexpr Repeats RepeatsMax { Delay::RepeatsMax };
            _delays.emplace_back(
                std::min(espurna::duration::Milliseconds(on), Delay::MillisecondsMax),
                std::min(espurna::duration::Milliseconds(off), Delay::MillisecondsMax),
                std::min(repeats, RepeatsMax));

            if (repeats) {
                goto loop;
            }
        }
*/
}
