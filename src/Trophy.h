//
// Created by qm210 on 20.05.2025.
//

#ifndef DLTROPHY_SIMULATOR_TROPHY_H
#define DLTROPHY_SIMULATOR_TROPHY_H

#include <cstddef>
#include <array>
#include <functional>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <GL/gl.h>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

struct Trophy {

    static constexpr GLuint N_LEDS_IN_BASE = 64;
    static constexpr GLuint N_LEDS_IN_LOGO = 106;
    static constexpr GLuint N_RGB_LEDS = N_LEDS_IN_BASE + N_LEDS_IN_LOGO;
    static constexpr GLuint N_SINGLE_LEDS = 2;
    static constexpr GLuint N_LEDS = N_RGB_LEDS + N_SINGLE_LEDS;

    // could be considered configurable (it's all WLED config...), but are fixed for now.
    static constexpr int baseStartIndex = 0;
    static constexpr int logoStartIndex = N_LEDS_IN_BASE;
    static constexpr int backLedIndex = N_LEDS - 2;
    static constexpr int floorLedIndex = N_LEDS - 1;

    std::array<glm::vec4, N_LEDS> position{};

    std::array<bool, N_LEDS> isSingleColor{};
    std::array<bool, N_LEDS> isLogo{};
    std::array<bool, N_LEDS> isBase{};

    glm::vec3 logoCenter = {-.025f, 0.26f, 0.f};
    glm::vec2 logoSize = {0.75f, 0.35f};
    glm::vec3 baseCenter = {0.f, -.35f, 0.f};
    float baseSize = 1.0f;
    glm::vec3 backLedPos{-0.05f, -0.1f, 0.02f};
    glm::vec3 floorLedPos{0.0f, -.35f, 0.f};

    glm::vec3 posMin{}, posMax{};

    Trophy() {
        rebuild();
    }

    void rebuild() {
        posMin = {0, 0, 0};
        posMax = {0, 0, 0};

        for (int i = 0; i < N_LEDS; i++) {

            isBase[i] = i >= baseStartIndex && i < baseStartIndex + N_LEDS_IN_BASE;
            isLogo[i] = i >= logoStartIndex && i < logoStartIndex + N_LEDS_IN_LOGO;
            isSingleColor[i] = !isLogo[i] && !isBase[i];

            glm::vec2 relative;
            glm::vec3 absolute;

            if (isBase[i]) {
                relative = calc_base_order(i - baseStartIndex);
                absolute = glm::vec3{
                        baseCenter.x + baseSize * relative.x,
                        baseCenter.y,
                        baseCenter.z + baseSize * relative.y
                };
            }
            else if (isLogo[i]) {
                relative = parse_logo_order(i);
                absolute = glm::vec3{
                        logoCenter.x + logoSize.x * relative.x,
                        logoCenter.y + logoSize.y * relative.y,
                        logoCenter.z
                };
            }
            else {
                if (i == floorLedIndex) {
                    absolute = floorLedPos;
                }
                else if (i == backLedIndex) {
                    absolute = backLedPos;
                } else {
                    std::cout << "[Trophy LED] Unclear what LED #" << i << " is supposed to be =/" << std::endl;
                }
            }

            position[i] = glm::vec4{
                absolute.x,
                absolute.y,
                absolute.z,
                0.f
            };

            posMin.x = std::min(posMin.x, position[i].x);
            posMin.y = std::min(posMin.y, position[i].y);
            posMin.z = std::min(posMin.z, position[i].z);
            posMax.x = std::max(posMax.x, position[i].x);
            posMax.y = std::max(posMax.y, position[i].y);
            posMax.z = std::max(posMax.z, position[i].z);
        }
    }

    static const int N_LOGO_WIDTH = 26;
    static const int N_LOGO_HEIGHT = 12;
    static const int N_LOGO_ORDER = N_LOGO_WIDTH * N_LOGO_HEIGHT;
    static const int _ = -1;
    static constexpr std::array<int, N_LOGO_ORDER> logo_order = {{
        _,  _,  _,  _,  _,  _,  _,  _,169,  _,  _,149,  _,148,125,  _,124,120,  _,  _,  _,  _,  _,  _,  _,  _,
        _,  _,  _,  _,  _,  _,  _,168,  _,  _,150,  _,147,126,  _,123,121,  _,119,  _,  _,  _,  _,  _,  _,  _,
        _,  _,  _,  _,  _,  _,167,  _,  _,151,  _,146,127,  _,  _,122,  _,118,114,  _,  _,  _,  _,  _,  _,  _,
        _,  _,  _,  _,  _,166,  _,  _,152,  _,145,128,  _,  _,  _,  _,117,115,  _,113,  _,  _, 90, 91,  _,108,
        _,  _,  _,  _,165,  _,  _,153,  _,144,129,  _,  _,  _,  _,  _,116,  _,112,109,  _, 89, 92,  _,107,  _,
        _,  _,  _,164,  _,  _,154,  _,143,130,  _,  _,  _,  _,  _,  _,  _,111,110,  _, 88, 93,  _,106,  _,  _,
        _,  _,163,  _,  _,155,  _,142,131,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _, 87, 94,  _,105,  _,  _,  _,
        _,162,  _,  _,156,  _,141,132,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _, 86, 95,  _,104,  _,  _,  _,  _,
      161,  _,  _,157,  _,140,133,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _, 85, 96,  _,103,  _,  _,  _,  _,  _,
        _,160,158,  _,139,134,  _, 66, 67,  _, 72, 73,  _, 78, 79,  _, 84, 97,  _,102,  _,  _,  _,  _,  _,  _,
        _,159,  _,138,135,  _, 65, 68,  _, 71, 74,  _, 77, 80,  _, 83, 98,  _,101,  _,  _,  _,  _,  _,  _,  _,
        _,  _,137,136,  _, 64, 69,  _, 70, 75,  _, 76, 81,  _, 82, 99,  _,100,  _,  _,  _,  _,  _,  _,  _,  _,
    }};

    static glm::vec2 parse_logo_order(int logo_index) {
        auto it = std::find(logo_order.begin(), logo_order.end(), logo_index);
        auto index = static_cast<int>(
                it != logo_order.end()
                ? std::distance(logo_order.begin(), it)
                : logo_order.size()
        );
        auto indexX = double(index % N_LOGO_WIDTH);
        auto indexY = -double((index - indexX) / N_LOGO_WIDTH);
        return {
            -.5 + indexX / double(N_LOGO_WIDTH),
            -.5 + indexY / double(N_LOGO_HEIGHT)
        };
    }

    static constexpr glm::vec2 baseCorner[4] = {
        {+0.5f, +0.5f},
        {+0.5f, -0.5f},
        {-0.5f, -0.5f},
        {-0.5f, +0.5f},
    };

    static glm::vec2 calc_base_order(int base_index) {
        int N_EDGE = N_LEDS_IN_BASE / 4;
        int edge_index = base_index / N_EDGE;

        // Note: "Shifted" by 0.5f because of the corner margin (no LED there)
        const float shift = 0.5f;
        auto step_in_edge = static_cast<float>(base_index % N_EDGE) + shift;
        auto step_length = 1.f / (static_cast<float>(N_EDGE - 1) + 2 * shift);

        auto from = baseCorner[edge_index % 4];
        auto to = baseCorner[(edge_index + 1) % 4];
        glm::vec2 delta = (to - from) * step_length;
        return from + delta * step_in_edge;
    }

    size_t alignedTotalSize() {
        return alignedSizeOfNumber() + alignedSizeOfPositions();
    }

    size_t alignedSizeOfNumber() {
        // first element is N_LEDS as uint -> 4 bytes -> too small.
        // from then on, the offset of an array must be a multiple of its base data size
        return sizeof(position[0]);
    }
    
    size_t alignedSizeOfPositions() {
        // position is now vec4 to match the alignment
        return position.size() * sizeof(position[0]);
    }

    void printDebug() {
        auto nLength = std::to_string(N_LEDS).length();
        std::cout << "=== DEBUG TROPHY LED POSITIONS === N = " << N_LEDS << std::endl;

        for (int i = 0; i < N_LEDS; i++) {

            if (i == logoStartIndex) {
                std::cout << "  LOGO:" << std::endl;
            } else if (i == baseStartIndex) {
                std::cout << "  BASE:" << std::endl;
            } else if (i == N_RGB_LEDS) {
                std::cout << "  WHITE-ONLY:" << std::endl;
            }

            auto p = position[i];
            // remember, the unused p.w component is only there for alignment, not used.
            std::cout << "    " << std::setw(nLength) << std::setfill('0') << i << ": "
                << p.x << ", " << p.y << ", " << p.z << std::endl;
        }

        std:: cout << "-> Ranges: " << std::endl
                   << "   X [" << posMin.x << ", " << posMax.x << "]" << std::endl
                   << "   Y [" << posMin.y << ", " << posMax.y << "]" << std::endl
                   << "   Z [" << posMin.z << ", " << posMax.z << "]" << std::endl;
        std::cout << "==================================" << std::endl;
    }

};

#endif //DLTROPHY_SIMULATOR_TROPHY_H
