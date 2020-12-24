/*
Part of the GARLAND MODULE
Copyright (C) 2020 by Dmitry Blinov <dblinov76 at gmail dot com>

Inspired by https://github.com/Vasil-Pahomov/ArWs2812 (currently https://github.com/Vasil-Pahomov/Liana)
*/

#pragma once

#if GARLAND_SUPPORT

#include <Arduino.h>

struct Color
{
    byte r;
    byte g;
    byte b;

    inline Color() : r(0), g(0), b(0) {}

    // allow construction from R, G, B
    inline Color(uint8_t ir, uint8_t ig, uint8_t ib)
        : r(ir), g(ig), b(ib) {
    }

    // allow construction from 32-bit (really 24-bit) bit 0xRRGGBB color code
    inline Color(uint32_t colorcode)
        : r((colorcode >> 16) & 0xFF), g((colorcode >> 8) & 0xFF), b((colorcode >> 0) & 0xFF) {
    }

    //interpolates between this color and provided. 
    //x is from 0 to 1, 0 gives this color, 1 gives provided color, values between give interpolation
    Color interpolate(Color color, float x) const {
        int r0 = x * (color.r - r) + r;
        int g0 = x * (color.g - g) + g;
        int b0 = x * (color.b - b) + b;
        return Color(r0, g0, b0);
    }

    //creates color with decreased brightness
    Color brightness(byte k) const {
        int r0 = r * (int)k / 255;
        int g0 = g * (int)k / 255;
        int b0 = b * (int)k / 255;
        return Color(r0, g0, b0);
    }

    //fades (decreases all RGB channels brightness) this color by k
    void fade(byte k) {
        if (r>=k) { r=r-k; } else { r=0; }
        if (g>=k) { g=g-k; } else { g=0; }
        if (b>=k) { b=b-k; } else { b=0; }
    }

    //fades color separately for each channel
    void fade3(byte dr, byte dg, byte db) {
        if (r>=dr) { r=r-dr; } else { r=0; }
        if (g>=dg) { g=g-dg; } else { g=0; }
        if (b>=db) { b=b-db; } else { b=0; }
    }  

    //checks whether this color is visually close to given one
    bool isCloseTo(Color c) const {
        int diff = abs(r - c.r) + abs(g - c.g) + abs(b - c.b);
        return diff <= 220;  //220 is magic number. Low values give "true" on closer colors, while higher can cause infinite loop while trying to find different color
    }

    //return value, that can be used to define how close one color to another
    int howCloseTo(Color c) const {
        return abs(r - c.r) + abs(g - c.g) + abs(b - c.b);
    }

    bool empty() const {
        return r == 0 && g == 0 && b == 0;
    }

    void println() const {
        Serial.print(("r="));Serial.print(r);Serial.print((" "));
        Serial.print(("g="));Serial.print(g);Serial.print( (" "));
        Serial.print(("b="));Serial.println(b);
    }

    friend bool operator== (const Color &c1, const Color &c2);
};

#endif // GARLAND_SUPPORT
