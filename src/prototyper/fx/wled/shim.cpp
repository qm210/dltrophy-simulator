//
// Created by qm210 on 27.07.2025.
//

// shimming the definitions that are just way too many lines for the headers
// reduced in whatever way seemed possible for our particular use case.

#include "shim.h"

#include <stdexcept>
#include <random>
#include <cmath>
#include <chrono>


// our most important "strip" reference, now globally defined
ShimmlerMcShimface strip;

// DeadlineTrophy.cpp needs these defined
std::vector<BusConfig> busConfigs;
bool gammaCorrectBri;
bool gammaCorrectCol;
float gammaCorrectVal;
int briS;
bool turnOnAtBoot;
int transitionDelayDefault;
int transitionDelay;
int blendingStyle;

// again, avoid these "undefined reference" (will be overwritten before actual use)
unsigned Segment::_usedSegmentData = 0U;
uint16_t Segment::maxWidth = 1;
uint16_t Segment::maxHeight = 1;
unsigned Segment::_vLength = 0;
unsigned Segment::_vWidth = 0;
unsigned Segment::_vHeight = 0;
uint32_t Segment::_currentColors[NUM_COLORS] = {0, 0, 0};
CRGBPalette16 Segment::_currentPalette    = CRGBPalette16(CRGB::Black);

// now, some stuff the embedded environment would give us for free...

unsigned long millis() {
    static std::chrono::steady_clock::time_point startedAt =
            std::chrono::steady_clock::now();
    // Note: being an unsigned long, this overflows every 49 days, but so does it on the device
    auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startedAt
            );
    return static_cast<unsigned long>(elapsed.count());
}


int32_t min(int32_t a, int32_t b) {
    return b < a ? b : a;
}

int32_t max(int32_t a, int32_t b) {
    return b > a ? b : a;
}

#ifndef __linux__
uint32_t random() {
    // the EPS32 uses some real hardware random, we don't have that, well, oops, oh no.
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen();
}
#endif

uint32_t random(uint32_t upperlimit) {
    uint32_t rnd = random();
    uint64_t scaled = uint64_t(rnd) * uint64_t(upperlimit);
    return scaled >> 32;
}

int32_t random(int32_t lowerlimit, int32_t upperlimit) {
    if (lowerlimit >= upperlimit) {
        return lowerlimit;
    }
    return random(upperlimit - lowerlimit) + lowerlimit;
}

// and now the missing definitions for the Segment.
// I removed some case distinctions that I deemed irrelevant,
// but kept most of the comments for... some reason

void Segment::setPixelColorXY(int x, int y, uint32_t col) const {
    if (!isActive()) return; // not active
    if (x >= (int)vWidth() || y >= (int)vHeight() || x < 0 || y < 0) return;  // if pixel would fall out of virtual segment just exit
    setPixelColorXYRaw(x, y, col);
}

uint32_t Segment::getPixelColorXY(int x, int y) const {
    if (!isActive()) return 0; // not active
    if (x >= (int)vWidth() || y >= (int)vHeight() || x<0 || y<0) return 0;  // if pixel would fall out of virtual segment just exit
    return getPixelColorXYRaw(x,y);
}

void Segment::fill(uint32_t c) const {
    if (!isActive()) return; // not active
    for (unsigned i = 0; i < length(); i++) setPixelColorRaw(i,c); // always fill all pixels (blending will take care of grouping, spacing and clipping)
}

void Segment::fade_out(uint8_t rate) const {
    if (!isActive()) return; // not active
    rate = (256-rate) >> 1;
    const int mappedRate = 256 / (rate + 1);
    const size_t rlength = rawLength();  // calculate only once
    for (unsigned j = 0; j < rlength; j++) {
        uint32_t color = getPixelColorRaw(j);
        if (color == colors[1]) continue; // already at target color
        for (int i = 0; i < 32; i += 8) {
            uint8_t c2 = (colors[1]>>i);  // get background channel
            uint8_t c1 = (color>>i);      // get foreground channel
            // we can't use bitshift since we are using int
            int delta = (c2 - c1) * mappedRate / 256;
            // if fade isn't complete, make sure delta is at least 1 (fixes rounding issues)
            if (delta == 0) delta += (c2 == c1) ? 0 : (c2 > c1) ? 1 : -1;
            // stuff new value back into color
            color &= ~(0xFF<<i);
            color |= ((c1 + delta) & 0xFF) << i;
        }
        setPixelColorRaw(j, color);
    }
}


/*
 * Color conversion & utility methods, from FastLED colors.cpp.
 */

uint32_t color_blend(uint32_t color1, uint32_t color2, uint8_t blend) {
    // min / max blend checking is omitted: calls with 0 or 255 are rare, checking lowers overall performance
    const uint32_t TWO_CHANNEL_MASK = 0x00FF00FF;     // mask for R and B channels or W and G if negated (poorman's SIMD; https://github.com/wled/WLED/pull/4568#discussion_r1986587221)
    uint32_t rb1 =  color1       & TWO_CHANNEL_MASK;  // extract R & B channels from color1
    uint32_t wg1 = (color1 >> 8) & TWO_CHANNEL_MASK;  // extract W & G channels from color1 (shifted for multiplication later)
    uint32_t rb2 =  color2       & TWO_CHANNEL_MASK;  // extract R & B channels from color2
    uint32_t wg2 = (color2 >> 8) & TWO_CHANNEL_MASK;  // extract W & G channels from color2 (shifted for multiplication later)
    uint32_t rb3 = ((((rb1 << 8) | rb2) + (rb2 * blend) - (rb1 * blend)) >> 8) &  TWO_CHANNEL_MASK; // blend red and blue
    uint32_t wg3 = ((((wg1 << 8) | wg2) + (wg2 * blend) - (wg1 * blend)))      & ~TWO_CHANNEL_MASK; // negated mask for white and green
    return rb3 | wg3;
}

uint32_t color_add(uint32_t c1, uint32_t c2, bool preserveCR)
{
    if (c1 == BLACK) return c2;
    if (c2 == BLACK) return c1;
    const uint32_t TWO_CHANNEL_MASK = 0x00FF00FF; // mask for R and B channels or W and G if negated
    uint32_t rb = ( c1     & TWO_CHANNEL_MASK) + ( c2     & TWO_CHANNEL_MASK); // mask and add two colors at once
    uint32_t wg = ((c1>>8) & TWO_CHANNEL_MASK) + ((c2>>8) & TWO_CHANNEL_MASK);
    uint32_t r = rb >> 16; // extract single color values
    uint32_t b = rb & 0xFFFF;
    uint32_t w = wg >> 16;
    uint32_t g = wg & 0xFFFF;

    if (preserveCR) { // preserve color ratios
        uint32_t max = std::max(r,g); // check for overflow note
        max = std::max(max,b);
        max = std::max(max,w);
        //unsigned max = r; // check for overflow note
        //max = g > max ? g : max;
        //max = b > max ? b : max;
        //max = w > max ? w : max;
        if (max > 255) {
            const uint32_t scale = (uint32_t(255)<<8) / max; // division of two 8bit (shifted) values does not work -> use bit shifts and multiplaction instead
            rb = ((rb * scale) >> 8) &  TWO_CHANNEL_MASK;
            wg =  (wg * scale)       & ~TWO_CHANNEL_MASK;
        } else wg <<= 8; //shift white and green back to correct position
        return rb | wg;
    } else {
        r = r > 255 ? 255 : r;
        g = g > 255 ? 255 : g;
        b = b > 255 ? 255 : b;
        w = w > 255 ? 255 : w;
        return RGBW32(r,g,b,w);
    }
}

uint32_t color_fade(uint32_t c1, uint8_t amount, bool video)
{
    if (amount == 255) return c1;
    if (c1 == BLACK || amount == 0) return BLACK;
    uint32_t scaledcolor; // color order is: W R G B from MSB to LSB
    uint32_t scale = amount; // 32bit for faster calculation
    uint32_t addRemains = 0;
    if (!video) scale++; // add one for correct scaling using bitshifts
    else { // video scaling: make sure colors do not dim to zero if they started non-zero
        addRemains  = R(c1) ? 0x00010000 : 0;
        addRemains |= G(c1) ? 0x00000100 : 0;
        addRemains |= B(c1) ? 0x00000001 : 0;
        addRemains |= W(c1) ? 0x01000000 : 0;
    }
    const uint32_t TWO_CHANNEL_MASK = 0x00FF00FF;
    uint32_t rb = (((c1 & TWO_CHANNEL_MASK) * scale) >> 8) &  TWO_CHANNEL_MASK; // scale red and blue
    uint32_t wg = (((c1 >> 8) & TWO_CHANNEL_MASK) * scale) & ~TWO_CHANNEL_MASK; // scale white and green
    scaledcolor = (rb | wg) + addRemains;
    return scaledcolor;
}

void Segment::fadeToSecondaryBy(uint8_t fadeBy) const {
    if (!isActive() || fadeBy == 0) return;   // optimization - no scaling to apply
    const size_t rlength = rawLength();  // calculate only once
    for (unsigned i = 0; i < rlength; i++) setPixelColorRaw(i, color_blend(getPixelColorRaw(i), colors[1], fadeBy));
}

void Segment::fadeToBlackBy(uint8_t fadeBy) const {
    if (!isActive() || fadeBy == 0) return;   // optimization - no scaling to apply
    const size_t rlength = rawLength();  // calculate only once
    for (unsigned i = 0; i < rlength; i++) setPixelColorRaw(i, color_fade(getPixelColorRaw(i), 255-fadeBy));
}

void Segment::blur(uint8_t blur_amount, bool smear) const {
    if (!isActive() || blur_amount == 0) return; // optimization: 0 means "don't blur"
    blur2D(blur_amount, blur_amount, smear); // symmetrical 2D blur
}

void Segment::blur2D(uint8_t blur_x, uint8_t blur_y, bool smear) const {
    if (!isActive()) return; // not active
    const unsigned cols = vWidth();
    const unsigned rows = vHeight();
    const auto XY = [&](unsigned x, unsigned y){ return x + y*cols; };
    uint32_t lastnew; // not necessary to initialize lastnew and last, as both will be initialized by the first loop iteration
    uint32_t last;
    if (blur_x) {
        const uint8_t keepx = smear ? 255 : 255 - blur_x;
        const uint8_t seepx = blur_x >> 1;
        for (unsigned row = 0; row < rows; row++) { // blur rows (x direction)
            uint32_t carryover = BLACK;
            uint32_t curnew = BLACK;
            for (unsigned x = 0; x < cols; x++) {
                uint32_t cur = getPixelColorRaw(XY(x, row));
                uint32_t part = color_fade(cur, seepx);
                curnew = color_fade(cur, keepx);
                if (x > 0) {
                    if (carryover) curnew = color_add(curnew, carryover);
                    uint32_t prev = color_add(lastnew, part);
                    // optimization: only set pixel if color has changed
                    if (last != prev) setPixelColorRaw(XY(x - 1, row), prev);
                } else setPixelColorRaw(XY(x, row), curnew); // first pixel
                lastnew = curnew;
                last = cur; // save original value for comparison on next iteration
                carryover = part;
            }
            setPixelColorRaw(XY(cols-1, row), curnew); // set last pixel
        }
    }
    if (blur_y) {
        const uint8_t keepy = smear ? 255 : 255 - blur_y;
        const uint8_t seepy = blur_y >> 1;
        for (unsigned col = 0; col < cols; col++) {
            uint32_t carryover = BLACK;
            uint32_t curnew = BLACK;
            for (unsigned y = 0; y < rows; y++) {
                uint32_t cur = getPixelColorRaw(XY(col, y));
                uint32_t part = color_fade(cur, seepy);
                curnew = color_fade(cur, keepy);
                if (y > 0) {
                    if (carryover) curnew = color_add(curnew, carryover);
                    uint32_t prev = color_add(lastnew, part);
                    // optimization: only set pixel if color has changed
                    if (last != prev) setPixelColorRaw(XY(col, y - 1), prev);
                } else setPixelColorRaw(XY(col, y), curnew); // first pixel
                lastnew = curnew;
                last = cur; //save original value for comparison on next iteration
                carryover = part;
            }
            setPixelColorRaw(XY(col, rows - 1), curnew);
        }
    }
}

// from FX.cpp, as a fallback if ever called from our actual mode_...()
uint16_t mode_static(void) {
    SEGMENT.fill(SEGCOLOR(0));
    return strip.isOffRefreshRequired() ? FRAMETIME : 350;
}
static const char _data_FX_MODE_STATIC[] PROGMEM = "Solid";
