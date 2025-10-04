//
// Created by qm210 on 25.07.2025.
//

#ifndef DLTROPHY_SIMULATOR_PROTOTYPER_H
#define DLTROPHY_SIMULATOR_PROTOTYPER_H

/**
 * This class aims to connect FX_DEADLINE_TROPHY.h with the Simulator
 * by "shimming" all required definitions and global variables in WLED.

 * therefore this explicit includes:
 *  - wled.h = our hollow shell of WLED, a shadow of its former strength
 *  - DeadlineTrophy.h = in the best case, an exact copy / symlink to the header in the firmware
 *  - note that FX_DEADLINE_TROPHY.h must be included over the .cpp (singular translation unit)
 */

#include "wled.h"
#include "../usermods/DEADLINE_TROPHY/DeadlineTrophy.h"

#include <chrono>
#include <vector>
#include <iostream>

#define WORKAROUND_GL
#include "../LED.h"

typedef uint16_t (*mode_ptr)();


class Prototyper {
private:
    bool enabled = false;
    std::chrono::steady_clock::time_point startedAt;

    static mode_ptr _mode;

public:
    explicit Prototyper(bool enabled)
    : enabled(enabled)
    {
        // TODO: can I somehow check whether the sources are even available,
        //       and if not, overwrite enable to always false? -- find out.

        setUpDeadlineTrophy();

        startedAt = std::chrono::steady_clock::now();
    }

    /**
     *  this mocks(*) FX_fcn.cpp / WS2812FX::service()
     *  but I didn't want the WS2812FX polyfill to contain much logic,
     *  rather expose everything it has and let the Prototyper() be in control.
     *
     *  (*) as in, making fun of. No, but really,
     *      I figured it best to keep the structure as original as I could,
     *      but declared some stuff not-as-interesting-right-now
     *      TODO: check whether we should care about the SKIPPED parts.
     */
    bool service() const {
        if (!enabled) {
            return false;
        }

        auto nowUp = millis();
        strip.now = nowUp + strip.timebase;

        /* // ... SKIP? that timing calculaterino at the begningning
        auto elapsed = nowUp - strip._lastServiceShow;
        if (elapsed < MIN_FRAME_DELAY) {
            return;
        }
        if (elapsed < strip._frametime) {
            return;
        }
        */

        // SKIPPED: the _isServicing flag because only used as a "hardware lock" (waitForIt())

        bool doShow = false;

        strip._segment_index = 0;
        for (auto &seg : strip._segments)
        {
            // SKIPPED: check whether isActive()

            if (nowUp > seg.next_time) {
                doShow = true;
                unsigned frameDelay = FRAMETIME;

                if (!seg.freeze) {
                    beginDraw(seg);
                    strip._currentSegment = &seg;

                    // This calls the actual FX, originally (*_mode[seg.mode])()
                    frameDelay = _mode();
                    seg.call++;

                    // SKIPPED: segment blending / transitions
                }

                seg.next_time = nowUp + frameDelay;
            }
            strip._segment_index++;
        }

        if (doShow) {
            // SKIPPED: Segment::handleRandomPalette()
            strip._lastServiceShow = nowUp; // unclear whether used
            show();
        }

        return doShow;
    }

    void toggle() {
        enabled ^= true;
        if (enabled) {
            restart();
        }
    }

    void restart() const {
        strip.timebase = 0UL - millis();
        for (auto &seg : strip._segments) {
            seg.call = 0;
            for (size_t i = 0; i < seg.length(); i++) {
                seg.pixels[i] = BLACK;
            }
        }
    }

    [[nodiscard]]
    unsigned long millis() const {
        auto elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - startedAt
                );
        return static_cast<unsigned long>(elapsed.count());
    }

    std::vector<LED> buildLeds() {
        // we do not want to expose any of our dodgy magic in here.
        // rather create some new memory every access.

        size_t segmentIndex = 0;
        size_t indexInSegment = 0;
        std::vector<LED> leds;

        for (size_t i = 0; i < strip._length; i++) {
            auto pixel = getPixel(i);
            LED led(R(pixel), G(pixel), B(pixel));

            Segment& seg = strip.getSegment(segmentIndex);
            if (!seg.on) {
                led = LED();
            } else if (isWhiteOnly(seg)) {
                led.set(led.gray());
            }

            leds.push_back(led);

            indexInSegment++;
            if (indexInSegment >= seg.length()) {
                segmentIndex++;
            }
        }

        return leds;
    }

private:

    void setUpDeadlineTrophy() const {
        Segment::maxWidth = DeadlineTrophy::logoW;
        Segment::maxHeight = DeadlineTrophy::logoH + DeadlineTrophy::baseSize;
            // <-- could just be logoH, right? (the larger one of these).
            // TODO: check the firmware if maxHeight can be logoH - could improve memory & performance

        strip._segments.assign(
                DeadlineTrophy::segment,
                DeadlineTrophy::segment + DeadlineTrophy::N_SEGMENTS
        );
        strip._mainSegment = 1; // is the Deadline Logo
        strip.isMatrix = true;

        strip._length = DeadlineTrophy::N_LEDS_TOTAL;
        strip._pixels = new uint32_t[strip._length]{};

        for (size_t i = 0; i < DeadlineTrophy::N_SEGMENTS; i++) {
            Segment& seg = strip.getSegment(i);
            seg.on = true;
            seg._capabilities = DeadlineTrophy::segmentCapabilities[i];
            seg.name = (char*)DeadlineTrophy::segmentName[i];

            // these are just my current defaults in the firmware colors:
            seg.colors[0] = 0xAA00FF;
            seg.colors[1] = 0xFF40FF;
            seg.colors[2] = 0xFFFFFF;

            // idea: these could be used to send UDP information (fake Audio signal etc.)
            seg.aux0 = 0;
            seg.aux1 = 0;
            seg.custom1 = 0;
            seg.custom2 = 0;

            // no idea whether we have any use for changing these...
            seg.freeze = false;
            seg.map1D2D = 0;
        }
    }

    void beginDraw(const Segment& seg) const {
        // SKIPPED: the transition progress argument (uint16_t prog = 0xFFFFU)
        setDrawDimensions(seg);
        for (unsigned i = 0; i < NUM_COLORS; i++) {
            Segment::_currentColors[i] = seg.colors[i];
        }
        // SKIPPED: loadPalette
        // SKIPPED: transition stuff
    }

    void setDrawDimensions(const Segment& seg) const {
        Segment::_vWidth = seg.virtualWidth();
        Segment::_vHeight = seg.virtualHeight();
        Segment::_vLength = seg.virtualLength();
    }

    void show() const {
        /*
         * We obviously do not need any of the hardware or extra stuff from WS2812FX::show()
         * i.e. this just reads all the segments into the overall pixels.
         *      there is also no blending of any kind (segments do not overlap)
        */

        for (size_t i = 0; i < strip._length; i++) {
            strip._pixels[i] = BLACK;
        }
        for (Segment &seg : strip._segments) {
            if (!seg.isActive() || !seg.on) {
                continue;
            }

            auto width = seg.width();
            auto height = seg.height();
            for (size_t y = 0; y < height; y++)
            for (size_t x = 0; x < width; x++) {
                auto indexInSegment = x + y * width;
                auto indexInMapping = (x + seg.start) + (y + seg.startY) * Segment::maxWidth;
                auto indexInTrophy = DeadlineTrophy::mappingTable[indexInMapping];

                if (indexInTrophy < strip._length) {
                    strip._pixels[indexInTrophy] = seg.getPixelColorRaw(indexInSegment);
                }
            }
        }
    }

    uint32_t getPixel(size_t index) const {
        if (index >= strip._length || !strip._pixels) {
            return BLACK;
        }
        return strip._pixels[index];
    }

    bool isWhiteOnly(const Segment& seg) const {
        return (seg._capabilities & SEG_CAPABILITY_RGB) == 0;
    }

};

#endif //DLTROPHY_SIMULATOR_PROTOTYPER_H
