/* Generated by re2c 3.0 */
#line 1 "espurna/led_pattern.re"
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
#line 33 "espurna/led_pattern.re.ipp"
const char *yyt1;const char *yyt2;const char *yyt3;
#line 30 "espurna/led_pattern.re"


#line 38 "espurna/led_pattern.re.ipp"
	{
		char yych;
		yych = (char)*YYCURSOR;
		switch (yych) {
			case '\t':
			case ' ': goto yy3;
			case '0' ... '9':
				yyt1 = YYCURSOR;
				goto yy5;
			default:
				if (YYLIMIT <= YYCURSOR) goto yy13;
				goto yy1;
		}
yy1:
		++YYCURSOR;
yy2:
#line 44 "espurna/led_pattern.re"
		{ return; }
#line 57 "espurna/led_pattern.re.ipp"
yy3:
		yych = (char)*++YYCURSOR;
		switch (yych) {
			case '\t':
			case ' ': goto yy3;
			default: goto yy4;
		}
yy4:
#line 46 "espurna/led_pattern.re"
		{ goto loop; }
#line 68 "espurna/led_pattern.re.ipp"
yy5:
		yych = (char)*(YYMARKER = ++YYCURSOR);
		switch (yych) {
			case ',': goto yy6;
			case '0' ... '9': goto yy8;
			default: goto yy2;
		}
yy6:
		yych = (char)*++YYCURSOR;
		switch (yych) {
			case '0' ... '9':
				yyt2 = YYCURSOR;
				goto yy9;
			default: goto yy7;
		}
yy7:
		YYCURSOR = YYMARKER;
		goto yy2;
yy8:
		yych = (char)*++YYCURSOR;
		switch (yych) {
			case ',': goto yy6;
			case '0' ... '9': goto yy8;
			default: goto yy7;
		}
yy9:
		yych = (char)*++YYCURSOR;
		switch (yych) {
			case ',': goto yy10;
			case '0' ... '9': goto yy9;
			default: goto yy7;
		}
yy10:
		yych = (char)*++YYCURSOR;
		switch (yych) {
			case '0' ... '9':
				yyt3 = YYCURSOR;
				goto yy11;
			default: goto yy7;
		}
yy11:
		yych = (char)*++YYCURSOR;
		switch (yych) {
			case '0' ... '9': goto yy11;
			default: goto yy12;
		}
yy12:
		on1 = yyt1;
		off1 = yyt2;
		repeat1 = yyt3;
		on2 = yyt2 - 1;
		off2 = yyt3 - 1;
		repeat2 = YYCURSOR;
#line 48 "espurna/led_pattern.re"
		{
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
#line 149 "espurna/led_pattern.re.ipp"
yy13:
#line 43 "espurna/led_pattern.re"
		{ return; }
#line 153 "espurna/led_pattern.re.ipp"
	}
#line 74 "espurna/led_pattern.re"

}
