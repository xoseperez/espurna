/*

Part of the IR MODULE

For more info:
- https://re2c.org/
- https://re2c.org/manual/manual_c.html

*/

#pragma once

ParseResult<Payload> parse(StringView view) {
    const char* YYCURSOR { view.begin() };
    const char* YYLIMIT { view.end() };
    const char* YYMARKER;

    const char *p0 = nullptr, *p1 = nullptr;
    const char *c0 = nullptr, *c1 = nullptr;
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
      val = [0-9A-Fa-f];

      @p0 dec+ @p1 [:]
      @c0 val+ @c1 { goto update_out; }

      @p0 dec+ @p1 [:]
      @c0 val+ @c1 [:]
      @s0 dec+ @s1 { goto update_out; }

      @p0 dec+ @p1 [:]
      @c0 val+ @c1 [:]
      @s0 dec+ @s1 [:]
      @d0 dec+ @d1 { goto update_out; }

      * { goto return_out; }
      $ { goto return_out; }
    */

update_out:
    {
        if (!((c1 - c0) % 2)) {
            out = prepare(
                StringView{p0, p1},
                StringView{c0, c1},
                StringView{s0, s1},
                StringView{d0, d1});
        }
    }

return_out:
    return out;
}
