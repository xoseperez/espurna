#ifndef palette_h
#define palette_h

#include "color.h"


struct Palette
{
    int numColors;
    Color *colors;
    
    /**
    * Get the interpolated color from the palette.
    * The argument is a floating number between 0 and 1
    */
    Color getPalColor(float i)
    {
        int i0 = (int)(i*numColors)%(numColors);
        int i1 = (int)(i*numColors+1)%(numColors);
        
        // decimal part is used to interpolate between the two colors
        float t0 = i*numColors - trunc(i*numColors);

        return colors[i0].interpolate(colors[i1], t0);
    }
       
};

////////////////////////////////////////////////////////////////////////////////
// Palette definitions
////////////////////////////////////////////////////////////////////////////////
extern Palette PalRgb;
extern Palette PalRainbow;
extern Palette PalRainbowStripe;
extern Palette PalParty;
extern Palette PalHeat;
extern Palette PalFire;
extern Palette PalIceBlue;

#endif