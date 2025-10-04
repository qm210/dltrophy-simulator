#pragma once

#include "FX.h"
#include "../usermods/DEADLINE_TROPHY/DeadlineTrophy.h"

extern uint16_t mode_static(void);

extern float beat;
extern float bpm;
extern float globalTime;

//////////////////////////////////////////////////////////
//  Deadline Trophy FX
//////////////////////////////////////////////////////////
// There's a lot of useful stuff in
// FX.cpp -> see the DEV INFO, also, other FX for "inspiration" :)
// wled_math.cpp
// util.cpp
// -- you can also reinvent the wheel.
// I have never tested how efficient we really _HAVE_ to be,
// thus all the helper functions. ..qm
//////////////////////////////////////////////////////////
//
// Notes:
// - the if (...SEGMENT) { ... } is due to "this FX will be evaluated for every segment on its own"
//   remember, that you also have to set it for each segment to be actually active.
// - for "state" variables, there are two uint8_t SEGMENT.aux0 and SEGMENT.aux1,
//   but you can also declare any variable "static" to be more flexible (QM figures that makes sense here)
// - the EVERY_NTH_CALL(n) { ... } helper is great for debugging.

inline float bpmAt(float t) {
    // just taken stumpfly out of bitwig
    const float t1 = 33.951;
    const float t12 = 41.971;
    const float t2 = 68.524;
    const float t23 = 72.454;
    const float t3 = 98.945;
    const float t34 = 102.733;
    const float bpm1 = 113.10;
    const float bpm2 = 126.54;
    const float bpm3 = 117.78;
    const float bpm4 = 136.10;
    if (t < t1) {
        return bpm1;
    } else if (t < t12) {
        return bpm1 + (bpm2 - bpm1) * (t - t1) / (t12 - t1);
    } else if (t < t2) {
        return bpm2;
    } else if (t < t23) {
        return bpm2 + (bpm3 - bpm2) * (t - t2) / (t23 - t2);
    } else if (t < t3) {
        return bpm3;
    } else if (t < t34) {
        return bpm3 + (bpm4 - bpm3) * (t - t3) / (t34 - t3);
    } else {
        return bpm4;
    }
}

struct Sparkle {
    DeadlineTrophy::Vec2 pos;
    DeadlineTrophy::Vec2 vel;
    float size;
    uint8_t hue;
};

static CHSV hsvNone = CHSV(0, 0, 0);

float elapsed = 0.;
float deltaBeat = 0.;

uint16_t mode_DeadlineTrophy(void) {
    using namespace DeadlineTrophy;
    using namespace DeadlineTrophy::FxHelpers;
    CRGB segcolor0 = CRGB(SEGCOLOR(0));
    CHSV color = rgb2hsv_approximate(segcolor0);
    CHSV color_(color);
    float hue = static_cast<float>(color.hue);

    if (IS_BASE_SEGMENT) {
        // only on first segment, remember - this gets called once for each segment
        elapsed = secondNow() - globalTime;
        bpm = bpmAt(globalTime);
        deltaBeat = bpm / 240.f * elapsed;
        beat += deltaBeat;
        // <-- factor 4 because of bars-per-beat
        globalTime += elapsed;
    }
    float time = globalTime;
    float fractBeat = fmodf(beat, 1.);
    float fractBar = fmodf(beat, 0.25f);
    float halfBeat = floor(beat * 2.f) * 0.5;

    if (SEGENV.call == 0) {
        // DEBUG_PRINTF("[DEADLINE_TROPHY] FX was called, now initialized for segment %d (%s) :)\n", strip.getCurrSegmentId(), SEGMENT.name);
        SEGMENT.fill(BLACK);
        // we use SEGENV.aux0 as a internal store for the random-flashes, but for no particular reason - stop me if you care
        SEGMENT.aux0 = 0;
    }

    // Idea is, that every 4 beats random numbers are drawn, then maybe some flashing effect shown.
    // (this demonstrates random numbers and how to use the palette).
    static float randomTakenAtBeat = 0.;
    CRGB lastRandomColor;
    if (beat > randomTakenAtBeat + 4.) {
        SEGMENT.aux0 = hw_random8();
        randomTakenAtBeat = floor_t(beat);
        lastRandomColor = ColorFromPalette(SEGPALETTE, hw_random8(), 255, LINEARBLEND_NOWRAP);
    }

    static Vec2 triangleLeft = Logo::coord(138).uv;
    static Vec2 triangleRight = Logo::coord(103).uv;

    if (IS_LOGO_SEGMENT) {
        using namespace Logo;

        // QM: that width is useful for a point with exp(-r*r/w/w), values go from [1/8..1]
        // float width = exp2f((static_cast<float>(SEGMENT.intensity) - 192.) / 64.);
        // mapping 0..255 to [1/4 .. 4] with centered 128 = 1:
        float k = exp2f((static_cast<float>(SEGMENT.intensity) - 128.) / 32.);
        static float k_factor = 1.;
        // custom3 is only 5bit (0..31)
        float w = exp2f((static_cast<float>(SEGMENT.custom3) - 16.) / 8.);

        float d;
        CHSV lineColor;
        float lineY = triangleLeft.y + 1. * unit;
        if (beat < 1.5) {
            lineColor = CHSV(230, 200, uint8_t(190. * beat / 1.5));
        }
        else if (beat < 17) {
            lineColor = CHSV(uint8_t(230 - (halfBeat - 3.) / (8. - 3.)), 210, uint8_t(200. * expf(-0.25f*(beat - 1.5))));
            lineY += (halfBeat - 2.f) * unit - 2. * unit * fractBeat;
        }
        else if (beat > 19. && beat < 21.) {
            lineY = 0.3f + 2.5 * unit * fractBar - 0.17 * floor(beat);
            lineColor = CHSV(0, 0, 188);
        }

        static std::vector<Sparkle> sparks;
        static float sparkedAtBeat = 4.;

        SEGMENT.fadeToBlackBy(0);
        for (auto &spark : sparks) {
            spark.pos = spark.pos + spark.vel * deltaBeat;
        }
        if (beat > 4. && beat < 18. && beat - sparkedAtBeat > 0.5) {
            float rand_x = 0.04f * float(random(-20, 20));
            float rand_y = 0.42f - 0.02f * float(random(20));
            Sparkle sparked{
                    { rand_x, rand_y },
                    { 1.f * rand_x, -2.5f * unit },
                    0.13f,
                    hw_random8(126, 142)
            };
            sparks.push_back(sparked);
            sparkedAtBeat = beat;
        }
        static CHSV beatColor = hsvNone;
        if (beat >= 18. && beat < 21.) {
            float mixB18 = clip((beat - 18.f) / 3.f);
            beatColor = CHSV(
                    98,
                    mix8(255, 0, mixB18),
                    mix8(255, 140, mixB18)
            );
        }
        if (beat >= 21. && beat < 36.) {
            if (beat >= sparkedAtBeat + 1.) {
                beatColor = CHSV(
                        hw_random8(160, 230),
                        hw_random8(128, 255),
                        hw_random8(200, 255)
                );
                sparkedAtBeat = beat;
            }
        }

        for (const auto& coord : logoCoordinates()) {
            if (beat < 18.) {
                d = coord.sdLine({-10., lineY}, {10., lineY});
                color = mixHsv(hsvNone, lineColor, exp(-4.f * d));

                for (auto& spark : sparks) {
                    d = coord.gaussAt(spark.pos, spark.size);
                    color = mixHsv(color, CHSV(spark.hue, 255, 255), d);
                }
            }
            else if (beat < 21.) {
                color = beatColor;
                color.val /= 2;
                color.sat = uint8_t(float(beatColor.sat) * exp(-4. * coord.uv.length()));
                d = coord.sdLine({-10., lineY}, {10., lineY});
                color = mixHsv(hsvNone, lineColor, exp(-3.f * d));
            }
            else if (beat < 36.) {
                d = coord.uv.length();
                d *= 0.3f * exp(-9.f * fractBar);
                float theta = coord.uv.polarAngleFrom({0, 0}, 0.125f * beat * M_TWOPI);
                d *= 0.8f + 0.2f * sinf(M_TWOPI * 0.666f * theta);
                color = mixHsv(hsvNone, beatColor, d);
                if (beat > 29. && fractBeat > 0.5f) {
                    auto someChance = random16(3269 + 71 * uint16_t(beat));
                    if (someChance % 100 < 35) {
                        color.val = 0;
                    }
                }
            }
            else if (beat < 38.) {
                color = hsvNone;
            }
            else if (beat < 45.) {
                float bar = (beat - 38) * 0.25f;
                d = coord.uv.length();
                d = sin_t(M_TWOPI * (d * k * k_factor + w * bar));
                d *= d;
                d = 0.02f / d;
                // nach Polarwinkel unterdrücken...
                float theta = coord.uv.polarAngleFrom({0, 0}, 0.25f * bar * M_TWOPI);
                float gaussCurveExponent = (fractBeat - 6.) / 2;
                float swirlIntensity = exp(-sq(gaussCurveExponent));
                d *= exp(-theta * swirlIntensity);

                // at some other time, make k large and suppress points
                gaussCurveExponent = (fmodf(bar, 16.f) - 9.f) / 2.4f;
                k_factor = 1.f + exp(-sq(gaussCurveExponent));
                d = powf(d, sq(k_factor));
                float maxVal = clip(floor(bar - 4.f * 45.f) * 0.2f * d) * 111.;
                color.hue = static_cast<uint8_t>(hue - 3. * theta);
                color.sat = uint8_t(210 * exp(-3. * fractBar));
                color.val = static_cast<uint8_t>(maxVal);
            } else if (beat < 72.) {
                float bar = (beat - 52) * 0.25f;
                float r_part = 0.;
                if (beat >= 60.) {
                    r_part = 0.8f * coord.uv.length();
                }
                float theta = coord.uv.polarAngleFrom({0, 0}, bar * 0.666f * M_TWOPI + r_part);
                d = exp(-3.4f * fmodf(theta, M_TWOPI / 3.));
                color = CHSV(uint8_t(190. + 50. * d), 230, 210);
                color.val = pow8(color.val, 1.7);
                if (color.hue < 180) {
                    color.hue /= 2;
                }
            }
                /*
            // draw one triangle to show usage of left/right tilt vectors
            CHSV triangleColor = CHSV(230, 200, 170);
            Vec2 pointForLeft = triangleLeft + (-8.f + 24.f * perlin1D(10. * beat)) * Logo::xUnit;
            d = coord.sdLine(pointForLeft - 10. * Logo::tiltRight, pointForLeft + 10. * Logo::tiltRight);
            color = mixHsv(color, triangleColor, exp(-6. * d));
            Vec2 pointForRight = triangleRight + (3.f - 7.f * perlin1D(10. * beat + 4.)) * Logo::xUnit;
            d = coord.sdLine(pointForRight - 10. * Logo::tiltLeft, pointForRight + 10. * Logo::tiltLeft);
            color = mixHsv(color, triangleColor, exp(-7. * d));
            float bottomLineHeight = triangleLeft.y + Logo::unit * (-1.f + 5.f * perlin1D(6. * beat + 100.));
            d = coord.sdLine({-10., bottomLineHeight}, {10., bottomLineHeight});
            color = mixHsv(color, triangleColor, exp(-8. * d));

            }
*/
            setLogo(coord.x, coord.y, color);
        }

        // static uint32_t contourColor = uint32_t(CRGB(190, 40, 120));
        auto contourColor = cosinePalette(
            0.66f * (beat - 36.),
            {0.762, 0.937, 0.495},
            {0.292, 0.290, 0.728},
            {1.184, 0.780, 0.646},
            {4.322, 6.210, 3.380}
        );
        uint32_t cColor = uint32_t(contourColor);
        if (beat >= 36. && beat < 45.) {
            int contourIndex = static_cast<int>((beat - 36.) * 8.) % MiddleTriangle.size();
            Coord wanderPixel = coord(MiddleTriangle[MiddleTriangle.size() - 1 - contourIndex]);
            setLogo(wanderPixel.x, wanderPixel.y, cColor);
            if (contourIndex < MiddleTriangle.size() - 1) { // small tail
                wanderPixel = coord(MiddleTriangle[MiddleTriangle.size() - 2 - contourIndex]);
                cColor = mixRgb(BLACK, cColor, 0.06);
                setLogo(wanderPixel.x, wanderPixel.y, cColor);
            }
        } else if (beat > 45. && beat < 51.) {
            int contourIndex = static_cast<uint8_t>((beat - 45.) * 4.) % OuterTriangle.size();
            Coord wanderPixel = coord(OuterTriangle[contourIndex]);
            setLogo(wanderPixel.x, wanderPixel.y, cColor);
            if (contourIndex > 0) { // small tail
                wanderPixel = coord(OuterTriangle[contourIndex - 1]);
                auto c2Color = mixRgb(BLACK, cColor, 0.06);
                setLogo(wanderPixel.x, wanderPixel.y, c2Color);
            }
            contourIndex = static_cast<uint8_t>((beat - 45.) * 8.) % InnerTriangle.size();
            wanderPixel = coord(InnerTriangle[InnerTriangle.size() - 1 - contourIndex]);
            setLogo(wanderPixel.x, wanderPixel.y, cColor);
            if (contourIndex < InnerTriangle.size() - 1) {
                wanderPixel = coord(InnerTriangle[InnerTriangle.size() - 2 - contourIndex]);
                auto c2Color = mixRgb(BLACK, cColor, 0.09);
                setLogo(wanderPixel.x, wanderPixel.y, c2Color);
            }
        } else if (beat >= 60.f & beat < 71.75f) {
            fillLogoArray(
                    Logo::Contour.data(),
                    Logo::Contour.size(),
                    uint32_t(CRGB(190, 3, 255)),
                    exp(-26.f*fractBar)
            );
        }
    }

    if (IS_BASE_SEGMENT) {
        static float phi = 0.;
        static float omega = 1.7;

        static std::vector<Sparkle> sparks;
        static float sparkedAtBeat = 4.;
        for (auto &spark : sparks) {
            spark.pos = spark.pos + spark.vel * deltaBeat;
        }
        if (beat > 4. && beat < 18. && beat - sparkedAtBeat > 0.5) {
            float rand_x = 0.5f * (hw_random8(0, 10) < 5 ? -1. : 1.f);
            float rand_y = 0.5f * (hw_random8(0, 10) < 5 ? -1. : 1.f);
            float rand_vel = 0.06f * static_cast<float>(hw_random8(1, 16));
            Vec2 vel{ rand_vel, rand_vel };
            if (hw_random8(0, 10) < 5) {
                vel.x = 0.;
            } else {
                vel.y = 0.;
            }
            if (rand_x > 0.) {
                vel.x *= -1.;
            }
            if (rand_y > 0.) {
                vel.y *= -1.;
            }
            Sparkle sparked{
                    { rand_x, rand_y },
                    vel,
                    0.17f,
                    hw_random8(142, 178)
            };
            sparks.push_back(sparked);
            sparkedAtBeat = beat;
        }
        float d;

        SEGMENT.fadeToBlackBy(10);

        for (const auto& coord : baseCoordinates()) {
            if (beat < 4.) {
                color = hsvNone;
            }
            else if (beat < 18.) {
                color = hsvNone;
                for (auto& spark : sparks) {
                    d = coord.gaussAt(spark.pos, spark.size);
                    color = mixHsv(color, CHSV(spark.hue, 120, 255), d);
                }
            }
            else if (beat < 36.) {
                FloatRgb col0 = cosinePalette(
                        0.25f * beat,
                        {0.29, 0.22, 0.5},
                        {0.43, 0.74f, 0.74},
                        {1.3, 0.7, 1.2},
                        {0.85, 0.14, 0.83}
                );
                // read the "uvS" coordinate as s == 0 in the middle of every side, becoming (-)0.5 towards the corners
                float uvs = min(abs(coord.uv.x), abs(coord.uv.y));
                float d = (1. - cos_t(M_PI * 5.f * uvs)) * (0.6 + 0.4 * sin_t(M_TWOPI * 0.125 * beat));
                col0.scale(d * d);
                col0.grade(0.8);
                float dimTheMiddle = 1.f - 0.8 * cos(M_PI * uvs);
                col0.scale(dimTheMiddle);
                phi = 0.25f * beat * omega;
                Vec2 end{1, 1};
                end.rotate(phi);
                d = coord.sdLine(-end, end);
                d = exp(-2.1f * d);

                color = col0.toCHSV();
                color.sat = mix8(255, 20, d);
                color.val = max(color.val, static_cast<uint8_t>(255. * d));
            }
            else if (beat < 38.) {
                color = hsvNone;
            }
            else {
                float baseVal = ((coord.index + int(halfBeat)) % 2 == 0) ? 150. : 240.;
                float uvs = min(abs(coord.uv.x), abs(coord.uv.y));
                float whitenTheCorners = 0.4f + 0.53f * cos(M_PI * uvs);
                color = CHSV(
                        (coord.index % 2 == 0) ? 234 : 197,
                        uint8_t(222. * whitenTheCorners),
                        uint8_t(baseVal * exp(-5. * fractBeat))
                );
            }
            setBase(coord.x, coord.y, color);
        }
    }

    // for the first argument of beatsin8_t (accum88 type), "BPM << 8" just oscillates once per beat
    uint8_t fourBeatSineWave = beatsin8_t(SEGMENT.speed << 6, 0, 255);
    uint8_t beatSineWave = beatsin8_t(SEGMENT.speed << 8, 0, 255);

    if (IS_BACK_LED) {
        if (beat > 4. && beat < 18.) {
            setSingle(uint8_t(200 * exp(-fmodf(fractBeat, 0.5))));
        }
        else if (beat >= 18. && beat < 36.){
            setSingle(fourBeatSineWave);
        }
        else if (beat > 45.) {
            setSingle(beatSineWave);
        }
        else {
            setSingle(BLACK);
        }
    }
    else if (IS_FLOOR_LED) {
        if (beat >= 18. && beat < 36.) {
            setSingle(uint8_t(255. * exp(-3. * fractBeat)));
        }
        else if ((beat > 45. && beat <= 47.) || (beat >= 48. && beat < 50.)) {
            setSingle(beatSineWave / 2);
        }
        else if (beat >= 50. && beat < 52.) {
            setSingle(beatSineWave);
        }
        else if (beat >= 52. && beat < 73.5f) {
            setSingle(100 + beatSineWave / 2);
        }
        else {
            setSingle(BLACK);
        }

    }

    return FRAMETIME;
}

static const char _data_FX_MODE_DEADLINE_TROPHY[] PROGMEM =
    "DEADLINE Trophy@BPM,!,Contour Intensity,!,!;!,!;!;2;c1=0,sx=110,pal=59";
// <-- cf. https://kno.wled.ge/interfaces/json-api/#effect-metadata
// <EffectParameters>;<Colors>;<Palette>;<Flags>;<Defaults>
// <Colors> = !,! = Two Defaults
// <Palette> = ! = Default enabled (Empty would disable palette selection)
// <Flags> = 2 for "it needs the 2D matrix", add "v" and/or "f" for AudioReactive volume and frequency.
// <Defaults>: <ParameterCode>=<Value>
//             si=0 is "sound interaction" (for AudioReactive),
// <EffectParameters>: These are then read by (if defined, comma-separated)
//   sx = SEGMENT.speed (int 0-255)
//   ix = SEGMENT.intensity (int 0-255)
//   c1 = SEGMENT.custom1 (int 0-255)
//   c2 = SEGMENT.custom2 (int 0-255)
//   c3 = SEGMENT.custom3 (int 0-31)
//   o1 = SEGMENT.check1 (bool)
//   o2 = SEGMENT.check2 (bool)
//   o3 = SEGMENT.check3 (bool)
