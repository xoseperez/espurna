/*

Part of the IR MODULE

For more info:
- https://re2c.org/
- https://re2c.org/manual/manual_c.html

*/

#pragma once

// TODO: some sanity checks for 'valid' ranges of the parsed values, when state machine reaches individual value decoders?

ParseResult<Payload> parse(StringView view) {
    const char* YYCURSOR { view.begin() };
    const char* YYLIMIT { view.end() };
    const char* YYMARKER;

    const char *p0 = nullptr, *p1 = nullptr;
    const char *c0 = nullptr, *c1 = nullptr;
    const char *b0 = nullptr, *b1 = nullptr;
    const char *r0 = nullptr, *r1 = nullptr;
    const char *s0 = nullptr, *s1 = nullptr;
    const char *d0 = nullptr, *d1 = nullptr;

    ParseResult<Payload> out;

    /*!stags:re2c format = 'const char *@@{tag} { nullptr };\n'; */
    /*!re2c

      re2c:eof = 0;
      re2c:yyfill:enable = 0;
      re2c:flags:tags = 1;
      re2c:define:YYCTYPE = "char";
      re2c:define:YYFILL = "goto return_out;";

      dec = [0-9];
      val = [0-9A-F]{2,8};

      @p0 dec+ @p1 [:]
      @c0 val @c1 [:]
      @b0 dec+ @b1 { goto update_out; }

      @p0 dec+ @p1 [:]
      @c0 val @c1 [:]
      @b0 dec+ @b1 [:]
      @r0 dec+ @r1 { goto update_out; }

      @p0 dec+ @p1 [:]
      @c0 val @c1 [:]
      @b0 dec+ @b1 [:]
      @r0 dec+ @r1 [:]
      @s0 dec+ @s1 { goto update_out; }

      @p0 dec+ @p1 [:]
      @c0 val @c1 [:]
      @b0 dec+ @b1 [:]
      @r0 dec+ @r1 [:]
      @s0 dec+ @s1 [:]
      @d0 dec+ @d1 { goto update_out; }

      * { goto return_out; }
      $ { goto return_out; }
    */

update_out:
    {
        Payload result;
        result.type = payload::type(StringView{p0, p1});
        result.value = payload::value(StringView{c0, c1});
        result.bits = payload::bits(StringView{b0, b1});
        if (r0 && r1) {
            result.repeats = payload::repeats(StringView{r0, r1});
        }
        if (s0 && s1) {
            result.series = payload::series(StringView{s0, s1});
        }
        if (d0 && d1) {
            result.delay = payload::delay(StringView{d0, d1});
        }
        out = std::move(result);
    }

return_out:
    return out;
}
