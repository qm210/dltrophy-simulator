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

#include "glHelpers.h"
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
    std::optional<std::time_t> lastReload;
    bool reloadFailed = false;
    Shader currentMode = TrophyView;

    GLuint vertexArrayObject = 0;
    GLuint vertexBufferObject = 0;
    void initVertices();
    static std::array<float, 18> createQuadVertices();

    ShaderState *state;
    GLuint stateBufferId = 0;
    GLuint definitionBufferId = 0;
    void initUniformBuffers();
    void fillStateUniformBuffer();

    FramebufferPingPong feedbackFramebuffers{};
    Framebuffer ledsOnly{};
    void initFramebuffers(const Rect& rect);

    ExtraOutputs extraOutputs{};
    GLuint extraOutputTexture[2];
    void handleExtraOutputs(int pingIndex);

    static constexpr GLenum extraOutputAttachment =
            GL_COLOR_ATTACHMENT1;
    static constexpr GLenum drawBuffers[] = {
            GL_COLOR_ATTACHMENT0,
            extraOutputAttachment,
    };

public:
    TrophyShader(const Config& config, ShaderState *state);
    ~TrophyShader();

    void use();
    void render();
    void onRectChange(Size resolution, const Config& config);

    void reload(const Config& config);
    void mightHotReload(const Config& config);
    const std::pair<std::string, std::string> lastReloadInfo() const;

    [[nodiscard]]
    std::string collectErrorLogs(std::optional<ProgramMeta> program = std::nullopt) const;
    bool assertSuccess(const std::function<void(const std::string&)>& callback) const;

    // TODO: this can surely be made more elegant, but pls. brain. quiet now.
    Uniform<glm::vec4> iRect = Uniform<glm::vec4>("iRect");
    Uniform<float> iTime = Uniform<float>("iTime");
    Uniform<float> iFPS = Uniform<float>("iFPS");
    Uniform<int> iFrame = Uniform<int>("iFrame");
    Uniform<int> iPass = Uniform<int>("iPass");
    Uniform<glm::vec4> iMouse = Uniform<glm::vec4>("iMouse");
    Uniform<int> iPreviousImage = Uniform<int>("iPreviousImage");
    Uniform<int> iBloomImage = Uniform<int>("iBloomImage");

    void updateLedPositions() const;

    bool shouldReadExtraOutputs = false;
    // these are for trying the PBO reading again, as soon as bog.
    int readingFromPingIndex = -1;
    int readInFrames = 0;
};

#endif //DLTROPHY_SIMULATOR_TROPHYSHADER_H
