#pragma once

/*
 * WARNING by qm:
 *   Check any other of these lib/ headers for my warning about these very headers.
 *   i.e. would be headless to use these headers elsewhere; heading towards disaster.
 *   Heed my warning.
 */

#include "pixeltypes.h"
#include "lib8tion.h"
#include "fastled_defines.h"

// Notiz qm210:
//     puh... diggi... was brauchen wir hier überhaupt von... anyway.
//     "Stealin' is appealin'" - qm, 2015

#define saccum87 int16_t

void fill_solid( struct CRGB * targetArray, int numToFill,
                 const struct CRGB& color);
void fill_solid( struct CHSV* targetArray, int numToFill,
                 const struct CHSV& color);
void fill_rainbow( struct CRGB * targetArray, int numToFill,
                   uint8_t initialhue,
                   uint8_t deltahue = 5);
void fill_rainbow( struct CHSV * targetArray, int numToFill,
                   uint8_t initialhue,
                   uint8_t deltahue = 5);
void fill_rainbow_circular(struct CRGB* targetArray, int numToFill,
                           uint8_t initialhue, bool reversed=false);
void fill_rainbow_circular(struct CHSV* targetArray, int numToFill,
                           uint8_t initialhue, bool reversed=false);
void fill_gradient_RGB( CRGB* leds,
                        uint16_t startpos, CRGB startcolor,
                        uint16_t endpos,   CRGB endcolor );
void fill_gradient_RGB( CRGB* leds, uint16_t numLeds, const CRGB& c1, const CRGB& c2);
void fill_gradient_RGB( CRGB* leds, uint16_t numLeds, const CRGB& c1, const CRGB& c2, const CRGB& c3);
void fill_gradient_RGB( CRGB* leds, uint16_t numLeds, const CRGB& c1, const CRGB& c2, const CRGB& c3, const CRGB& c4);


class CRGBPalette16;
class CHSVPalette16;

typedef uint32_t TProgmemRGBPalette16[16];  ///< CRGBPalette16 entries stored in PROGMEM memory
typedef uint32_t TProgmemHSVPalette16[16];  ///< CHSVPalette16 entries stored in PROGMEM memory
/// Alias for TProgmemRGBPalette16
#define TProgmemPalette16 TProgmemRGBPalette16
typedef uint32_t TProgmemRGBPalette32[32];  ///< CRGBPalette32 entries stored in PROGMEM memory
typedef uint32_t TProgmemHSVPalette32[32];  ///< CHSVPalette32 entries stored in PROGMEM memory
/// Alias for TProgmemRGBPalette32
#define TProgmemPalette32 TProgmemRGBPalette32

/// Byte of an RGB gradient, stored in PROGMEM memory
typedef const uint8_t TProgmemRGBGradientPalette_byte;
/// Pointer to bytes of an RGB gradient, stored in PROGMEM memory
/// @see DEFINE_GRADIENT_PALETTE
/// @see DECLARE_GRADIENT_PALETTE
typedef const TProgmemRGBGradientPalette_byte *TProgmemRGBGradientPalette_bytes;
/// Alias of ::TProgmemRGBGradientPalette_bytes
typedef TProgmemRGBGradientPalette_bytes TProgmemRGBGradientPalettePtr;

/// Struct for digesting gradient pointer data into its components.
/// This is used when loading a gradient stored in PROGMEM or on
/// the heap into a palette. The pointer is dereferenced and interpreted as
/// this struct, so the component parts can be addressed and copied by name.
typedef union {
    struct {
        uint8_t index;  ///< index of the color entry in the gradient
        uint8_t r;      ///< CRGB::red channel value of the color entry
        uint8_t g;      ///< CRGB::green channel value of the color entry
        uint8_t b;      ///< CRGB::blue channel value of the color entry
    };
    uint32_t dword;     ///< values as a packed 32-bit double word
    uint8_t  bytes[4];  ///< values as an array
} TRGBGradientPaletteEntryUnion;

typedef uint8_t TDynamicRGBGradientPalette_byte;  ///< Byte of an RGB gradient entry, stored in dynamic (heap) memory
typedef const TDynamicRGBGradientPalette_byte *TDynamicRGBGradientPalette_bytes;  ///< Pointer to bytes of an RGB gradient, stored in dynamic (heap) memory
typedef TDynamicRGBGradientPalette_bytes TDynamicRGBGradientPalettePtr;  ///< Alias of ::TDynamicRGBGradientPalette_bytes


typedef enum {
    FORWARD_HUES,   ///< Hue always goes clockwise around the color wheel
    BACKWARD_HUES,  ///< Hue always goes counter-clockwise around the color wheel
    SHORTEST_HUES,  ///< Hue goes whichever way is shortest
    LONGEST_HUES    ///< Hue goes whichever way is longest
} TGradientDirectionCode;
template <typename T>
void fill_gradient( T* targetArray,
                    uint16_t startpos, CHSV startcolor,
                    uint16_t endpos,   CHSV endcolor,
                    TGradientDirectionCode directionCode  = SHORTEST_HUES )
{
    // if the points are in the wrong order, straighten them
    if( endpos < startpos ) {
        uint16_t t = endpos;
        CHSV tc = endcolor;
        endcolor = startcolor;
        endpos = startpos;
        startpos = t;
        startcolor = tc;
    }

    // If we're fading toward black (val=0) or white (sat=0),
    // then set the endhue to the starthue.
    // This lets us ramp smoothly to black or white, regardless
    // of what 'hue' was set in the endcolor (since it doesn't matter)
    if( endcolor.value == 0 || endcolor.saturation == 0) {
        endcolor.hue = startcolor.hue;
    }

    // Similarly, if we're fading in from black (val=0) or white (sat=0)
    // then set the starthue to the endhue.
    // This lets us ramp smoothly up from black or white, regardless
    // of what 'hue' was set in the startcolor (since it doesn't matter)
    if( startcolor.value == 0 || startcolor.saturation == 0) {
        startcolor.hue = endcolor.hue;
    }

    saccum87 huedistance87;
    saccum87 satdistance87;
    saccum87 valdistance87;

    satdistance87 = (endcolor.sat - startcolor.sat) << 7;
    valdistance87 = (endcolor.val - startcolor.val) << 7;

    uint8_t huedelta8 = endcolor.hue - startcolor.hue;

    if( directionCode == SHORTEST_HUES ) {
        directionCode = FORWARD_HUES;
        if( huedelta8 > 127) {
            directionCode = BACKWARD_HUES;
        }
    }

    if( directionCode == LONGEST_HUES ) {
        directionCode = FORWARD_HUES;
        if( huedelta8 < 128) {
            directionCode = BACKWARD_HUES;
        }
    }

    if( directionCode == FORWARD_HUES) {
        huedistance87 = huedelta8 << 7;
    }
    else /* directionCode == BACKWARD_HUES */
    {
        huedistance87 = (uint8_t)(256 - huedelta8) << 7;
        huedistance87 = -huedistance87;
    }

    uint16_t pixeldistance = endpos - startpos;
    int16_t divisor = pixeldistance ? pixeldistance : 1;

    saccum87 huedelta87 = huedistance87 / divisor;
    saccum87 satdelta87 = satdistance87 / divisor;
    saccum87 valdelta87 = valdistance87 / divisor;

    huedelta87 *= 2;
    satdelta87 *= 2;
    valdelta87 *= 2;

    accum88 hue88 = startcolor.hue << 8;
    accum88 sat88 = startcolor.sat << 8;
    accum88 val88 = startcolor.val << 8;
    for( uint16_t i = startpos; i <= endpos; ++i) {
        targetArray[i] = CHSV( hue88 >> 8, sat88 >> 8, val88 >> 8);
        hue88 += huedelta87;
        sat88 += satdelta87;
        val88 += valdelta87;
    }
}


/// Fill a range of LEDs with a smooth HSV gradient between two HSV colors.
/// @see fill_gradient()
/// @param targetArray a pointer to the color array to fill
/// @param numLeds the number of LEDs to fill
/// @param c1 the starting color in the gradient
/// @param c2 the end color for the gradient
/// @param directionCode the direction to travel around the color wheel
template <typename T>
void fill_gradient( T* targetArray, uint16_t numLeds, const CHSV& c1, const CHSV& c2,
                    TGradientDirectionCode directionCode = SHORTEST_HUES )
{
    uint16_t last = numLeds - 1;
    fill_gradient( targetArray, 0, c1, last, c2, directionCode);
}

/// Fill a range of LEDs with a smooth HSV gradient between three HSV colors.
/// @see fill_gradient()
/// @param targetArray a pointer to the color array to fill
/// @param numLeds the number of LEDs to fill
/// @param c1 the starting color in the gradient
/// @param c2 the middle color for the gradient
/// @param c3 the end color for the gradient
/// @param directionCode the direction to travel around the color wheel
template <typename T>
void fill_gradient( T* targetArray, uint16_t numLeds,
                    const CHSV& c1, const CHSV& c2, const CHSV& c3,
                    TGradientDirectionCode directionCode = SHORTEST_HUES )
{
    uint16_t half = (numLeds / 2);
    uint16_t last = numLeds - 1;
    fill_gradient( targetArray,    0, c1, half, c2, directionCode);
    fill_gradient( targetArray, half, c2, last, c3, directionCode);
}

/// Fill a range of LEDs with a smooth HSV gradient between four HSV colors.
/// @see fill_gradient()
/// @param targetArray a pointer to the color array to fill
/// @param numLeds the number of LEDs to fill
/// @param c1 the starting color in the gradient
/// @param c2 the first middle color for the gradient
/// @param c3 the second middle color for the gradient
/// @param c4 the end color for the gradient
/// @param directionCode the direction to travel around the color wheel
template <typename T>
void fill_gradient( T* targetArray, uint16_t numLeds,
                    const CHSV& c1, const CHSV& c2, const CHSV& c3, const CHSV& c4,
                    TGradientDirectionCode directionCode = SHORTEST_HUES )
{
    uint16_t onethird = (numLeds / 3);
    uint16_t twothirds = ((numLeds * 2) / 3);
    uint16_t last = numLeds - 1;
    fill_gradient( targetArray,         0, c1,  onethird, c2, directionCode);
    fill_gradient( targetArray,  onethird, c2, twothirds, c3, directionCode);
    fill_gradient( targetArray, twothirds, c3,      last, c4, directionCode);
}

/// Convenience synonym
#define fill_gradient_HSV fill_gradient


/// HSV color palette with 16 discrete values
class CHSVPalette16 {
public:
    CHSV entries[16];  ///< the color entries that make up the palette

    /// @copydoc CHSV::CHSV()
    CHSVPalette16() {};

    /// Create palette from 16 CHSV values
    CHSVPalette16( const CHSV& c00,const CHSV& c01,const CHSV& c02,const CHSV& c03,
                   const CHSV& c04,const CHSV& c05,const CHSV& c06,const CHSV& c07,
                   const CHSV& c08,const CHSV& c09,const CHSV& c10,const CHSV& c11,
                   const CHSV& c12,const CHSV& c13,const CHSV& c14,const CHSV& c15 )
    {
        entries[0]=c00; entries[1]=c01; entries[2]=c02; entries[3]=c03;
        entries[4]=c04; entries[5]=c05; entries[6]=c06; entries[7]=c07;
        entries[8]=c08; entries[9]=c09; entries[10]=c10; entries[11]=c11;
        entries[12]=c12; entries[13]=c13; entries[14]=c14; entries[15]=c15;
    };

    /// Copy constructor
    CHSVPalette16( const CHSVPalette16& rhs)
    {
        memmove8( (void *) &(entries[0]), &(rhs.entries[0]), sizeof( entries));
    }

    /// @copydoc CHSVPalette16(const CHSVPalette16& rhs)
    CHSVPalette16& operator=( const CHSVPalette16& rhs)
    {
        memmove8( (void *) &(entries[0]), &(rhs.entries[0]), sizeof( entries));
        return *this;
    }

    /// Create palette from palette stored in PROGMEM
    CHSVPalette16( const TProgmemHSVPalette16& rhs)
    {
        for( uint8_t i = 0; i < 16; ++i) {
            CRGB xyz   =  FL_PGM_READ_DWORD_NEAR( rhs + i);
            entries[i].hue = xyz.red;
            entries[i].sat = xyz.green;
            entries[i].val = xyz.blue;
        }
    }

    /// @copydoc CHSVPalette16(const TProgmemHSVPalette16&)
    CHSVPalette16& operator=( const TProgmemHSVPalette16& rhs)
    {
        for( uint8_t i = 0; i < 16; ++i) {
            CRGB xyz   =  FL_PGM_READ_DWORD_NEAR( rhs + i);
            entries[i].hue = xyz.red;
            entries[i].sat = xyz.green;
            entries[i].val = xyz.blue;
        }
        return *this;
    }

    /// Array access operator to index into the gradient entries
    /// @param x the index to retrieve
    /// @returns reference to an entry in the palette's color array
    /// @note This does not perform any interpolation like ColorFromPalette(),
    /// it accesses the underlying entries that make up the gradient. Beware
    /// of bounds issues!
    inline CHSV& operator[] (uint8_t x) __attribute__((always_inline))
    {
        return entries[x];
    }

    /// @copydoc operator[]
    inline const CHSV& operator[] (uint8_t x) const __attribute__((always_inline))
    {
        return entries[x];
    }

    /// @copydoc operator[]
    inline CHSV& operator[] (int x) __attribute__((always_inline))
    {
        return entries[(uint8_t)x];
    }

    /// @copydoc operator[]
    inline const CHSV& operator[] (int x) const __attribute__((always_inline))
    {
        return entries[(uint8_t)x];
    }

    /// Get the underlying pointer to the CHSV entries making up the palette
    operator CHSV*()
    {
        return &(entries[0]);
    }

    /// Check if two palettes have the same color entries
    bool operator==( const CHSVPalette16 &rhs) const
    {
        const uint8_t* p = (const uint8_t*)(&(this->entries[0]));
        const uint8_t* q = (const uint8_t*)(&(rhs.entries[0]));
        if( p == q) return true;
        for( uint8_t i = 0; i < (sizeof( entries)); ++i) {
            if( *p != *q) return false;
            ++p;
            ++q;
        }
        return true;
    }

    /// Check if two palettes do not have the same color entries
    bool operator!=( const CHSVPalette16 &rhs) const
    {
        return !( *this == rhs);
    }

    /// Create palette filled with one color
    /// @param c1 the color to fill the palette with
    CHSVPalette16( const CHSV& c1)
    {
        fill_solid( &(entries[0]), 16, c1);
    }

    /// Create palette with a gradient from one color to another
    /// @param c1 the starting color for the gradient
    /// @param c2 the end color for the gradient
    CHSVPalette16( const CHSV& c1, const CHSV& c2)
    {
        fill_gradient( &(entries[0]), 16, c1, c2);
    }

    /// Create palette with three-color gradient
    /// @param c1 the starting color for the gradient
    /// @param c2 the middle color for the gradient
    /// @param c3 the end color for the gradient
    CHSVPalette16( const CHSV& c1, const CHSV& c2, const CHSV& c3)
    {
        fill_gradient( &(entries[0]), 16, c1, c2, c3);
    }

    /// Create palette with four-color gradient
    /// @param c1 the starting color for the gradient
    /// @param c2 the first middle color for the gradient
    /// @param c3 the second middle color for the gradient
    /// @param c4 the end color for the gradient
    CHSVPalette16( const CHSV& c1, const CHSV& c2, const CHSV& c3, const CHSV& c4)
    {
        fill_gradient( &(entries[0]), 16, c1, c2, c3, c4);
    }

};

/// RGB color palette with 16 discrete values
class CRGBPalette16 {
public:
    CRGB entries[16];  ///< @copydoc CHSVPalette16::entries

    /// @copydoc CRGB::CRGB()
    CRGBPalette16() {};

    /// Create palette from 16 CRGB values
    CRGBPalette16( const CRGB& c00,const CRGB& c01,const CRGB& c02,const CRGB& c03,
                   const CRGB& c04,const CRGB& c05,const CRGB& c06,const CRGB& c07,
                   const CRGB& c08,const CRGB& c09,const CRGB& c10,const CRGB& c11,
                   const CRGB& c12,const CRGB& c13,const CRGB& c14,const CRGB& c15 )
    {
        entries[0]=c00; entries[1]=c01; entries[2]=c02; entries[3]=c03;
        entries[4]=c04; entries[5]=c05; entries[6]=c06; entries[7]=c07;
        entries[8]=c08; entries[9]=c09; entries[10]=c10; entries[11]=c11;
        entries[12]=c12; entries[13]=c13; entries[14]=c14; entries[15]=c15;
    };

    /// Copy constructor
    CRGBPalette16( const CRGBPalette16& rhs)
    {
        memmove8( (void *) &(entries[0]), &(rhs.entries[0]), sizeof( entries));
    }
    /// Create palette from array of CRGB colors
    CRGBPalette16( const CRGB rhs[16])
    {
        memmove8( (void *) &(entries[0]), &(rhs[0]), sizeof( entries));
    }
    /// @copydoc CRGBPalette16(const CRGBPalette16&)
    CRGBPalette16& operator=( const CRGBPalette16& rhs)
    {
        memmove8( (void *) &(entries[0]), &(rhs.entries[0]), sizeof( entries));
        return *this;
    }
    /// Create palette from array of CRGB colors
    CRGBPalette16& operator=( const CRGB rhs[16])
    {
        memmove8( (void *) &(entries[0]), &(rhs[0]), sizeof( entries));
        return *this;
    }

    /// Create palette from CHSV palette
    CRGBPalette16( const CHSVPalette16& rhs)
    {
        for( uint8_t i = 0; i < 16; ++i) {
            entries[i] = rhs.entries[i]; // implicit HSV-to-RGB conversion
        }
    }
    /// Create palette from array of CHSV colors
    CRGBPalette16( const CHSV rhs[16])
    {
        for( uint8_t i = 0; i < 16; ++i) {
            entries[i] = rhs[i]; // implicit HSV-to-RGB conversion
        }
    }
    /// @copydoc CRGBPalette16(const CHSVPalette16&)
    CRGBPalette16& operator=( const CHSVPalette16& rhs)
    {
        for( uint8_t i = 0; i < 16; ++i) {
            entries[i] = rhs.entries[i]; // implicit HSV-to-RGB conversion
        }
        return *this;
    }
    /// Create palette from array of CHSV colors
    CRGBPalette16& operator=( const CHSV rhs[16])
    {
        for( uint8_t i = 0; i < 16; ++i) {
            entries[i] = rhs[i]; // implicit HSV-to-RGB conversion
        }
        return *this;
    }

    /// Create palette from palette stored in PROGMEM
    CRGBPalette16( const TProgmemRGBPalette16& rhs)
    {
        for( uint8_t i = 0; i < 16; ++i) {
            entries[i] =  FL_PGM_READ_DWORD_NEAR( rhs + i);
        }
    }
    /// @copydoc CRGBPalette16(const TProgmemRGBPalette16&)
    CRGBPalette16& operator=( const TProgmemRGBPalette16& rhs)
    {
        for( uint8_t i = 0; i < 16; ++i) {
            entries[i] =  FL_PGM_READ_DWORD_NEAR( rhs + i);
        }
        return *this;
    }

    /// @copydoc CHSVPalette16::operator==
    bool operator==( const CRGBPalette16 &rhs) const
    {
        const uint8_t* p = (const uint8_t*)(&(this->entries[0]));
        const uint8_t* q = (const uint8_t*)(&(rhs.entries[0]));
        if( p == q) return true;
        for( uint8_t i = 0; i < (sizeof( entries)); ++i) {
            if( *p != *q) return false;
            ++p;
            ++q;
        }
        return true;
    }
    /// @copydoc CHSVPalette16::operator!=
    bool operator!=( const CRGBPalette16 &rhs) const
    {
        return !( *this == rhs);
    }
    /// @copydoc CHSVPalette16::operator[]
    inline CRGB& operator[] (uint8_t x) __attribute__((always_inline))
    {
        return entries[x];
    }
    /// @copydoc CHSVPalette16::operator[]
    inline const CRGB& operator[] (uint8_t x) const __attribute__((always_inline))
    {
        return entries[x];
    }

    /// @copydoc CHSVPalette16::operator[]
    inline CRGB& operator[] (int x) __attribute__((always_inline))
    {
        return entries[(uint8_t)x];
    }
    /// @copydoc CHSVPalette16::operator[]
    inline const CRGB& operator[] (int x) const __attribute__((always_inline))
    {
        return entries[(uint8_t)x];
    }

    /// Get the underlying pointer to the CHSV entries making up the palette
    operator CRGB*()
    {
        return &(entries[0]);
    }

    /// @copydoc CHSVPalette16::CHSVPalette16(const CHSV&)
    CRGBPalette16( const CHSV& c1)
    {
        fill_solid( &(entries[0]), 16, c1);
    }
    /// @copydoc CHSVPalette16::CHSVPalette16(const CHSV&, const CHSV&)
    CRGBPalette16( const CHSV& c1, const CHSV& c2)
    {
        fill_gradient( &(entries[0]), 16, c1, c2);
    }
    /// @copydoc CHSVPalette16::CHSVPalette16(const CHSV&, const CHSV&, const CHSV&)
    CRGBPalette16( const CHSV& c1, const CHSV& c2, const CHSV& c3)
    {
        fill_gradient( &(entries[0]), 16, c1, c2, c3);
    }
    /// @copydoc CHSVPalette16::CHSVPalette16(const CHSV&, const CHSV&, const CHSV&, const CHSV&)
    CRGBPalette16( const CHSV& c1, const CHSV& c2, const CHSV& c3, const CHSV& c4)
    {
        fill_gradient( &(entries[0]), 16, c1, c2, c3, c4);
    }

    /// @copydoc CHSVPalette16::CHSVPalette16(const CHSV&)
    CRGBPalette16( const CRGB& c1)
    {
        fill_solid( &(entries[0]), 16, c1);
    }
    /// @copydoc CHSVPalette16::CHSVPalette16(const CHSV&, const CHSV&)
    CRGBPalette16( const CRGB& c1, const CRGB& c2)
    {
        fill_gradient_RGB( &(entries[0]), 16, c1, c2);
    }
    /// @copydoc CHSVPalette16::CHSVPalette16(const CHSV&, const CHSV&, const CHSV&)
    CRGBPalette16( const CRGB& c1, const CRGB& c2, const CRGB& c3)
    {
        fill_gradient_RGB( &(entries[0]), 16, c1, c2, c3);
    }
    /// @copydoc CHSVPalette16::CHSVPalette16(const CHSV&, const CHSV&, const CHSV&, const CHSV&)
    CRGBPalette16( const CRGB& c1, const CRGB& c2, const CRGB& c3, const CRGB& c4)
    {
        fill_gradient_RGB( &(entries[0]), 16, c1, c2, c3, c4);
    }

    /// Creates a palette from a gradient palette in PROGMEM.
    ///
    /// Gradient palettes are loaded into CRGBPalettes in such a way
    /// that, if possible, every color represented in the gradient palette
    /// is also represented in the CRGBPalette.
    ///
    /// For example, consider a gradient palette that is all black except
    /// for a single, one-element-wide (1/256th!) spike of red in the middle:
    ///   @code
    ///     0,   0,0,0
    ///   124,   0,0,0
    ///   125, 255,0,0  // one 1/256th-palette-wide red stripe
    ///   126,   0,0,0
    ///   255,   0,0,0
    ///   @endcode
    /// A naive conversion of this 256-element palette to a 16-element palette
    /// might accidentally completely eliminate the red spike, rendering the
    /// palette completely black.
    ///
    /// However, the conversions provided here would attempt to include a
    /// the red stripe in the output, more-or-less as faithfully as possible.
    /// So in this case, the resulting CRGBPalette16 palette would have a red
    /// stripe in the middle which was 1/16th of a palette wide -- the
    /// narrowest possible in a CRGBPalette16.
    ///
    /// This means that the relative width of stripes in a CRGBPalette16
    /// will be, by definition, different from the widths in the gradient
    /// palette.  This code attempts to preserve "all the colors", rather than
    /// the exact stripe widths at the expense of dropping some colors.
    CRGBPalette16( TProgmemRGBGradientPalette_bytes progpal )
    {
        *this = progpal;
    }
    /// @copydoc CRGBPalette16(TProgmemRGBGradientPalette_bytes)
    CRGBPalette16& operator=( TProgmemRGBGradientPalette_bytes progpal )
    {
        TRGBGradientPaletteEntryUnion* progent = (TRGBGradientPaletteEntryUnion*)(progpal);
        TRGBGradientPaletteEntryUnion u;

        // Count entries
        uint16_t count = 0;
        do {
            u.dword = FL_PGM_READ_DWORD_NEAR(progent + count);
            ++count;
        } while ( u.index != 255);

        int8_t lastSlotUsed = -1;

        u.dword = FL_PGM_READ_DWORD_NEAR( progent);
        CRGB rgbstart( u.r, u.g, u.b);

        int indexstart = 0;
        uint8_t istart8 = 0;
        uint8_t iend8 = 0;
        while( indexstart < 255) {
            ++progent;
            u.dword = FL_PGM_READ_DWORD_NEAR( progent);
            int indexend  = u.index;
            CRGB rgbend( u.r, u.g, u.b);
            istart8 = indexstart / 16;
            iend8   = indexend   / 16;
            if( count < 16) {
                if( (istart8 <= lastSlotUsed) && (lastSlotUsed < 15)) {
                    istart8 = lastSlotUsed + 1;
                    if( iend8 < istart8) {
                        iend8 = istart8;
                    }
                }
                lastSlotUsed = iend8;
            }
            fill_gradient_RGB( &(entries[0]), istart8, rgbstart, iend8, rgbend);
            indexstart = indexend;
            rgbstart = rgbend;
        }
        return *this;
    }
    /// Creates a palette from a gradient palette in dynamic (heap) memory.
    /// @copydetails CRGBPalette16::CRGBPalette16(TProgmemRGBGradientPalette_bytes)
    CRGBPalette16& loadDynamicGradientPalette( TDynamicRGBGradientPalette_bytes gpal )
    {
        TRGBGradientPaletteEntryUnion* ent = (TRGBGradientPaletteEntryUnion*)(gpal);
        TRGBGradientPaletteEntryUnion u;

        // Count entries
        uint16_t count = 0;
        do {
            u = *(ent + count);
            ++count;
        } while ( u.index != 255);

        int8_t lastSlotUsed = -1;


        u = *ent;
        CRGB rgbstart( u.r, u.g, u.b);

        int indexstart = 0;
        uint8_t istart8 = 0;
        uint8_t iend8 = 0;
        while( indexstart < 255) {
            ++ent;
            u = *ent;
            int indexend  = u.index;
            CRGB rgbend( u.r, u.g, u.b);
            istart8 = indexstart / 16;
            iend8   = indexend   / 16;
            if( count < 16) {
                if( (istart8 <= lastSlotUsed) && (lastSlotUsed < 15)) {
                    istart8 = lastSlotUsed + 1;
                    if( iend8 < istart8) {
                        iend8 = istart8;
                    }
                }
                lastSlotUsed = iend8;
            }
            fill_gradient_RGB( &(entries[0]), istart8, rgbstart, iend8, rgbend);
            indexstart = indexend;
            rgbstart = rgbend;
        }
        return *this;
    }

};


/// Color interpolation options for palette
typedef enum {
    NOBLEND=0,            ///< No interpolation between palette entries
    LINEARBLEND=1,        ///< Linear interpolation between palette entries, with wrap-around from end to the beginning again
    LINEARBLEND_NOWRAP=2  ///< Linear interpolation between palette entries, but no wrap-around
} TBlendType;

/// Get a color from a palette.
/// These are the main functions for getting and using palette colors. Regardless
/// of the number of entries in the base palette, this function will interpolate
/// between entries to turn the discrete colors into a smooth gradient.
/// @param pal the palette to retrieve the color from
/// @param index the position in the palette to retrieve the color for (0-255)
/// @param brightness optional brightness value to scale the resulting color
/// @param blendType whether to take the palette entries directly (NOBLEND)
/// or blend linearly between palette entries (LINEARBLEND)
CRGB ColorFromPalette( const CRGBPalette16& pal,
                       uint8_t index,
                       uint8_t brightness=255,
                       TBlendType blendType=LINEARBLEND);

/// @copydoc ColorFromPalette(const CRGBPalette16&, uint8_t, uint8_t, TBlendType)
CRGB ColorFromPalette( const TProgmemRGBPalette16& pal,
                       uint8_t index,
                       uint8_t brightness=255,
                       TBlendType blendType=LINEARBLEND);

/// @copydoc ColorFromPalette(const CRGBPalette16&, uint8_t, uint8_t, TBlendType)
CHSV ColorFromPalette( const CHSVPalette16& pal,
                       uint8_t index,
                       uint8_t brightness=255,
                       TBlendType blendType=LINEARBLEND);
