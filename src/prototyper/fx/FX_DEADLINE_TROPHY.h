#pragma once

#include "FX.h"
#include "../usermods/DEADLINE_TROPHY/DeadlineTrophy.h"

extern uint16_t mode_static(void);

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

inline float bpm(float time) {
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
    if (time < t1) {
        return bpm1;
    } else if (time < t12) {
        return bpm1 + (bpm2 - bpm1) * (time - t1) / (t12 - t1);
    } else if (time < t2) {
        return bpm2;
    } else if (time < t23) {
        return bpm2 + (bpm3 - bpm2) * (time - t2) / (t23 - t2);
    } else if (time < t3) {
        return bpm3;
    } else if (time < t34) {
        return bpm3 + (bpm4 - bpm3) * (time - t3) / (t34 - t3);
    } else {
        return bpm4;
    }
}

struct Spark {
    DeadlineTrophy::Vec2 pos;
    DeadlineTrophy::Vec2 vel;
    float size;
};

uint16_t mode_DeadlineTrophy(void) {
    using namespace DeadlineTrophy;
    using namespace DeadlineTrophy::FxHelpers;
    CRGB segcolor0 = CRGB(SEGCOLOR(0));
    CHSV color = rgb2hsv_approximate(segcolor0);
    CHSV color_(color);
    float hue = static_cast<float>(color.hue);

    static float beat = 0.;
    static float time = 0.;
    float elapsed = secondNow() - time;
    beat += bpm(time)/60. * elapsed;
    time += elapsed;

    float fractBeat = fmodf(beat, 1.);
    float halfBeat = floor(beat * 2.) * 0.5;

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
        float lineY = triangleLeft.y - unit;
        if (beat < 18.) {
            if (beat < 1.5) {
                lineColor = CHSV(230, 200, uint8_t(170. * beat / 1.5));
            } else {
                lineColor = CHSV(uint8_t(230 - (halfBeat - 3.) / (8. - 3.)), 200, uint8_t(170. * expf(-(beat - 1.5))));
                lineY += (halfBeat - 3.);
            }
        }

        static std::vector<Spark> sparks;
        static float sparkedAtBeat = 4.;

        for (auto& spark : sparks) {
            spark.pos += spark.vel * elapsed;
        }
        if (beat > 4. && beat < 18. && beat - sparkedAtBeat > 0.5) {
            float rand_x = 0.005 * float(random(-20, 20));
            float rand_y = -0.3 + 0.02 * float(random(20));
            sparks.push_back({
                { rand_x, rand_y },
                { 0.2f * rand_x, -unit },
                0.1f
            });
            sparkedAtBeat = beat;
            // recycle them later
        }

        for (const auto& coord : logoCoordinates()) {
            if (beat < 18.) {
                d = coord.sdLine({-10., lineY}, {10., lineY});
                color = mixHsv(CHSV(0, 0, 0), lineColor, exp(-3. * d));

                for (auto& spark : sparks) {
                    d = coord.gaussAt(spark.pos, spark.size);
                    color = mixHsv(color, CHSV(98, 255, 255), d);
                }
            }
            else if (beat < 21.) {
                // TODO:
                float mixB = (beat - 18.) / 3.;
                color = CHSV(
                        98,
                        mix8(255, 0, mixB),
                        mix8(255, 140, mixB)
                );
            }
            else if (beat < 36.) {
                static CHSV beatColor = CHSV(210, 128, 200);
                sparkedAtBeat = beat;
                if (beat >= sparkedAtBeat + 1.) {
                    beatColor = CHSV(
                            hw_random8(160, 230),
                            hw_random8(128, 255),
                            hw_random8(200, 255)
                    );
                    sparkedAtBeat = beat;
                }
                color = mixHsv(color, beatColor, fractBeat);
            }
            setLogo(coord.x, coord.y, color);

//
//            float d = coord.uv.length();
//            d = sin_t(M_TWOPI * (d * k * k_factor + w * beat));
//            d *= d;
//            d = 0.02 / d;
//
//            // nach Polarwinkel unterdrücken...
//            float theta = coord.uv.polarAngleFrom({0, 0}, 0.25 * beat * M_TWOPI);
//            // aber reicht alle 8 Takte mal.
//            float every8Beats = fmodf(beat, 8.);
//            float gaussCurveExponent = (every8Beats - 6.) / 2;
//            float swirlIntensity = exp(-sq(gaussCurveExponent));
//            d *= exp(-theta * swirlIntensity);
//
//            // at some other time, make k large and suppress points
//            gaussCurveExponent = (fmodf(beat, 16.) - 9.) / 2.4;
//            k_factor = 1. + exp(-sq(gaussCurveExponent));
//            d = powf(d, sq(k_factor));
//
//            color.hue = static_cast<uint8_t>(hue - 3. * theta);
//            color.sat = color_.sat;
//            color.val = static_cast<uint8_t>(50. * clip(d));
//
//            // as an example of controlling the parameters. draws a gauss curve / circle around teh given point, white.
//            /*
//            d = 255. * coord.gaussAt(center, 0.5 * k, w - 1.);
//            color.val = max(color.val, static_cast<uint8_t>(d));
//            color.sat = min(color.sat, static_cast<uint8_t>(255. - d));
//            */
//
//            // draw one triangle to show usage of left/right tilt vectors
//            CHSV triangleColor = CHSV(230, 200, 170);
//            Vec2 pointForLeft = triangleLeft + (-8.f + 24.f * perlin1D(10. * beat)) * Logo::xUnit;
//            d = coord.sdLine(pointForLeft - 10. * Logo::tiltRight, pointForLeft + 10. * Logo::tiltRight);
//            color = mixHsv(color, triangleColor, exp(-6. * d));
//            Vec2 pointForRight = triangleRight + (3.f - 7.f * perlin1D(10. * beat + 4.)) * Logo::xUnit;
//            d = coord.sdLine(pointForRight - 10. * Logo::tiltLeft, pointForRight + 10. * Logo::tiltLeft);
//            color = mixHsv(color, triangleColor, exp(-7. * d));
//            float bottomLineHeight = triangleLeft.y + Logo::unit * (-1.f + 5.f * perlin1D(6. * beat + 100.));
//            d = coord.sdLine({-10., bottomLineHeight}, {10., bottomLineHeight});
//            color = mixHsv(color, triangleColor, exp(-8. * d));


        }

        // now, about these random draws we did somewhere above
        /*
        if (SEGMENT.aux0 > 150) {
            // these are just two exp decays one eighth of a beat apart
            float lastExpPeakAt = fractBeat < 0.125 ? 0. : 0.125;
            float exponent = fractBeat < 0.125 ? 5. : 1.4;
            float flashIntensity =
                exp(-exponent * (fractBeat-lastExpPeakAt) * static_cast<float>(fractBeat >= lastExpPeakAt));
            if (fractBeat < 0.125) {
                fillLogoArray(Logo::InnerTriangle.data(), Logo::InnerTriangle.size(),
                              RGBW32(200, 255, 50, 0), flashIntensity);
            } else {
                fillLogoArray(Logo::OuterTriangle.data(), Logo::OuterTriangle.size(),
                              RGBW32(210, 110, 10, 0), flashIntensity);
            }
        }
        */

        /*
        static uint32_t contourColor = uint32_t(CRGB(30, 40, 120));
        int contourIndex = static_cast<int>(beat * 8.) % Contour.size();
        // some contour frames
        if (contourIndex == 0) {
            fillLogoArray(Contour.data(), Contour.size(), contourColor, 0.01);
        } else if (contourIndex == 1) {
            fillLogoArray(MiddleTriangle.data(), MiddleTriangle.size(), YELLOW, 0.003);
            fillLogoArray(LeftmostBar.data(), LeftmostBar.size(), CYAN, 0.004);
        } else if (contourIndex == 2) {
            fillLogoArray(InnerTriangle.data(), InnerTriangle.size(), WHITE, 0.01);
            fillLogoArray(OuterTriangle.data(), OuterTriangle.size(), CYAN, 0.004);
        }
        */
        // did have one bright wandering point but it was too ugly
        // Coord wanderPixel = coord(Contour[Contour.size() - 1 - contourIndex]);
        // setLogo(wanderPixel.x, wanderPixel.y, contourColor);
        // // and another, as a tail
        // if (contourIndex < Contour.size() - 1) {
        //     wanderPixel = coord(Contour[Contour.size() - 2 - contourIndex]);
        //     uint32_t paleContour = mixRgb(BLACK, contourColor, 0.07);
        //     setLogo(wanderPixel.x, wanderPixel.y, paleContour);
        // }

        // and change colors when a round is completed
        /*
        if (contourIndex == Contour.size() - 1) {
            contourColor = uint32_t(CRGB(30, 90 + perlin8(SEGMENT.call) % 80, 170));
        }
        */
    }

    if (IS_BASE_SEGMENT) {
        static float phi = 0.;
        static float omega = 1.7;

        SEGMENT.fadeToBlackBy(20);

        for (const auto& coord : baseCoordinates()) {
            FloatRgb col0 = cosinePalette(
                0.2 * time,
                {0.29, 0.22, 0.5},
                {0.43, 0.74, 0.74},
                {1.3, 0.7, 1.2},
                {0.85, 0.14, 0.83}
            );
            // read the "uvS" coordinate as s == 0 in the middle of every side, becoming (-)0.5 towards the corners
            float uvs = min(abs(coord.uv.x), abs(coord.uv.y));
            float d = (1. - cos_t(M_PI * 5. * uvs)) * (0.6 + 0.4 * sin_t(M_TWOPI * 0.125 * beat));
            col0.scale(d*d);
            col0.grade(0.8);
            float dimTheMiddle = 1. - 0.8 * cos(M_PI * uvs);
            col0.scale(dimTheMiddle);

            // now add a rotating line, showing the use of SDF geometry
            phi = 0.25 * beat * omega;
            Vec2 end{1, 1};
            end.rotate(phi);
            d = coord.sdLine(-end, end);
            d = exp(-2.5*d);

            color = col0.toCHSV();
            color.sat = mix8(255, 20, d);
            color.val = max(color.val, static_cast<uint8_t>(255.*d));
            setBase(coord.x, coord.y, color);
        }
    }

    // for the first argument of beatsin8_t (accum88 type), "BPM << 8" just oscillates once per beat
    //uint8_t fourBeatSineWave = beatsin8_t(SEGMENT.speed << 6, 0, 255);
    uint8_t beatSineWave = beatsin8_t(SEGMENT.speed << 8, 0, 255);
    uint8_t annoyingBlink = mix8(255, 0, exp(-fractBeat));

    if (IS_BACK_LED) {
        setSingle(annoyingBlink);

    } else if (IS_FLOOR_LED) {
        setSingle(fmodf(beat, 4.) < 1. ? beatSineWave : BLACK);

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
