//
// Created by qm210 on 14.05.2025.
//

#ifndef DLTROPHY_SIMULATOR_CONFIG_H
#define DLTROPHY_SIMULATOR_CONFIG_H

#include <string>
#include <filesystem>
#include <GLFW/glfw3.h>
#include <glm/vec4.hpp>
#include <nlohmann/json.hpp>
#include "geometryHelpers.h"
#include "ShaderState.h"

class Config {
public:
    const std::string defaultFilename = "smiuluator.config";
    std::filesystem::path path;

    Size windowSize {
        1080,
        720
    };

    int udpPort = 3413;

    std::string customVertexShaderPath;
    std::string customFragmentShaderPath;
    bool hotReloadShaders = true;
    bool useLogoDevelShader = false;

    bool usePrototyper = false;

    Config(int argc, char* argv[]);

    void store(GLFWwindow* window, ShaderState* state = nullptr) const;
    void restore(GLFWwindow* window);
    void restore(ShaderState* state);

    [[nodiscard]]
    bool wasRead() const { return currentJson != std::nullopt; };

    [[nodiscard]]
    Rect shaderRect(Size resolution) const {
        auto width = static_cast<float>(resolution.width);
        auto height = static_cast<float>(resolution.height);
        return Rect{
            {
                .width = static_cast<int>(shaderView.width * width),
                .height = static_cast<int>(shaderView.height * height),
            },
            static_cast<int>(shaderView.x * width),
            static_cast<int>(shaderView.y * height),
        };
    };

    [[nodiscard]]
    inline float relativeRemainingWidth() const {
        // assumes that .x is the left margin - might change
        const float margin = shaderView.x;
        return 1.f - shaderView.width - shaderView.x - margin;
    }

private:
    bool tryReadFile();
    std::optional<nlohmann::json> currentJson;

    [[nodiscard]]
    std::optional<nlohmann::json> tryReadJson() const;

    std::optional<Coord> windowPos = std::nullopt;
    RelativeRect shaderView{
            .width = 0.5,
            .height = 0.9,
            .x = 0.05,
            .y = 0.05,
    };
};

#endif //DLTROPHY_SIMULATOR_CONFIG_H
