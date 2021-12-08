/*

Part of the TERMINAL MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include <vector>
#include <cctype>

#include "terminal_parsing.h"

namespace terminal {
namespace parsing {


// c/p with minor modifications from redis / sds, so that we don't have to roll a custom parser
// ref:
// - https://github.com/antirez/sds/blob/master/sds.c
// - https://github.com/antirez/redis/blob/unstable/src/networking.c
//
// Things are kept mostly the same, we are replacing Redis-specific things:
// - sds structure -> String
// - sds array -> std::vector<String>
// - we return always return custom structure, nullptr can no longer be used
//   to notify about the missing / unterminated / mismatching quotes
// - hex_... function helpers types are changed

// Original code is part of the SDSLib 2.0 -- A C dynamic strings library
//  *
//  * Copyright (c) 2006-2015, Salvatore Sanfilippo <antirez at gmail dot com>
//  * Copyright (c) 2015, Oran Agra
//  * Copyright (c) 2015, Redis Labs, Inc
//  * All rights reserved.
//  *
//  * Redistribution and use in source and binary forms, with or without
//  * modification, are permitted provided that the following conditions are met:
//  *
//  *   * Redistributions of source code must retain the above copyright notice,
//  *     this list of conditions and the following disclaimer.
//  *   * Redistributions in binary form must reproduce the above copyright
//  *     notice, this list of conditions and the following disclaimer in the
//  *     documentation and/or other materials provided with the distribution.
//  *   * Neither the name of Redis nor the names of its contributors may be used
//  *     to endorse or promote products derived from this software without
//  *     specific prior written permission.
//  *
//  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//  * POSSIBILITY OF SUCH DAMAGE.

// Helper functions to handle \xHH codes
static bool is_hex_digit(char c) {
    return (c >= '0' && c <= '9') \
         ||(c >= 'a' && c <= 'f') \
         ||(c >= 'A' && c <= 'F');
}

static char hex_digit_to_int(char c) {
    switch (c) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'a': case 'A': return 10;
        case 'b': case 'B': return 11;
        case 'c': case 'C': return 12;
        case 'd': case 'D': return 13;
        case 'e': case 'E': return 14;
        case 'f': case 'F': return 15;
        default: return 0;
    }
}

// Our port of `sdssplitargs`
CommandLine parse_commandline(const char *line) {
    const char *p = line;

    CommandLine out;
    out.argv.reserve(4);

    String current;

    while(1) {
        /* skip blanks */
        while(*p && isspace(*p)) p++;
        if (*p) {
            /* get a token */
            int inq=0;  /* set to 1 if we are in "quotes" */
            int insq=0; /* set to 1 if we are in 'single quotes' */
            int done=0;

            while(!done) {
                if (inq) {
                    if (*p == '\\' && *(p+1) == 'x' &&
                                             is_hex_digit(*(p+2)) &&
                                             is_hex_digit(*(p+3)))
                    {
                        // XXX: make sure that we append `char` or `char[]`,
                        // even with -funsigned-char this can accidentally append itoa conversion
                        unsigned char byte = 
                            (hex_digit_to_int(*(p+2))*16)+
                            hex_digit_to_int(*(p+3));
                        char buf[2] { static_cast<char>(byte), 0x00 };
                        current += buf;
                        p += 3;
                    } else if (*p == '\\' && *(p+1)) {
                        char c;

                        p++;
                        switch(*p) {
                        case 'n': c = '\n'; break;
                        case 'r': c = '\r'; break;
                        case 't': c = '\t'; break;
                        case 'b': c = '\b'; break;
                        case 'a': c = '\a'; break;
                        default: c = *p; break;
                        }
                        current += c;
                    } else if (*p == '"') {
                        /* closing quote must be followed by a space or
                         * nothing at all. */
                        if (*(p+1) && !isspace(*(p+1))) goto on_error;
                        done=1;
                    } else if (!*p) {
                        /* unterminated quotes */
                        goto on_error;
                    } else {
                        char buf[2] {*p, '\0'};
                        current += buf;
                    }
                } else if (insq) {
                    if (*p == '\\' && *(p+1) == '\'') {
                        p++;
                        current += '\'';
                    } else if (*p == '\'') {
                        /* closing quote must be followed by a space or
                         * nothing at all. */
                        if (*(p+1) && !isspace(*(p+1))) goto on_error;
                        done=1;
                    } else if (!*p) {
                        /* unterminated quotes */
                        goto on_error;
                    } else {
                        char buf[2] {*p, '\0'};
                        current += buf;
                    }
                } else {
                    switch(*p) {
                    case ' ':
                    case '\n':
                    case '\r':
                    case '\t':
                    case '\0':
                        done=1;
                        break;
                    case '"':
                        inq=1;
                        break;
                    case '\'':
                        insq=1;
                        break;
                    default: {
                        char buf[2] {*p, '\0'};
                        current += buf;
                        break;
                    }
                    }
                }
                if (*p) p++;
            }
            /* add the token to the vector */
            out.argv.emplace_back(std::move(current));
        } else {
            /* Even on empty input string return something not NULL. */
            goto on_out;
        }
    }

on_error:
    out.argv.clear();

on_out:
    return out;
}

// Fowler–Noll–Vo hash function to hash command strings that treats input as lowercase
// ref: https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
//
// This is here in case `std::unordered_map` becomes viable
// TODO: afaik, map implementation should handle collisions (however rare they are in our case)
// if not, we can always roll static commands allocation and just match strings with strcmp_P

uint32_t lowercase_fnv1_hash(const char* ptr) {
    constexpr uint32_t fnv_prime = 16777619u;
    constexpr uint32_t fnv_basis = 2166136261u;

    const auto length = strlen_P(ptr);

    uint32_t hash = fnv_basis;
    for (size_t idx = 0; idx < length; ++idx) {
        hash = hash ^ static_cast<uint32_t>(tolower(pgm_read_byte(&ptr[idx])));
        hash = hash * fnv_prime;
    }

    return hash;
}

uint32_t lowercase_fnv1_hash(const __FlashStringHelper* ptr) {
    return lowercase_fnv1_hash(reinterpret_cast<const char*>(ptr));
}

} // namespace parsing
} // namespace terminal
