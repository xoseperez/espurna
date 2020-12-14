#pragma once

#if GARLAND_SUPPORT

#include <vector>

#include "color.h"

class Palette {
   public:
    Palette(String name, std::vector<Color> colors) : _name(name), _colors(colors), numColors(colors.size()) {}

    String name() { return _name; }

    /**
    * Get the interpolated color from the palette.
    * The argument is a floating number between 0 and 1
    * Used to smoothly traverse through palette.
    */
    Color getPalColor(float i) {
        int i0 = (int)(i * numColors) % (numColors);
        int i1 = (int)(i * numColors + 1) % (numColors);

        // decimal part is used to interpolate between the two colors
        float t0 = i * numColors - trunc(i * numColors);
        return _colors[i0].interpolate(_colors[i1], t0);
    }

    /**
    * Get the interpolated color between two random neighbour colors.
    */
    Color getRndNeighborInterpColor() {
        int i0 = secureRandom(numColors);
        int i1 = (i0 + 1) % (numColors);

        float t0 = (float)(secureRandom(256)) / 256;
        return _colors[i0].interpolate(_colors[i1], t0);
    }

    /**
    * Get the interpolated color between two random colors.
    */
    Color getRndInterpColor() {
        int i0 = secureRandom(numColors);
        int i1 = secureRandom(numColors);

        float t0 = (float)(secureRandom(256)) / 256;
        return _colors[i0].interpolate(_colors[i1], t0);
    }

   private:
    const String _name;
    std::vector<Color> _colors;
    int numColors;
};

#endif  // GARLAND_SUPPORT
