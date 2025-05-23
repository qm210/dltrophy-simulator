//
// Created by qm210 on 10.05.2025.
//

#ifndef DLTROPHY_SIMULATOR_TROPHYSHADER_H
#define DLTROPHY_SIMULATOR_TROPHYSHADER_H

#include <string>
#include <utility>
#include <vector>
#include <functional>

#include <glad/gl.h>
#include <glm/vec2.hpp>

#include "shaderHelpers.h"
#include "Config.h"
#include "ShaderState.h"

class TrophyShader {

private:
    ShaderMeta vertex = ShaderMeta(GL_VERTEX_SHADER);
    ShaderMeta fragment = ShaderMeta(GL_FRAGMENT_SHADER);
    ProgramMeta program;
    ProgramMeta createProgram();
    void initializeProgram(const Config& config);
    void teardown();

    GLuint vertexArrayObject = 0;
    GLuint vertexBufferObject = 0;
    void initVertices();
    static std::array<float, 18> createQuadVertices();

    ShaderState *state;
    GLuint stateBufferId = 0;
    GLuint definitionBufferId = 0;
    void initUniformBuffers();

    std::optional<std::time_t> lastReload;
    bool reloadFailed = false;

public:
    TrophyShader(const Config& config, ShaderState *state);
    ~TrophyShader();

    // TODO: this can surely be made more elegant, but pls. brain. quiet now.
    Uniform<float> iTime = Uniform<float>("iTime");
    Uniform<glm::vec4> iRect = Uniform<glm::vec4>("iRect");
    Uniform<float> iFPS = Uniform<float>("iFPS");
    Uniform<glm::vec4> iMouse = Uniform<glm::vec4>("iMouse");

    void use();
    void render();
    void onRectChange(Size resolution, const Config& config);

    void reload(const Config& config);
    void mightHotReload(const Config& config);
    const std::pair<std::string, std::string> lastReloadInfo() const;

    [[nodiscard]]
    std::string collectErrorLogs(std::optional<ProgramMeta> program = std::nullopt) const;
    void assertSuccess(const std::function<void(const std::string&)>& callback) const;

    bool debugFlag = false;
};

#endif //DLTROPHY_SIMULATOR_TROPHYSHADER_H
