/*

Part of the IR MODULE

For more info:
- https://re2c.org/
- https://re2c.org/manual/manual_c.html

*/

#pragma once

// ref.
// - https://github.com/skvadrik/re2c/blob/423c56ca556de944598f29b59abc43f89b616fb6/examples/c/submatch/02_mtags.re
// - https://github.com/skvadrik/re2c/blob/b4efac3cc09abcab6cd063df80b23d0da70acc70/examples/c/submatch/http_rfc7230.re
// - https://github.com/skvadrik/re2c/blob/423c56ca556de944598f29b59abc43f89b616fb6/examples/go/submatch/02_mtags.re
// - https://github.com/skvadrik/re2c/blob/b4efac3cc09abcab6cd063df80b23d0da70acc70/test/golang/005_mtags.re
//
// Example code MTAG updates are stored in a tree-like structure (in the submatch example, a flat vector of TAG and YYCURSOR)
// Using the observed parser behaviour, instead try to parse things inline right when the MTAG function is called,
// by comparing the last TAG value with the current one, noticing the increment that will only happen when the match actually happens.
// May be problematic, but at least there is some safety in the fact that the previous values are only updated when the match block is executed.

// TODO: same as the 'simple' variant, does not really notify about any errors

struct TimeRange {
    int tag;
    const char* start;
};

ParseResult<Payload> parse(StringView view) {
    const char* YYCURSOR { view.begin() };
    const char* YYLIMIT { view.end() };
    const char* YYMARKER;

    const char *d0, *d1;
    const char *s0, *s1;
    const char *f0, *f1;

    static constexpr int Root { -1 };
    TimeRange range { Root, nullptr };
    int t0 { Root };
    int t1 { Root };

    decltype(Payload::time) time;
    ParseResult<Payload> out;

    auto update_range = [&](const char* ptr, int tag) {
        if (ptr && !range.start) {
            time.reserve(std::count(view.begin(), view.end(), ','));
            range.start = ptr;
        }

        if (ptr && range.start) {
            if (range.tag != tag) {
                range.start = ptr;
                range.tag = tag;
            }

            if (ptr - range.start) {
                time.push_back(payload::time(StringView{range.start, ptr}));
            }
        }
    };

    /*!stags:re2c format = 'const char *@@{tag};\n'; */
    /*!mtags:re2c format = 'int @@{tag} { Root };\n'; */

    /*!re2c

      re2c:eof = 0;
      re2c:api:style = free-form;
      re2c:flags:tags = 1;
      re2c:yyfill:enable = 0;
      re2c:define:YYMTAGP = "update_range(YYCURSOR, @@{tag}++);";
      re2c:define:YYMTAGN = "update_range(nullptr, @@{tag}++);";
      re2c:define:YYFILL = "goto return_out;";
      re2c:define:YYCTYPE = "char";

      @f0 [0-9]+ @f1 [:]
      @s0 [0-9]+ @s1 [:]
      @d0 [0-9]+ @d1 [:]
      (#t0 [0-9]+ #t1 [,]?)+ {
          if ((t0 != Root) && (t1 != Root)) {
              goto update_out;
          }
          goto return_out;
      }

      * { goto return_out; }
      $ { goto return_out; }
    */

update_out:
    {
        out = prepare(
            StringView{f0, f1},
            StringView{s0, s1},
            StringView{d0, d1},
            std::move(time));
    }

return_out:
    return out;
}
