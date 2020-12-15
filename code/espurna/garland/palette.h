#pragma once

#if GARLAND_SUPPORT

#include <vector>

#include "color.h"

class Palette {
   public:
    Palette(const String name, std::vector<Color> colors) : _name(name), _numColors(colors.size()), _colors(colors) {}

    String name() const { return _name; }

    /**
    * Get the interpolated color from the palette.
    * The argument is a floating number between 0 and 1
    * Used to smoothly traverse through palette.
    */
    // TODO: looks like this operation takes a long time and made often
    // for some animations.
    // It worth to implement cache or table for pre-calculated colors (256)
    Color getPalColor(float i) const {
        int i0 = (int)(i * _numColors) % (_numColors);
        int i1 = (int)(i * _numColors + 1) % (_numColors);

        // decimal part is used to interpolate between the two colors
        float t0 = i * _numColors - trunc(i * _numColors);
        return _colors[i0].interpolate(_colors[i1], t0);
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

   private:
    const String _name;
    const int _numColors;
    std::vector<Color> _colors;
};

#endif  // GARLAND_SUPPORT
