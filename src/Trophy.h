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

    glm::vec3 logoCenter = {-0.175f, 0.262f, 0.f};
    glm::vec2 logoSize = {0.5f, 0.375f};
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
                relative = parse_logo_order(i - logoStartIndex);
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

    static const int N_LOGO_WIDTH = 27;
    static const int N_LOGO_HEIGHT = 21;
    static const int N_LOGO_ORDER = N_LOGO_WIDTH * N_LOGO_HEIGHT;
    static const int _ = -1;
    static constexpr std::array<int, N_LOGO_ORDER> logo_order = {{
         _,  _,  _, 97,  _, 98,  _, 99,  _,100,  _,101,  _,102,  _,103,  _,104,  _,105,  _,  _,  _,  _,  _,  _,  _,
         _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
         _,  _, 96,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
         _, 95,  _, 94,  _, 93,  _, 92,  _, 91,  _, 90,  _, 89,  _, 88,  _, 87,  _, 86,  _, 85,  _,  _,  _,  _,  _,
         _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
        73,  _, 74,  _, 75,  _, 76,  _, 77,  _, 78,  _, 79,  _, 80,  _, 81,  _, 82,  _, 83,  _, 84,  _,  _,  _,  _,
         _, 72,  _, 71,  _, 70,  _, 69,  _, 68,  _, 67,  _, 66,  _, 65,  _, 64,  _, 63,  _, 62,  _, 61,  _,  _,  _,
         _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
         _,  _,  0,  _,  1,  _,  2,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _, 59,  _, 60,  _,  _,
         _,  _,  _,  5,  _,  4,  _,  3,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _, 58,  _, 57,  _, 56,  _,
         _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
         _,  _,  _,  _,  6,  _,  7,  _,  8,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _, 53,  _, 54,  _, 55,  _,  _,
         _,  _,  _,  _,  _, 11,  _, 10,  _,  9,  _,  _,  _,  _,  _,  _,  _,  _,  _, 52,  _, 51,  _, 50,  _,  _,  _,
         _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
         _,  _,  _,  _,  _,  _, 12,  _, 13,  _, 14,  _,  _,  _,  _,  _,  _,  _, 47,  _, 48,  _, 49,  _,  _,  _,  _,
         _,  _,  _,  _,  _,  _,  _, 17,  _, 16,  _, 15,  _,  _,  _,  _,  _,  _,  _, 46,  _, 45,  _,  _,  _,  _,  _,
         _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
         _,  _,  _,  _,  _,  _,  _,  _, 18,  _, 19,  _, 20,  _, 21,  _, 22,  _, 23,  _, 24,  _, 25,  _, 26,  _,  _,
         _,  _,  _,  _,  _,  _,  _,  _,  _, 35,  _, 34,  _, 33,  _, 32,  _, 31,  _, 30,  _, 29,  _, 28,  _, 27,  _,
         _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
         _,  _,  _,  _,  _,  _,  _,  _,  _,  _, 36,  _, 37,  _, 38,  _, 39,  _, 40,  _, 41,  _, 42,  _, 43,  _, 44,
    }};

    static glm::vec2 parse_logo_order(size_t logo_index) {
        auto it = std::find(logo_order.begin(), logo_order.end(), logo_index);
        auto index = (it != logo_order.end())
                ? static_cast<size_t>(std::distance(logo_order.begin(), it))
                : logo_order.size();
        auto indexX = double(index % N_LOGO_WIDTH);
        auto indexY = -double((index - indexX) / N_LOGO_WIDTH);
        glm::vec2 result{
                -0.5 + indexX / double(N_LOGO_WIDTH),
                -0.5 + indexY / double(N_LOGO_HEIGHT)
        };
        auto cos60 = cos(60. * M_PI/180.);
        auto sin60 = sin(60. * M_PI/180.);
        return {
                cos60 * result.x - sin60 * result.y - 0.5,
                sin60 * result.x + cos60 * result.y - 0.5,
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
//
//        if (edge_index > 0 && edge_index < 3) {
//            int y_index = (base_index % (2 * N_EDGE)) / 2;
//            return glm::vec2{
//                    -0.5f + (base_index % 2),
//                    -0.5f + step_length * (1 + y_index)
//            };
//        }
//        else {
//            int x_index = base_index % N_EDGE;
//            return glm::vec2{
//                    -0.5f + step_length * (1 + x_index),
//                    -0.5f + (edge_index > 0)
//            };
//        }
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
