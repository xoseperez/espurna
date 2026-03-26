/*

Part of BITS MODULE

*/

#pragma once

#include "bits.h"
#include "types.h"
#include "utils.h"

namespace espurna {
namespace bits {

bool fill_range(Range& range, StringView view) {
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

} // namespace bits
} // namespace espurna
