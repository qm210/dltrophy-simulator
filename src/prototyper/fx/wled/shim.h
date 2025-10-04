#pragma once

/*
 * Mostly somewhat brainlessly copied from our current WLED fork.
 */

#include <cstdint>
#include <array>
#include <vector>

#include "FX.h"
#include "fx/wled/const.h"

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352

#define MAXFLOAT	3.40282347e+38F

#define M_E		2.7182818284590452354
#define M_LOG2E		1.4426950408889634074
#define M_LOG10E	0.43429448190325182765
// #define M_LN2		_M_LN2
#define M_LN10		2.30258509299404568402
#define M_PI		3.14159265358979323846
#define M_PI_2		1.57079632679489661923
#define M_PI_4		0.78539816339744830962
#define M_1_PI		0.31830988618379067154
#define M_2_PI		0.63661977236758134308
#define M_2_SQRTPI	1.12837916709551257390
#define M_SQRT2		1.41421356237309504880
#define M_SQRT1_2	0.70710678118654752440

#define M_TWOPI         (M_PI * 2.0)
#define M_3PI_4		2.3561944901923448370E0
#define M_SQRTPI        1.77245385090551602792981
#define M_LN2LO         1.9082149292705877000E-10
#define M_LN2HI         6.9314718036912381649E-1
#define M_SQRT3	1.73205080756887719000
#define M_IVLN10        0.43429448190325182765 /* 1 / log(10) */
#define M_LOG2_E        _M_LN2
#define M_INVLN2        1.4426950408889633870E0  /* 1 / log(2) */

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))

int32_t min(int32_t a, int32_t b);
int32_t max(int32_t a, int32_t b);

#ifndef __linux__
uint32_t random();
#endif
uint32_t random(uint32_t upperlimit);
int32_t random(int32_t lowerlimit, int32_t upperlimit);

typedef uint8_t byte;

unsigned long millis();

#define F(x) x
#define PROGMEM

#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#define DEBUG_PRINTLN(...) do { printf(__VA_ARGS__); printf("\n"); } while(0)

#define DEFAULT_BRIGHTNESS (uint8_t)127
#define DEFAULT_MODE       (uint8_t)0
#define DEFAULT_SPEED      (uint8_t)128
#define DEFAULT_INTENSITY  (uint8_t)128
#define DEFAULT_COLOR      (uint32_t)0xFFAA00
#define DEFAULT_C1         (uint8_t)128
#define DEFAULT_C2         (uint8_t)128
#define DEFAULT_C3         (uint8_t)16

struct Segment {
    /*
     * qm210: Tried to copy the minimum useful-looking set, but that as-is
     *        (thus all the "mutable", which probably follow no deeper sense here)
     */
    uint16_t start;
    uint16_t stop;
    uint16_t startY;
    uint16_t stopY;

    uint32_t *pixels = nullptr;
    mutable uint8_t _capabilities;
    uint32_t colors[NUM_COLORS];
    char *name;

    Segment(uint16_t sStart = 0, uint16_t sStop = 0, uint16_t sStartY = 0, uint16_t sStopY = 0)
      : start(sStart),
        stop(sStop > sStart ? sStop : sStart + 1),
        startY(sStartY),
        stopY(sStopY > sStartY ? sStopY : sStartY + 1),
        colors{DEFAULT_COLOR,BLACK,BLACK}
    {
        pixels = new uint32_t[length()]{};
        name = (char*)"--unnamed--";
    }

    Segment(const Segment&) = default;
    Segment& operator=(const Segment&) = default;
    Segment(Segment&&) = default;
    Segment& operator=(Segment&&) = default;

    ~Segment() {
        // this SegFaults...?
        // delete[] pixels;
    }

    static uint16_t maxWidth, maxHeight;

    inline uint16_t width() const { return stop > start ? (stop - start) : 0; }
    inline uint16_t height() const { return stopY - startY; }
    inline uint16_t length() const { return width() * height(); }
    inline bool isActive() const { return stop > start && pixels; }

    mutable bool on;
    mutable bool freeze;
    uint8_t map1D2D;
    uint8_t soundSim;
    uint8_t opacity;
    uint8_t palette;
    uint8_t speed;
    uint8_t intensity;
    uint8_t custom1;
    uint8_t custom2;
    uint8_t custom3; // is actually only 5-bit because packed with options1..3
    bool option1;
    bool option2;
    bool option3;
    uint8_t blendMode;

    mutable unsigned long next_time;
    mutable uint32_t step;
    mutable uint32_t call;
    mutable uint16_t aux0;
    mutable uint16_t aux1;
    byte     *data;

    // WLED uses these static members as pre-calc cache
    // this works because in its global context, there's always ever one the current one.
    static unsigned      _usedSegmentData;
    static unsigned      _vLength;
    static unsigned      _vWidth, _vHeight;
    static uint32_t      _currentColors[NUM_COLORS];
    static CRGBPalette16 _currentPalette;

    inline static unsigned vLength()                       { return Segment::_vLength; }
    inline static unsigned vWidth()                        { return Segment::_vWidth; }
    inline static unsigned vHeight()                       { return Segment::_vHeight; }
    inline static uint32_t getCurrentColor(unsigned i)     { return Segment::_currentColors[i<NUM_COLORS?i:0]; }
    inline static const CRGBPalette16 &getCurrentPalette() { return Segment::_currentPalette; }

    // simplified for our specific use case (nothing transposed, grouped or mirrored )
    inline unsigned virtualWidth() const { return width(); };
    inline unsigned virtualHeight() const { return height(); }
    inline unsigned virtualLength() const { return length(); }
    inline unsigned rawLength() const { return virtualWidth() * virtualHeight(); }

    inline void     setPixelColorRaw(unsigned i, uint32_t c) const  { pixels[i] = c; }
    inline uint32_t getPixelColorRaw(unsigned i) const              { return pixels[i]; };
    inline void     setPixelColorXYRaw(unsigned x, unsigned y, uint32_t c) const  { auto XY = [](unsigned X, unsigned Y){ return X + Y*Segment::vWidth(); }; pixels[XY(x,y)] = c; }
    inline uint32_t getPixelColorXYRaw(unsigned x, unsigned y) const              { auto XY = [](unsigned X, unsigned Y){ return X + Y*Segment::vWidth(); }; return pixels[XY(x,y)]; };
    [[gnu::hot]] void setPixelColorXY(int x, int y, uint32_t c) const;
    [[gnu::hot]] uint32_t getPixelColorXY(int x, int y) const;

    void blur(uint8_t, bool smear = false) const;
    void clear() const { fill(BLACK); } // clear segment
    void fill(uint32_t c) const;
    void fade_out(uint8_t r) const;
    void fadeToSecondaryBy(uint8_t fadeBy) const;
    void fadeToBlackBy(uint8_t fadeBy) const;
    void blur2D(uint8_t blur_x, uint8_t blur_y, bool smear = false) const;
    inline void fill_solid(CRGB c) const { fill(RGBW32(c.r,c.g,c.b,0)); }
};

[[gnu::hot, gnu::pure]] uint32_t color_blend(uint32_t c1, uint32_t c2 , uint8_t blend);
inline uint32_t color_blend16(uint32_t c1, uint32_t c2, uint16_t b) { return color_blend(c1, c2, b >> 8); };
[[gnu::hot, gnu::pure]] uint32_t color_add(uint32_t, uint32_t, bool preserveCR = false);
[[gnu::hot, gnu::pure]] uint32_t color_fade(uint32_t c1, uint8_t amount, bool video=false);

struct BusConfig {
    uint8_t type;
    uint16_t count;
    uint16_t start;
    uint8_t colorOrder;
    bool reversed;
    uint8_t skipAmount;
    bool refreshReq;
    uint8_t autoWhite;
    uint8_t pins[5] = {255, 255, 255, 255, 255};
    uint16_t frequency;
    uint8_t milliAmpsPerLed;
    uint16_t milliAmpsMax;

    BusConfig(uint8_t busType, uint8_t* ppins, uint16_t pstart, uint16_t len = 1, uint8_t pcolorOrder = COL_ORDER_GRB, bool rev = false, uint8_t skip = 0, byte aw=RGBW_MODE_MANUAL_ONLY, uint16_t clock_kHz=0U, uint8_t maPerLed=LED_MILLIAMPS_DEFAULT, uint16_t maMax=ABL_MILLIAMPS_DEFAULT)
            : count(std::max(len,(uint16_t)1))
            , start(pstart)
            , colorOrder(pcolorOrder)
            , reversed(rev)
            , skipAmount(skip)
            , autoWhite(aw)
            , frequency(clock_kHz)
            , milliAmpsPerLed(maPerLed)
            , milliAmpsMax(maMax)
    {
        type = busType & 0x7F;
        pins[0] = ppins[0];
        pins[1] = ppins[1];
    }
};

namespace BusManager {
    inline uint16_t ablMilliampsMax() { return ABL_MILLIAMPS_DEFAULT; }
};

namespace NeoGammaWLEDMethod {
    inline void calcGammaTable(float) {}
}

// Shims WS2812FX, the main WLED class. Is only a hollow, transparent shell for us.
struct ShimmlerMcShimface {
    uint32_t *_pixels;
    std::vector<Segment> _segments;
    uint16_t _length;
    bool isMatrix;
    Segment* _currentSegment;
    uint8_t _segment_index;
    uint8_t _mainSegment;

    inline uint8_t getCurrSegmentId() const { return _segment_index; }
    inline uint8_t getMainSegmentId() const { return _mainSegment; }      // returns main segment index
    inline Segment& getSegment(unsigned id) { return _segments[id >= _segments.size() ? getMainSegmentId() : id]; }

    inline static unsigned vLength()                       { return Segment::_vLength; }
    inline static unsigned vWidth()                        { return Segment::_vWidth; }
    inline static unsigned vHeight()                       { return Segment::_vHeight; }
    inline static uint32_t getCurrentColor(unsigned i)     { return Segment::_currentColors[i<NUM_COLORS?i:0]; }
    inline static const CRGBPalette16 &getCurrentPalette() { return Segment::_currentPalette; }

    unsigned long now, timebase;
    uint16_t _frametime;
    unsigned long _lastServiceShow;

    inline uint16_t getFrameTime() const    { return _frametime; }

    // unclear yet what to make of this, just deny everything -- maybe call a lawyer
    inline bool isOffRefreshRequired() const { return false; }
};

extern ShimmlerMcShimface strip;

extern std::vector<BusConfig> busConfigs;
extern bool gammaCorrectBri;
extern bool gammaCorrectCol;
extern float gammaCorrectVal;
extern int briS;
extern bool turnOnAtBoot;
extern int transitionDelayDefault;
extern int transitionDelay;
extern int blendingStyle;
