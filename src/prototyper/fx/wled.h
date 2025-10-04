#pragma once

/*
 * This file is a mock / shim so that DeadlineTrophy.h can work as expected
 * without any WLED or any embedded environment even being there.
 */

#include "fx/wled/shim.h"
#include <cstdio>
#include <math.h>

uint16_t beatsin88_t(accum88 beats_per_minute_88, uint16_t lowest = 0, uint16_t highest = 65535, uint32_t timebase = 0, uint16_t phase_offset = 0);
uint16_t beatsin16_t(accum88 beats_per_minute, uint16_t lowest = 0, uint16_t highest = 65535, uint32_t timebase = 0, uint16_t phase_offset = 0);
uint8_t beatsin8_t(accum88 beats_per_minute, uint8_t lowest = 0, uint8_t highest = 255, uint32_t timebase = 0, uint8_t phase_offset = 0);
float mapf(float x, float in_min, float in_max, float out_min, float out_max);

// fcn_declare.h
uint32_t hashInt(uint32_t s);
int32_t perlin1D_raw(uint32_t x, bool is16bit = false);
int32_t perlin2D_raw(uint32_t x, uint32_t y, bool is16bit = false);
int32_t perlin3D_raw(uint32_t x, uint32_t y, uint32_t z, bool is16bit = false);
uint16_t perlin16(uint32_t x);
uint16_t perlin16(uint32_t x, uint32_t y);
uint16_t perlin16(uint32_t x, uint32_t y, uint32_t z);
uint8_t perlin8(uint16_t x);
uint8_t perlin8(uint16_t x, uint16_t y);
uint8_t perlin8(uint16_t x, uint16_t y, uint16_t z);

#define HW_RND_REGISTER uint8_t(random())
inline uint32_t hw_random() { return HW_RND_REGISTER; };
uint32_t hw_random(uint32_t upperlimit); // not inlined for code size
int32_t hw_random(int32_t lowerlimit, int32_t upperlimit);
inline uint16_t hw_random16() { return HW_RND_REGISTER; };
inline uint16_t hw_random16(uint32_t upperlimit) { return (hw_random16() * upperlimit) >> 16; }; // input range 0-65535 (uint16_t)
inline int16_t hw_random16(int32_t lowerlimit, int32_t upperlimit) { int32_t range = upperlimit - lowerlimit; return lowerlimit + hw_random16(range); }; // signed limits, use int16_t ranges
inline uint8_t hw_random8() { return HW_RND_REGISTER; };
inline uint8_t hw_random8(uint32_t upperlimit) { return (hw_random8() * upperlimit) >> 8; }; // input range 0-255
inline uint8_t hw_random8(uint32_t lowerlimit, uint32_t upperlimit) { uint32_t range = upperlimit - lowerlimit; return lowerlimit + hw_random8(range); }; // input range 0-255
