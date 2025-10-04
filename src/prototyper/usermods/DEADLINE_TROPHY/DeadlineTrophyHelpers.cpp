#include "DeadlineTrophy.h"

namespace DeadlineTrophy {

    namespace FxHelpers {

        uint32_t floatHSV(float hue, float sat, float val) {
            // parameters in [0, 255] but as float
            uint32_t color = uint32_t(CRGB(CHSV(
                static_cast<uint8_t>(hue),
                static_cast<uint8_t>(sat),
                static_cast<uint8_t>(val)
            )));
            // remove the specific "white" that might be in the color as no Trophy pixel has a white
            return color & 0x00FFFFFF;
        }

        FloatRgb cosinePalette(float t, FloatRgb offset, FloatRgb scale, FloatRgb frequency, FloatRgb phase) {
            // useful tool right here: https://dev.thi.ng/gradients/
            float r = offset.r + scale.r * cos_t(M_TWOPI * (frequency.r * t + phase.r));
            float g = offset.g + scale.g * cos_t(M_TWOPI * (frequency.g * t + phase.g));
            float b = offset.b + scale.b * cos_t(M_TWOPI * (frequency.b * t + phase.b));
            return {clip(r), clip(g), clip(b)};
        }

        float invSqrt(float x) {
            // the Quake III Arena implementation, to compare against direct sqrtf() and/or sqrt32_bw() with casting
            uint32_t i;
            float x2, y;
            const float threehalfs = 1.5F;
            x2 = x * 0.5F;
            y = x;
            memcpy(&i, &y, sizeof(float)); // i = *(long*)&y;
            i = 0x5f3759df - (i >> 1);
            memcpy(&y, &i, sizeof(uint32_t)); // y = *(float*)&i;
            y = y * (threehalfs - (x2 * y * y));
            return y;
        }

        float perlin1D(float x) {
            // result gonne be within [-1, 1], while util.cpp's perlin1D_raw/() is in [-24691, 24689]
            int32_t raw = perlin1D_raw(static_cast<uint32_t>(x * 1e3), false);
            return raw / 24691.;
        }

        uint8_t mix8(uint8_t a, uint8_t b, float t) {
            float offset = static_cast<float>(a);
            float delta = static_cast<float>(b) - offset;
            float mixed = constrain(offset + t * delta, 0., 255.);
            return static_cast<uint8_t>(mixed + 0.5f);
        }

        uint32_t mixRgb(uint32_t color1, uint32_t color2, float t) {
            // yes, RGB mixing is ugly, but that's the easiest to get right here
            uint8_t r = mix8(R(color1), R(color2), t);
            uint8_t g = mix8(G(color1), G(color2), t);
            uint8_t b = mix8(B(color1), B(color2), t);
            return (r << 16) | (g << 8) | b;
        }

        CHSV mixHsv(CHSV c1, CHSV c2, float t) {
            float hue1 = static_cast<float>(c1.hue);
            float hue2 = static_cast<float>(c2.hue);
            float deltaH = hue2 - hue1;
            if (deltaH > 128) deltaH -= 256;
            else if (deltaH < -128) deltaH += 256;
            return CHSV(
                static_cast<uint8_t>(hue1 + t * deltaH),
                mix8(c1.sat, c2.sat, t),
                mix8(c1.val, c2.val, t)
            );
        }

        uint8_t pow8(uint8_t base, float exponent) {
            if (base == 0) {
                return 0;
            }
            float value = static_cast<float>(base) / 255.;
            return static_cast<uint8_t>(255. * exp(exponent * logf(value)));
        }

        uint8_t scale8f(uint8_t val, float factor) {
            return static_cast<uint8_t>(factor * static_cast<float>(val));
        }

        CRGB& scale(CRGB& color, float factor) {
            color.r = scale8f(color.r, factor);
            color.g = scale8f(color.g, factor);
            color.b = scale8f(color.b, factor);
            return color;
        }

        CRGB& grade(CRGB& color, float exponent) {
            color.r = pow8(color.r, exponent);
            color.g = pow8(color.g, exponent);
            color.b = pow8(color.b, exponent);
            return color;
        }

        long measureMicros() {
//            static int64_t t0 = 0, t1 = 0;
//            int64_t delta = -1;
//            if (t0 != 0) {
//                t1 = esp_timer_get_time();
//                delta = static_cast<long>(t1 - t0);
//            }
//            t0 = esp_timer_get_time();
//            return delta;
            return -1; // doesn't make sense in the Prototyper
        }

        void fillLogoArray(const uint8_t* pixels, size_t nPixels, uint32_t color, float opacity, bool debug)
        {
            for (int i = 0; i < nPixels; i++) {
                auto coord = Logo::coord(pixels[i]);
                uint32_t blend = color;
                if (opacity < 1.) {
                    uint32_t current = SEGMENT.getPixelColorXY(coord.x, coord.y);
                    uint32_t blend = mixRgb(current, color, opacity);

                    if (debug) {
                        DEBUG_PRINTF("[DEBUG_LOGO_ARRAY] %d %d -> %d; %.3f * %d (%d/%d/%d) on current %d (%d/%d/%d) -> %d (%d/%d/%d)\n",
                            nPixels, i, coord.index, opacity,
                            color, R(color), G(color), B(color),
                            current, R(current), G(current), B(current),
                            blend, R(blend), G(blend), B(blend)
                        );
                    }
                }

                setLogo(coord.x, coord.y, blend);
            }
        }
    }

}
