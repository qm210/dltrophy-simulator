//
// Created by qm210 on 10.05.2025.
//

#ifndef DLTROPHY_SIMULATOR_SHADERSTATE_H
#define DLTROPHY_SIMULATOR_SHADERSTATE_H

#include <cstdint>
#include <array>
#include <span>
#include <functional>
#include <format>
#include <optional>
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "Trophy.h"
#include "LED.h"
#include "geometryHelpers.h"
#include "debug.h"

enum Shader {
    TrophyView,
    // the LogoDevel is WIP, i.e. when it is not explicitly referenced, it is not meant.
    LogoDevel,
};

struct ShaderOptions {
    // this needs minimum alignment (GLint = 4 bytes), therefore a multiple of 4 of flags.
    // (well, at the end of the std140 layout this could even deviate, but don't rely on that.)
    bool showGrid;
    bool accumulateForever;
    bool noStochasticVariation;
    bool onlyPyramidFrame;
};

struct LogoDevelOptions {
    bool maskOnlyPixels;
    bool hidePixels;
    bool hideCrosshair;
    bool hideOrigin;
};

struct Parameters {
    // if only floats / ints / other 4 byte-types, this makes alignment easy
    // therefore e.g. do not keep camera position as vec3 -> gets annoying
    float ledSize, ledGlow;
    float camX, camY, camZ, camFov, camTilt;
    float fogScaling, fogGrading, backgroundSpin;
    float floorLevel, floorGraytone, floorLineBrightness,
          floorSpacingX, floorSpacingZ,
          floorLineWidth, floorExponent, floorGrading;
    float pyramidX, pyramidY, pyramidZ,
          pyramidScale, pyramidHeight,
          pyramidAngle, pyramidAngularVelocity;
    float epoxyPermittivity;
    float blendPreviousMixing;
    float traceMinDistance, traceMaxDistance, traceFixedStep;
    int traceMaxSteps, traceMaxRecursions;
    float ledBlurSamples, ledBlurRadius, ledBlurPrecision,
          ledBlurMixing;
    // remember: what is added here, should be cared about
    // - in Config.cpp -> NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Parameters, ...)
    // - include default values in the current smiululator.config, if present
};

struct ShaderState {
    Trophy* trophy;
    GLuint nLeds;
    std::vector<LED> leds;

    Parameters params {
        .ledSize = 0.015,
        .ledGlow = 8.,
        .camX = 0.,
        .camY = 0.17,
        .camZ = -1.8,
        .camFov = 1.7,
        .camTilt = 12.3,
        .fogScaling = 0.0001,
        .fogGrading = 2.2,
        .backgroundSpin = 0.1,
        .floorLevel = -4.,
        .floorGraytone = 0.1,
        .floorLineBrightness = 0.8,
        .floorSpacingX = 2.21,
        .floorSpacingZ = 5.21,
        .floorLineWidth = .05,
        .floorExponent = 30.,
        .floorGrading = 0.5,
        .pyramidX = 0.,
        .pyramidY = -.5,
        .pyramidZ = 0.,
        .pyramidScale = 1.26,
        .pyramidHeight = 0.85,
        .pyramidAngle = 0.,
        .pyramidAngularVelocity = 0.,
        .epoxyPermittivity = 1.6,
        .blendPreviousMixing = 0.35,
        .traceMinDistance = 1.e-3, // no reason to change
        .traceMaxDistance = 60., // past the pyramid, we trace the plane analytically anyway
        .traceFixedStep = 0.1,
        .traceMaxSteps = 40,
        .traceMaxRecursions = 6,
        .ledBlurSamples = 32.,
        .ledBlurRadius = 4.8,
        .ledBlurPrecision = 420.,
        .ledBlurMixing = 0.6,
    };

    ShaderOptions options {
        .showGrid = false,
        .accumulateForever = false,
        .noStochasticVariation = false,
        .onlyPyramidFrame = true,
    };

    // --> for Logo-Devel-Shader
    glm::vec4 logoGeometry {};
    LogoDevelOptions logoDevelOptions {
        .maskOnlyPixels = false,
        .hidePixels = false,
        .hideCrosshair = false,
        .hideOrigin = false,
    };
    // <-- for Logo-Devel-Shader

    bool verbose = false;

    explicit ShaderState(Trophy* trophy):
        trophy(trophy),
        nLeds(trophy->position.size())
        {
            leds.resize(nLeds);
            for (int i = 0; i < nLeds; i++) {
                leds[i] = LED();
            }
            logoGeometry = {
                    trophy->logoCenter.x,
                    trophy->logoCenter.y,
                    trophy->logoSize.x,
                    trophy->logoSize.y
            };
        };

    GLsizei alignedTotalSize(Shader mode) {
        GLsizei baseSize = alignedSizeForLeds();
        switch (mode) {
            case Shader::TrophyView:
                return baseSize
                       + sizeof(params)
                       + sizeof(options);
            case Shader::LogoDevel:
                // vec4 -> size and center of the 2D logo (... just why, again?)
                return baseSize
                        + sizeof(glm::vec4)
                        + sizeof(logoDevelOptions);
            default:
                return baseSize;
        }
    }

    GLsizei alignedSizeForLeds() {
        return static_cast<GLsizei>(leds.size()) * sizeof(leds[0]);
    }

    void set(size_t index, LED led, bool silent = false) {
        if (index >= nLeds) {
            if (silent) {
                return;
            }
            throw std::runtime_error(
                    std::format("Trophy has no LED at index {0}", index)
            );
        }
        if (trophy->isSingleColor[index]) {
            leds[index].set(led.gray());
        } else {
            leds[index].set(led);
        }
    }

    void set(size_t index, GLuint r = 0, GLuint g = 0, GLuint b = 0) {
        set(index, LED(r, g, b));
    }

    void setFrom(std::unordered_map<uint8_t, LED> mapping) {
        for (const auto& [index, led] : mapping) {
            set(index, led);
        }
    }

    void setFrom(std::vector<LED> mapping) {
        for (size_t i = 0; i < mapping.size(); i++) {
            set(i, mapping[i]);
        }
    }

    // The following functions are here for debugging / development

    void set(std::function<LED(size_t)> func) {
        for (int i = 0; i < nLeds; i++) {
            auto led = func(i);
            set(i, led);
        }
    }

    void setAll(uint8_t r, uint8_t g, uint8_t b) {
        auto led = LED(r, g, b);
        set([&led](size_t index) {
            return led;
        });
    }

    void randomize() {
        for (int i = 0; i < nLeds; i++) {
            uint8_t r = std::rand() % 256;
            uint8_t g = std::rand() % 256;
            uint8_t b = std::rand() % 256;
            set(i, r, g, b);
        }
    }
};

struct ExtraOutputs {
    // we allow ourselves to get one extra vec4 (i.e. four extra floats)
    // from the rendering. these are then distinguished in here.

private:
    Rect rect_;
    std::vector<float> values;
    std::vector<float> testOutput_;
    std::optional<int> clickedLedIndex_;
    std::vector<float> unusedZ_;
    std::vector<float> unusedW_;

    static constexpr float noLedClicked = -1.f;
    static constexpr float uninitialized = -0.123f;

public:
    float* data() { return values.data(); }
    const Rect& rect() { return rect_; }
    const std::optional<int> clickedLedIndex() { return clickedLedIndex_; }

    void initialize(Rect rect) {
        rect_ = rect;
        auto totalSize = rect.extent().area();
        values = std::vector<float>(4 * totalSize, uninitialized);
        testOutput_.resize(totalSize);
        clickedLedIndex_ = std::nullopt;
    }

    void interpretValues(glm::vec4 iMouse) {
        glm::vec4* vecValues = reinterpret_cast<glm::vec4*>(values.data());
        clickedLedIndex_ = std::nullopt;

        int rangeMinX = 10000;
        int rangeMaxX = -1;
        int rangeMinY = 10000;
        int rangeMaxY = -1;
        int rangeMinLedIndex = 10000;
        int rangeMaxLedIndex = -10000;

        for (int y = 0; y < rect_.height; y++) {
            for (int x = 0; x < rect_.width; x++) {
                auto index = x + rect_.width * y;
                auto value = vecValues[index];
                testOutput_[index] = value.x;

                if (value.x != 0.) {
                    // std::cout << "Woah! " << x << ", " << y << " = " << thing << std::endl;
                    rangeMinX = std::min(rangeMinX, x);
                    rangeMaxX = std::max(rangeMaxX, x);
                    rangeMinY = std::min(rangeMinY, y);
                    rangeMaxY = std::max(rangeMaxY, y);

                    if (value.y != noLedClicked) {
                        auto ledIndex = static_cast<int>(value.y);
                        clickedLedIndex_ = static_cast<int>(value.y);
                        rangeMinLedIndex = std::min(rangeMinLedIndex, ledIndex);
                        rangeMaxLedIndex = std::max(rangeMaxLedIndex, ledIndex);
                    }
                }

                if (samePixel(x, y, iMouse.z, iMouse.w)) {
                        std::cout << "Cursor: " << iMouse.z
                                  << ", " << iMouse.w << "; ";
                        print("ExtraOutput", value);
                }
            }
        }
        std::cout << " -- LedIndex: " << rangeMinLedIndex
                  << " .. " << rangeMaxLedIndex << " -- "
                  << std::endl;
    }
};

#endif //DLTROPHY_SIMULATOR_SHADERSTATE_H
