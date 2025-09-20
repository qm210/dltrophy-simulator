//
// Created by qm210 on 14.05.2025.
//

#include <fstream>
#include <format>
#include <iostream>
#include <getopt.h>
#include "Config.h"
#include "FileHelper.h"

using nlohmann::json;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        ShaderOptions,
        showGrid, accumulateForever, noStochasticVariation, onlyPyramidFrame
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        Parameters,
        ledSize, ledGlow,
        camX, camY, camZ, camFov, camTilt,
        fogScaling, fogGrading, backgroundSpin,
        floorLevel, floorGraytone, floorLineBrightness,
        floorSpacingX, floorSpacingZ,
        floorLineWidth, floorExponent, floorGrading,
        pyramidX, pyramidY, pyramidZ,
        pyramidScale, pyramidHeight,
        pyramidAngle, pyramidAngularVelocity,
        epoxyPermittivity, blendPreviousMixing,
        traceMinDistance, traceMaxDistance, traceFixedStep,
        traceMaxSteps, traceMaxRecursions,
        ledBlurSamples, ledBlurRadius, ledBlurPrecision, ledBlurMixing
)

inline void overwrite_if_path_exists(int opt, int targetOpt, std::string& target) {
    if (opt != targetOpt) {
        return;
    }
    if (std::filesystem::exists(optarg)) {
        target = optarg;
    } else {
        std::cerr << "Ignore given path, as it is invalid: " << optarg << std::endl;
    }
}

Config::Config(int argc, char **argv) {
    std::string configFilename = defaultFilename;
    std::string overwriteVertexShaderPath;
    std::string overwriteFragmentShaderPath;

    int opt;
    while ((opt = getopt(argc, argv, "c:f:v:")) != -1) {
        if (opt == '?') {
            std::cerr << "Invalid option: " << opt << std::endl;
            continue;
        }
        overwrite_if_path_exists(opt, 'c', configFilename);
        overwrite_if_path_exists(opt, 'f', overwriteFragmentShaderPath);
        overwrite_if_path_exists(opt, 'v', overwriteVertexShaderPath);
    }

    path = std::filesystem::path(configFilename);
    tryReadFile();

    if (!overwriteVertexShaderPath.empty()) {
        customVertexShaderPath = overwriteVertexShaderPath;
    }
    if (!overwriteFragmentShaderPath.empty()) {
        customFragmentShaderPath = overwriteFragmentShaderPath;
    }
}

void Config::restore(GLFWwindow* window) {
    if (!wasRead()) {
        return;
    }

    if (windowPos.has_value()) {
        windowPos->moveToLarger(totalMonitorMinimum());
        glfwSetWindowPos(window, windowPos->x, windowPos->y);
    }

    auto rect = Rect::query(window);
    if (windowSize.width > 0) {
        rect.width = windowSize.width;
    }
    if (windowSize.height > 0) {
        rect.height = windowSize.height;
    }
    glfwSetWindowSize(window, rect.width, rect.height);
}

std::optional<json> Config::tryReadJson() const {
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::nullopt;
    }
    json result;
    file >> result;
    return result;
}

bool Config::tryReadFile() {
    try {
        currentJson = tryReadJson();
        if (!currentJson.has_value()) {
            return false;
        }

        json jWindow = (*currentJson)["window"];
        if (jWindow.is_object()) {
            windowSize.width = jWindow.value(
                    "width",
                    windowSize.width
            );
            windowSize.height = jWindow.value(
                    "height",
                    windowSize.height
            );
            windowPos = std::optional(Coord{
                .x = jWindow.value("x", 0),
                .y = jWindow.value("y", 0)
            });
        }

        json jView = (*currentJson)["view"];
        if (jView.is_object()) {
            shaderView.x = jView.value("x", shaderView.x);
            shaderView.y = jView.value("y", shaderView.y);
            shaderView.width = jView.value("width", shaderView.width);
            shaderView.height = jView.value("height", shaderView.height);
        }

        json jShaders = (*currentJson)["shaders"];
        if (jShaders.is_object()) {
            customVertexShaderPath = jShaders.value("vertex", "");
            customFragmentShaderPath = jShaders.value("fragment", "");
            hotReloadShaders = jShaders.value("reload", hotReloadShaders);
            useLogoDevelShader = jShaders.value("useLogoDevelShader", useLogoDevelShader);
            logoDevelShaderPath = jShaders.value("logoDevel", logoDevelShaderPath);
        }

        udpPort = currentJson->value("udpPort", udpPort);
        usePrototyper = currentJson->value("usePrototyper", usePrototyper);

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error initializing Config: " << e.what() << std::endl;
    }
    return false;
}

void Config::store(GLFWwindow* window, ShaderState* state) const {
    json j = tryReadJson().value_or(json::object());
    auto rect = Rect::query(window);
    j["window"] = {
        {"x", rect.x},
        {"y", rect.y},
        {"width", rect.width},
        {"height", rect.height}
    };
    j["view"] = {
       {"x", shaderView.x},
       {"y", shaderView.y},
       {"width", shaderView.width},
       {"height", shaderView.height},
    };
    j["shaders"] = {
       {"vertex", customVertexShaderPath},
       {"fragment", customFragmentShaderPath},
       {"reload", hotReloadShaders},
       {"useLogoDevelShader", useLogoDevelShader},
       {"logoDevel", logoDevelShaderPath},
    };
    j["udpPort"] = udpPort;
    j["usePrototyper"] = usePrototyper;

    if (state != nullptr) {
        j["params"] = state->params;
        j["options"] = state->options;
        j["trophy"] = {
            {"logo", {
                 {"x", state->trophy->logoCenter.x},
                 {"y", state->trophy->logoCenter.y},
                 {"z", state->trophy->logoCenter.z},
                 {"width", state->trophy->logoSize.x},
                 {"height", state->trophy->logoSize.y},
            }},
            {"base", {
                 {"x", state->trophy->baseCenter.x},
                 {"y", state->trophy->baseCenter.y},
                 {"z", state->trophy->baseCenter.z},
                 {"size", state->trophy->baseSize},
            }},
        };
    }

    try {
        std::ofstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error(
                    std::format("Cannot open file {0}", path.string())
            );
        }

        file << j.dump(4);

    } catch (const std::exception& e) {
        std::cerr << "Error storing Config: " << e.what() << std::endl;
    }
}

void Config::restore(ShaderState* state) {
    if (!wasRead() && !tryReadFile()) {
        std::cerr << "Could not restore state, because could not read file." << std::endl;
        return;
    }
    if (currentJson->contains("params")) {
        auto paramsJson = currentJson->at("params");
        auto params = paramsJson.get<Parameters>();
        state->params = params;
    }

    if (currentJson->contains("options")) {
        auto optionsJson = currentJson->at("options");
        state->options = optionsJson.get<ShaderOptions>();
    }

    if (currentJson->contains("trophy")) {
        auto trophyJson = currentJson->at("trophy");
        std::cout << "Rebuilding Trophy from stored config: " << trophyJson << std::endl;

        json logoJson = trophyJson["logo"];
        state->trophy->logoCenter.x = logoJson["x"];
        state->trophy->logoCenter.y = logoJson["y"];
        state->trophy->logoCenter.z = logoJson["z"];
        state->trophy->logoSize.x = logoJson["width"];
        state->trophy->logoSize.y = logoJson["height"];

        json baseJson = trophyJson["base"];
        state->trophy->baseCenter.x = baseJson["x"];
        state->trophy->baseCenter.y = baseJson["y"];
        state->trophy->baseCenter.z = baseJson["z"];
        state->trophy->baseSize = baseJson["size"];

        state->trophy->rebuild();
    }

}