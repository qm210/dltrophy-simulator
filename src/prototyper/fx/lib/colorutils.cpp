/// from FastLED colorutils.cpp
/// Utility functions for color fill, palettes, blending, and more

#include "colorutils.h"

void fill_solid( struct CRGB * targetArray, int numToFill,
                        const struct CRGB& color)
{
    for( int i = 0; i < numToFill; ++i) {
        targetArray[i] = color;
    }
}

void fill_solid( struct CHSV * targetArray, int numToFill,
                        const struct CHSV& color)
{
    for( int i = 0; i < numToFill; ++i) {
        targetArray[i] = color;
    }
}

void fill_rainbow( struct CRGB * targetArray, int numToFill,
                          uint8_t initialhue,
                          uint8_t deltahue )
{
    CHSV hsv;
    hsv.hue = initialhue;
    hsv.val = 255;
    hsv.sat = 240;
    for( int i = 0; i < numToFill; ++i) {
        targetArray[i] = hsv;
        hsv.hue += deltahue;
    }
}

void fill_rainbow( struct CHSV * targetArray, int numToFill,
                          uint8_t initialhue,
                          uint8_t deltahue )
{
    CHSV hsv;
    hsv.hue = initialhue;
    hsv.val = 255;
    hsv.sat = 240;
    for( int i = 0; i < numToFill; ++i) {
        targetArray[i] = hsv;
        hsv.hue += deltahue;
    }
}

void fill_rainbow_circular(struct CRGB* targetArray, int numToFill, uint8_t initialhue, bool reversed)
{
    if (numToFill == 0) return;  // avoiding div/0

    CHSV hsv;
    hsv.hue = initialhue;
    hsv.val = 255;
    hsv.sat = 240;

    const uint16_t hueChange = 65535 / (uint16_t)numToFill;  // hue change for each LED, * 256 for precision (256 * 256 - 1)
    uint16_t hueOffset = 0;  // offset for hue value, with precision (*256)

    for (int i = 0; i < numToFill; ++i) {
        targetArray[i] = hsv;
        if (reversed) hueOffset -= hueChange;
        else hueOffset += hueChange;
        hsv.hue = initialhue + (uint8_t)(hueOffset >> 8);  // assign new hue with precise offset (as 8-bit)
    }
}

void fill_rainbow_circular(struct CHSV* targetArray, int numToFill, uint8_t initialhue, bool reversed)
{
    if (numToFill == 0) return;  // avoiding div/0

    CHSV hsv;
    hsv.hue = initialhue;
    hsv.val = 255;
    hsv.sat = 240;

    const uint16_t hueChange = 65535 / (uint16_t) numToFill;  // hue change for each LED, * 256 for precision (256 * 256 - 1)
    uint16_t hueOffset = 0;  // offset for hue value, with precision (*256)

    for (int i = 0; i < numToFill; ++i) {
        targetArray[i] = hsv;
        if (reversed) hueOffset -= hueChange;
        else hueOffset += hueChange;
        hsv.hue = initialhue + (uint8_t)(hueOffset >> 8);  // assign new hue with precise offset (as 8-bit)
    }
}


void fill_gradient_RGB( CRGB* leds,
                               uint16_t startpos, CRGB startcolor,
                               uint16_t endpos,   CRGB endcolor )
{
    // if the points are in the wrong order, straighten them
    if( endpos < startpos ) {
        uint16_t t = endpos;
        CRGB tc = endcolor;
        endcolor = startcolor;
        endpos = startpos;
        startpos = t;
        startcolor = tc;
    }

    saccum87 rdistance87;
    saccum87 gdistance87;
    saccum87 bdistance87;

    rdistance87 = (endcolor.r - startcolor.r) << 7;
    gdistance87 = (endcolor.g - startcolor.g) << 7;
    bdistance87 = (endcolor.b - startcolor.b) << 7;

    uint16_t pixeldistance = endpos - startpos;
    int16_t divisor = pixeldistance ? pixeldistance : 1;

    saccum87 rdelta87 = rdistance87 / divisor;
    saccum87 gdelta87 = gdistance87 / divisor;
    saccum87 bdelta87 = bdistance87 / divisor;

    rdelta87 *= 2;
    gdelta87 *= 2;
    bdelta87 *= 2;

    accum88 r88 = startcolor.r << 8;
    accum88 g88 = startcolor.g << 8;
    accum88 b88 = startcolor.b << 8;
    for( uint16_t i = startpos; i <= endpos; ++i) {
        leds[i] = CRGB( r88 >> 8, g88 >> 8, b88 >> 8);
        r88 += rdelta87;
        g88 += gdelta87;
        b88 += bdelta87;
    }
}


void fill_gradient_RGB( CRGB* leds, uint16_t numLeds, const CRGB& c1, const CRGB& c2)
{
    uint16_t last = numLeds - 1;
    fill_gradient_RGB( leds, 0, c1, last, c2);
}


void fill_gradient_RGB( CRGB* leds, uint16_t numLeds, const CRGB& c1, const CRGB& c2, const CRGB& c3)
{
    uint16_t half = (numLeds / 2);
    uint16_t last = numLeds - 1;
    fill_gradient_RGB( leds,    0, c1, half, c2);
    fill_gradient_RGB( leds, half, c2, last, c3);
}

void fill_gradient_RGB( CRGB* leds, uint16_t numLeds, const CRGB& c1, const CRGB& c2, const CRGB& c3, const CRGB& c4)
{
    uint16_t onethird = (numLeds / 3);
    uint16_t twothirds = ((numLeds * 2) / 3);
    uint16_t last = numLeds - 1;
    fill_gradient_RGB( leds,         0, c1,  onethird, c2);
    fill_gradient_RGB( leds,  onethird, c2, twothirds, c3);
    fill_gradient_RGB( leds, twothirds, c3,      last, c4);
}

inline uint8_t lsrX4( uint8_t dividend) __attribute__((always_inline));
inline uint8_t lsrX4( uint8_t dividend)
{
#if defined(__AVR__)
    dividend /= 2;
    dividend /= 2;
    dividend /= 2;
    dividend /= 2;
#else
    dividend >>= 4;
#endif
    return dividend;
}

CRGB ColorFromPalette( const CRGBPalette16& pal, uint8_t index, uint8_t brightness, TBlendType blendType)
{
    if ( blendType == LINEARBLEND_NOWRAP) {
        index = map8(index, 0, 239);  // Blend range is affected by lo4 blend of values, remap to avoid wrapping
    }

    //      hi4 = index >> 4;
    uint8_t hi4 = lsrX4(index);
    uint8_t lo4 = index & 0x0F;

    // const CRGB* entry = &(pal[0]) + hi4;
    // since hi4 is always 0..15, hi4 * sizeof(CRGB) can be a single-byte value,
    // instead of the two byte 'int' that avr-gcc defaults to.
    // So, we multiply hi4 X sizeof(CRGB), giving hi4XsizeofCRGB;
    uint8_t hi4XsizeofCRGB = hi4 * sizeof(CRGB);
    // We then add that to a base array pointer.
    const CRGB* entry = (CRGB*)( (uint8_t*)(&(pal[0])) + hi4XsizeofCRGB);

    uint8_t blend = lo4 && (blendType != NOBLEND);

    uint8_t red1   = entry->red;
    uint8_t green1 = entry->green;
    uint8_t blue1  = entry->blue;


    if( blend ) {

        if( hi4 == 15 ) {
            entry = &(pal[0]);
        } else {
            ++entry;
        }

        uint8_t f2 = lo4 << 4;
        uint8_t f1 = 255 - f2;

        //    rgb1.nscale8(f1);
        uint8_t red2   = entry->red;
        red1   = scale8_LEAVING_R1_DIRTY( red1,   f1);
        red2   = scale8_LEAVING_R1_DIRTY( red2,   f2);
        red1   += red2;

        uint8_t green2 = entry->green;
        green1 = scale8_LEAVING_R1_DIRTY( green1, f1);
        green2 = scale8_LEAVING_R1_DIRTY( green2, f2);
        green1 += green2;

        uint8_t blue2  = entry->blue;
        blue1  = scale8_LEAVING_R1_DIRTY( blue1,  f1);
        blue2  = scale8_LEAVING_R1_DIRTY( blue2,  f2);
        blue1  += blue2;

        cleanup_R1();
    }

    if( brightness != 255) {
        if( brightness ) {
            ++brightness; // adjust for rounding
            // Now, since brightness is nonzero, we don't need the full scale8_video logic;
            // we can just to scale8 and then add one (unless scale8 fixed) to all nonzero inputs.
            if( red1 )   {
                red1 = scale8_LEAVING_R1_DIRTY( red1, brightness);
#if !(FASTLED_SCALE8_FIXED==1)
                ++red1;
#endif
            }
            if( green1 ) {
                green1 = scale8_LEAVING_R1_DIRTY( green1, brightness);
#if !(FASTLED_SCALE8_FIXED==1)
                ++green1;
#endif
            }
            if( blue1 )  {
                blue1 = scale8_LEAVING_R1_DIRTY( blue1, brightness);
#if !(FASTLED_SCALE8_FIXED==1)
                ++blue1;
#endif
            }
            cleanup_R1();
        } else {
            red1 = 0;
            green1 = 0;
            blue1 = 0;
        }
    }

    return CRGB( red1, green1, blue1);
}

CRGB ColorFromPalette( const TProgmemRGBPalette16& pal, uint8_t index, uint8_t brightness, TBlendType blendType)
{
    if ( blendType == LINEARBLEND_NOWRAP) {
        index = map8(index, 0, 239);  // Blend range is affected by lo4 blend of values, remap to avoid wrapping
    }

    //      hi4 = index >> 4;
    uint8_t hi4 = lsrX4(index);
    uint8_t lo4 = index & 0x0F;

    CRGB entry   =  FL_PGM_READ_DWORD_NEAR( &(pal[0]) + hi4 );


    uint8_t red1   = entry.red;
    uint8_t green1 = entry.green;
    uint8_t blue1  = entry.blue;

    uint8_t blend = lo4 && (blendType != NOBLEND);

    if( blend ) {

        if( hi4 == 15 ) {
            entry =   FL_PGM_READ_DWORD_NEAR( &(pal[0]) );
        } else {
            entry =   FL_PGM_READ_DWORD_NEAR( &(pal[1]) + hi4 );
        }

        uint8_t f2 = lo4 << 4;
        uint8_t f1 = 255 - f2;

        uint8_t red2   = entry.red;
        red1   = scale8_LEAVING_R1_DIRTY( red1,   f1);
        red2   = scale8_LEAVING_R1_DIRTY( red2,   f2);
        red1   += red2;

        uint8_t green2 = entry.green;
        green1 = scale8_LEAVING_R1_DIRTY( green1, f1);
        green2 = scale8_LEAVING_R1_DIRTY( green2, f2);
        green1 += green2;

        uint8_t blue2  = entry.blue;
        blue1  = scale8_LEAVING_R1_DIRTY( blue1,  f1);
        blue2  = scale8_LEAVING_R1_DIRTY( blue2,  f2);
        blue1  += blue2;

        cleanup_R1();
    }

    if( brightness != 255) {
        if( brightness ) {
            ++brightness; // adjust for rounding
            // Now, since brightness is nonzero, we don't need the full scale8_video logic;
            // we can just to scale8 and then add one (unless scale8 fixed) to all nonzero inputs.
            if( red1 )   {
                red1 = scale8_LEAVING_R1_DIRTY( red1, brightness);
#if !(FASTLED_SCALE8_FIXED==1)
                ++red1;
#endif
            }
            if( green1 ) {
                green1 = scale8_LEAVING_R1_DIRTY( green1, brightness);
#if !(FASTLED_SCALE8_FIXED==1)
                ++green1;
#endif
            }
            if( blue1 )  {
                blue1 = scale8_LEAVING_R1_DIRTY( blue1, brightness);
#if !(FASTLED_SCALE8_FIXED==1)
                ++blue1;
#endif
            }
            cleanup_R1();
        } else {
            red1 = 0;
            green1 = 0;
            blue1 = 0;
        }
    }

    return CRGB( red1, green1, blue1);
}

CHSV ColorFromPalette( const CHSVPalette16& pal, uint8_t index, uint8_t brightness, TBlendType blendType)
{
    if ( blendType == LINEARBLEND_NOWRAP) {
        index = map8(index, 0, 239);  // Blend range is affected by lo4 blend of values, remap to avoid wrapping
    }

    //      hi4 = index >> 4;
    uint8_t hi4 = lsrX4(index);
    uint8_t lo4 = index & 0x0F;

    //  CRGB rgb1 = pal[ hi4];
    const CHSV* entry = &(pal[0]) + hi4;

    uint8_t hue1   = entry->hue;
    uint8_t sat1   = entry->sat;
    uint8_t val1   = entry->val;

    uint8_t blend = lo4 && (blendType != NOBLEND);

    if( blend ) {

        if( hi4 == 15 ) {
            entry = &(pal[0]);
        } else {
            ++entry;
        }

        uint8_t f2 = lo4 << 4;
        uint8_t f1 = 255 - f2;

        uint8_t hue2  = entry->hue;
        uint8_t sat2  = entry->sat;
        uint8_t val2  = entry->val;

        // Now some special casing for blending to or from
        // either black or white.  Black and white don't have
        // proper 'hue' of their own, so when ramping from
        // something else to/from black/white, we set the 'hue'
        // of the black/white color to be the same as the hue
        // of the other color, so that you get the expected
        // brightness or saturation ramp, with hue staying
        // constant:

        // If we are starting from white (sat=0)
        // or black (val=0), adopt the target hue.
        if( sat1 == 0 || val1 == 0) {
            hue1 = hue2;
        }

        // If we are ending at white (sat=0)
        // or black (val=0), adopt the starting hue.
        if( sat2 == 0 || val2 == 0) {
            hue2 = hue1;
        }


        sat1  = scale8_LEAVING_R1_DIRTY( sat1, f1);
        val1  = scale8_LEAVING_R1_DIRTY( val1, f1);

        sat2  = scale8_LEAVING_R1_DIRTY( sat2, f2);
        val2  = scale8_LEAVING_R1_DIRTY( val2, f2);

        //    cleanup_R1();

        // These sums can't overflow, so no qadd8 needed.
        sat1  += sat2;
        val1  += val2;

        uint8_t deltaHue = (uint8_t)(hue2 - hue1);
        if( deltaHue & 0x80 ) {
            // go backwards
            hue1 -= scale8( 256 - deltaHue, f2);
        } else {
            // go forwards
            hue1 += scale8( deltaHue, f2);
        }

        cleanup_R1();
    }

    if( brightness != 255) {
        val1 = scale8_video( val1, brightness);
    }

    return CHSV( hue1, sat1, val1);
}
