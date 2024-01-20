/*
Part of the GARLAND MODULE
Copyright (C) 2020 by Dmitry Blinov <dblinov76 at gmail dot com>

Inspired by https://github.com/Vasil-Pahomov/ArWs2812 (currently https://github.com/Vasil-Pahomov/Liana)
*/

#pragma once

#include "color.h"

class Palette {
   public:
    Palette(const char* name, bool bright, std::vector<Color>&& colors) : _name(name), _bright(bright), _numColors(colors.size()), _colors(std::move(colors)), _cache(256) {
    }

    const char* name() const { return _name; }
    const bool bright() const { return _bright; }

    /**
    * Get the interpolated color from the palette.
    * The argument is a floating number between 0 and 1
    * Used to smoothly traverse through palette.
    */
    Color getPalColor(float i) const {
        int i0 = (int)(i * _numColors) % (_numColors);
        int i1 = (int)(i * _numColors + 1) % (_numColors);

        // decimal part is used to interpolate between the two colors
        float t0 = i * _numColors - trunc(i * _numColors);
        return _colors[i0].interpolate(_colors[i1], t0);
    }

    Color getCachedPalColor(byte i) {
        if (!_cache[i].is_empty())
            return _cache[i];

        Color col = getPalColor((float)i / 256);
        if (col.is_empty())
            col = 1;

        _cache[i] = col;
        return col;
    }

    /**
    * Get the interpolated color between two random neighbour colors.
    */
    Color getRndNeighborInterpColor() const {
        int i0 = secureRandom(_numColors);
        int i1 = (i0 + 1) % (_numColors);

        float t0 = (float)(secureRandom(256)) / 256;
        return _colors[i0].interpolate(_colors[i1], t0);
    }

    /**
    * Get the interpolated color between two random colors.
    */
    Color getRndInterpColor() const {
        int i0 = secureRandom(_numColors);
        int i1 = secureRandom(_numColors);

        float t0 = (float)(secureRandom(256)) / 256;
        return _colors[i0].interpolate(_colors[i1], t0);
    }

    Color getContrastColor(const Color& prevColor) const {
        int tries = 0;
        int bestDiff = 0;
        Color bestColor;
        // 220 is magic number. Low values give "true" on closer colors, while higher can cause infinite loop while trying to find different color
        // let's try to find contras enough color but no more than 10 tries
        while (bestDiff <= 220 && tries++ < 10) {
            Color newColor = getRndInterpColor();
            int diff = prevColor.howCloseTo(newColor);
            if (bestDiff < diff) {
                bestDiff = diff;
                bestColor = newColor;
            }
        }
        return bestColor;
    }

    Color getCleanColor(uint16_t i) const {
        return _colors[i % _numColors];
    }

    Color getCloseCleanColor(const Color& refColor) const {
        int bestDiff = 0;
        Color bestColor;
        
        for (auto i = 0; i < _numColors; ++i) {
            Color newColor = _colors[i];
            int diff = refColor.howCloseTo(newColor);
            if (bestDiff < diff) {
                bestDiff = diff;
                bestColor = newColor;
            }
        }
        return bestColor;
    }

    Color getRndCleanColor() const {
        return _colors[secureRandom(_numColors)];
    }


   private:
    const char* _name;
    const bool _bright;
    const uint16_t _numColors;
    std::vector<Color> _colors;
    std::vector<Color> _cache;
};
