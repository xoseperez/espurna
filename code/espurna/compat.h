/*

COMPATIBILITY BETWEEN 2.3.0 and latest versions

*/

#pragma once

#include "espurna.h"

// -----------------------------------------------------------------------------
// Core version 2.4.2 and higher changed the cont_t structure to a pointer:
// https://github.com/esp8266/Arduino/commit/5d5ea92a4d004ab009d5f642629946a0cb8893dd#diff-3fa12668b289ccb95b7ab334833a4ba8L35
// Core version 2.5.0 introduced EspClass helper method:
// https://github.com/esp8266/Arduino/commit/0e0e34c614fe8a47544c9998201b1d9b3c24eb18
// -----------------------------------------------------------------------------

extern "C" {
    #include <cont.h>
#if defined(ARDUINO_ESP8266_RELEASE_2_3_0) \
    || defined(ARDUINO_ESP8266_RELEASE_2_4_0) \
    || defined(ARDUINO_ESP8266_RELEASE_2_4_1)
    extern cont_t g_cont;
    #define getFreeStack() cont_get_free_stack(&g_cont)
#elif defined(ARDUINO_ESP8266_RELEASE_2_4_2)
    extern cont_t* g_pcont;
    #define getFreeStack() cont_get_free_stack(g_pcont)
#else
    #define getFreeStack() ESP.getFreeContStack()
#endif
}

#include <pgmspace.h>

// -----------------------------------------------------------------------------
// ref: https://github.com/esp8266/Arduino/blob/master/tools/sdk/libc/xtensa-lx106-elf/include/sys/pgmspace.h
// __STRINGIZE && __STRINGIZE_NX && PROGMEM definitions port
// -----------------------------------------------------------------------------

// Do not replace macros unless running version older than 2.5.0
#if defined(ARDUINO_ESP8266_RELEASE_2_3_0) \
    || defined(ARDUINO_ESP8266_RELEASE_2_4_0) \
    || defined(ARDUINO_ESP8266_RELEASE_2_4_1) \
    || defined(ARDUINO_ESP8266_RELEASE_2_4_2)

// Quoting esp8266/Arduino comments:
// "Since __section__ is supposed to be only use for global variables,
// there could be conflicts when a static/inlined function has them in the
// same file as a non-static PROGMEM object.
// Ref: https://gcc.gnu.org/onlinedocs/gcc-3.2/gcc/Variable-Attributes.html
// Place each progmem object into its own named section, avoiding conflicts"

#define __TO_STR_(A) #A
#define __TO_STR(A) __TO_STR_(A)

#undef PROGMEM
#define PROGMEM __attribute__((section( "\".irom.text." __FILE__ "." __TO_STR(__LINE__) "."  __TO_STR(__COUNTER__) "\"")))

// "PSTR() macro modified to start on a 32-bit boundary.  This adds on average
// 1.5 bytes/string, but in return memcpy_P and strcpy_P will work 4~8x faster"
#undef PSTR
#define PSTR(s) (__extension__({static const char __c[] __attribute__((__aligned__(4))) PROGMEM = (s); &__c[0];}))

#endif

// -----------------------------------------------------------------------------
// Division by zero bug
// https://github.com/esp8266/Arduino/pull/2397
// https://github.com/esp8266/Arduino/pull/2408
// -----------------------------------------------------------------------------

#if defined(ARDUINO_ESP8266_RELEASE_2_3_0)
long  __attribute__((deprecated("Please avoid using map() with Core 2.3.0"))) map(long x, long in_min, long in_max, long out_min, long out_max);
#endif

// -----------------------------------------------------------------------------
// Proxy min & max same as the latest Arduino.h
// -----------------------------------------------------------------------------

#if defined(ARDUINO_ESP8266_RELEASE_2_3_0)

#undef min
#undef max
#undef _min
#undef _max

#include <algorithm>

using std::min;
using std::max;
using std::isinf;
using std::isnan;

#define _min(a,b) ({ decltype(a) _a = (a); decltype(b) _b = (b); _a < _b? _a : _b; })
#define _max(a,b) ({ decltype(a) _a = (a); decltype(b) _b = (b); _a > _b? _a : _b; })

#endif

// -----------------------------------------------------------------------------
// std::make_unique backport for C++11, since we still use it
// -----------------------------------------------------------------------------
#if 201103L >= __cplusplus

#include <memory>
namespace std {
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args&&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}

#endif

#define UNUSED(x) (void)(x)

