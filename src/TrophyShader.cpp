//
// Created by qm210 on 10.05.2025.
//

#include <format>
#include "TrophyShader.h"
#include "shaderHelpers.h"

const std::string default_vertex_shader_path = "./shaders/vertex.glsl";
const std::string default_fragment_shader_path = "./shaders/fragment.glsl";

TrophyShader::TrophyShader(const Config& config, ShaderState *state)
: state(state) {
    vertex.read(FileHelper::first_if_exists(
            config.customVertexShaderPath,
            default_vertex_shader_path
    ));
    fragment.read(FileHelper::first_if_exists(
            config.customFragmentShaderPath,
            default_fragment_shader_path
    ));
    program = createProgram();
    initializeProgram(config);
}

TrophyShader::~TrophyShader() {
    teardown();
}

void TrophyShader::initializeProgram(const Config& config) {
    if (!glIsProgram(program)) {
        std::cerr << "[Shader Program] is not valid, error is \"" << program.error << "\"" << std::endl;
    };

    glUseProgram(program);
    iRect.loadLocation(program);
    iTime.loadLocation(program);
    iFPS.loadLocation(program);
    iFrame.loadLocation(program);
    iPass.loadLocation(program);
    iPreviousImage.loadLocation(program);
    iBloomImage.loadLocation(program);
    iMouse.loadLocation(program);

    initUniformBuffers();
    
    onRectChange(config.windowSize, config);
}

void TrophyShader::teardown() {
    if (program.works()) {
        glDeleteProgram(program);
    }
    glDeleteVertexArrays(1, &vertexArrayObject);
    glDeleteBuffers(1, &vertexBufferObject);
    glDeleteBuffers(1, &stateBufferId);
    glDeleteBuffers(1, &definitionBufferId);
    glDeleteFramebuffers(feedbackFramebuffers.object.size(), &feedbackFramebuffers.object[0]);
    glDeleteTextures(feedbackFramebuffers.texture.size(), &feedbackFramebuffers.texture[0]);
    glDeleteFramebuffers(1, &bloomFramebuffer.object);
    glDeleteTextures(1, &bloomFramebuffer.texture);
}

void TrophyShader::onRectChange(Size resolution, const Config& config) {
    auto rect = config.shaderRect(resolution);
    iRect.value = glm::vec4(rect.x, rect.y, rect.width, rect.height);
    glViewport(rect.x, rect.y, rect.width, rect.height);
    initVertices();
    initFramebuffers(rect);
}

void TrophyShader::reload(const Config& config) {
    vertex.read();
    fragment.read();

    lastReload = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now()
    );

    auto newProgram = createProgram();
    auto totalError = collectErrorLogs(newProgram);
    reloadFailed = !totalError.empty();
    if (reloadFailed) {
        std::cerr << totalError << std::endl;
        return;
    }

    teardown();
    program = newProgram;
    initializeProgram(config);
}

void TrophyShader::mightHotReload(const Config &config) {
    if (!config.hotReloadShaders) {
        return;
    }
    auto vertexShaderChanged = vertex.fileHasChanged();
    auto fragmentShaderChanged = fragment.fileHasChanged();
    if (!vertexShaderChanged && !fragmentShaderChanged) {
        return;
    }

    reload(config);
    auto info = lastReloadInfo();
    std::cout << "Hot Reload: " << info.first;
    if (!info.second.empty()) {
        std::cout << " -- " << info.second;
    }
    if (!fragmentShaderChanged) {
        std::cout << " -- Vertex Shader: " << vertex.filePath;
    }
    else if (!vertexShaderChanged) {
        std::cout << " -- Fragment Shader: " << fragment.filePath;
    }
    else {
        std::cout << " -- Fragment & Vertex Shader.";
    }
    std::cout << std::endl;
}

ProgramMeta TrophyShader::createProgram() {
    vertex.compile();
    fragment.compile();

    ProgramMeta prog;
    prog.id = glCreateProgram();
    glAttachShader(prog, vertex);
    glAttachShader(prog, fragment);
    glLinkProgram(prog);
    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(prog, 512, nullptr, &prog.error[0]);
        glDeleteProgram(prog);
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return prog;
}

std::string TrophyShader::collectErrorLogs(std::optional<ProgramMeta> otherProgram) const {
    std::string result;
    if (!vertex.error.empty()) {
        result += "Error in Vertex Shader: " + vertex.filePath + "\n" + vertex.error + "\n";
    }
    if (!fragment.error.empty()) {
        result += "Error in Fragment Shader: " + fragment.filePath + "\n" + fragment.error + "\n";
    }
    auto givenProgram = otherProgram.value_or(program);
    if (!givenProgram.error.empty()) {
        result += "Shader Linker Error:\n" + givenProgram.error + "\n";
    }
    return result;
}

void TrophyShader::assertSuccess(const std::function<void(const std::string &)> &callback) const {
    const auto errorLog = collectErrorLogs();
    if (!errorLog.empty()) {
        callback(errorLog);
    }
}

std::array<float, 18> TrophyShader::createQuadVertices() {
    const float left =  -1.f;
    const float bottom = -1.f;
    const float right = 1.f;
    const float top = 1.f;
    return {{
        left, bottom, 0.,
        right, top, 0.,
        left, top, 0.,
        left, bottom, 0.,
        right, bottom, 0.,
        right, top, 0.,
    }};
}

void TrophyShader::initVertices() {
    if (vertexArrayObject) {
        glDeleteVertexArrays(1, &vertexArrayObject);
    }
    if (vertexBufferObject) {
        glDeleteBuffers(1, &vertexBufferObject);
    }

    glGenVertexArrays(1, &vertexArrayObject);
    glBindVertexArray(vertexArrayObject);

    glGenBuffers(1, &vertexBufferObject);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);

    auto vertices = createQuadVertices();

    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(float),
                 vertices.data(),
                 GL_STATIC_DRAW
                 );

    constexpr GLint positionAttributeLocation = 0;
    // <-- vertex.glsl must match this. (obviöslich)
    //     could also use glGetAttribLocation(),
    //     but then names must match, so... little gained.

    glVertexAttribPointer(
            positionAttributeLocation,
            3,
            GL_FLOAT,
            GL_FALSE,
            3 * sizeof(float),
            (void*)0
            );
    glEnableVertexAttribArray(positionAttributeLocation);
}

static inline void attachFramebufferFloatTexture(
        GLuint fbo,
        GLuint textureId,
        Rect rect,
        GLuint colorAttachment
) {
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // the resolution must include the origin shift, I suppose.
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA32F,
                 rect.width + rect.x,
                 rect.height + rect.y,
                 0,
                 GL_RGBA,
                 GL_FLOAT,
                 nullptr);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           colorAttachment,
                           GL_TEXTURE_2D,
                           textureId,
                           0);
}

void TrophyShader::initFramebuffers(const Rect& rect) {
    // First the two for the Feedback Ping-Pong:
    const int n = feedbackFramebuffers.object.size();
    glDeleteFramebuffers(n, &feedbackFramebuffers.object[0]);
    glDeleteTextures(n, &feedbackFramebuffers.texture[0]);
    glGenTextures(n, &feedbackFramebuffers.texture[0]);
    glGenFramebuffers(n, &feedbackFramebuffers.object[0]);

    for (int i = 0; i < n; i++) {
        attachFramebufferFloatTexture(feedbackFramebuffers.object[i],
                                      feedbackFramebuffers.texture[i],
                                      rect,
                                      GL_COLOR_ATTACHMENT0);
        feedbackFramebuffers.status[i] = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        feedbackFramebuffers.assertStatus(i);
    }

    // Now the one for the Bloom Effect / Glowing LEDs:
    glDeleteFramebuffers(1, &bloomFramebuffer.object);
    glDeleteTextures(1, &bloomFramebuffer.texture);
    glGenTextures(1, &bloomFramebuffer.texture);
    glGenFramebuffers(1, &bloomFramebuffer.object);

    attachFramebufferFloatTexture(bloomFramebuffer.object,
                                  bloomFramebuffer.texture,
                                  rect,
                                  GL_COLOR_ATTACHMENT0);
    bloomFramebuffer.status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    bloomFramebuffer.assertStatus();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TrophyShader::initUniformBuffers() {
    // pass LEDs as Uniform Buffers, but separated in
    // - the Definition (is set once)
    // - the RGB State (is updated frequently)

    glDeleteBuffers(1, &stateBufferId);
    glDeleteBuffers(1, &definitionBufferId);

    GLuint bufferId[2];
    glGenBuffers(2, &bufferId[0]);
    stateBufferId = bufferId[0];
    definitionBufferId = bufferId[1];

    GLuint blockIndex;
    GLuint bindingPoint = 0;

    blockIndex = glGetUniformBlockIndex(program, "TrophyDefinition");
    glUniformBlockBinding(program, blockIndex, bindingPoint);
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, definitionBufferId);
    glBindBuffer(GL_UNIFORM_BUFFER, definitionBufferId);
    glBufferData(GL_UNIFORM_BUFFER,
                 state->trophy->alignedTotalSize(),
                 nullptr,
                 GL_STATIC_DRAW);
    glBufferSubData(GL_UNIFORM_BUFFER,
                    0,
                    sizeof(state->nLeds),
                    &state->nLeds
    );
    updateLedPositions();

    bindingPoint++;
    blockIndex = glGetUniformBlockIndex(program, "StateBuffer");
    glUniformBlockBinding(program, blockIndex, bindingPoint);
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, stateBufferId);
    glBindBuffer(GL_UNIFORM_BUFFER, stateBufferId);
    glBufferData(GL_UNIFORM_BUFFER,
                 state->alignedTotalSize(),
                 nullptr,
                 GL_DYNAMIC_DRAW);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void TrophyShader::updateLedPositions() const {
    // if you changed the public fields of state->trophy,
    // this will publish the changes into the uniform buffer.

    glBindBuffer(GL_UNIFORM_BUFFER, definitionBufferId);
    glBufferSubData(GL_UNIFORM_BUFFER,
                    state->trophy->alignedSizeOfNumber(),
                    state->trophy->alignedSizeOfPositions(),
                    state->trophy->position.data()
    );
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void TrophyShader::use() {
    if (!program.works()) {
        throw std::runtime_error("Cannot use program, because linking failed.");
    }

    glUseProgram(program);

    iRect.set();
}

inline void fillStateUniformBuffer(ShaderState* state) {
    int offset = 0;

    auto putIntoUniformBuffer = [&](size_t size, const void* data) {
        glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
        offset += size;
    };

    putIntoUniformBuffer(
            state->alignedSizeForLeds(),
            state->leds.data()
    );
    putIntoUniformBuffer(
            sizeof(state->params),
            &state->params
    );
    putIntoUniformBuffer(
            sizeof(state->options),
            &state->options
    );
}

const int ONLY_LEDS_PASS = 0;
const int SCENE_PASS = 1;
const int POST_PASS = 2;

void TrophyShader::render() {
    glBindBuffer(GL_UNIFORM_BUFFER, stateBufferId);
    fillStateUniformBuffer(state);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    iBloomImage.set(1);
    iPass.set(ONLY_LEDS_PASS);
    glBindFramebuffer(GL_FRAMEBUFFER, bloomFramebuffer.object);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bloomFramebuffer.texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    auto order = feedbackFramebuffers.getOrderAndAdvance();


    iPreviousImage.set(0);
    iPass.set(SCENE_PASS);
    glBindFramebuffer(GL_FRAMEBUFFER, feedbackFramebuffers.object[order.first]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, feedbackFramebuffers.texture[order.second]);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    iPass.set(POST_PASS);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

const std::pair<std::string, std::string> TrophyShader::lastReloadInfo() const {
    // .first  = message (last success time / fail)
    // .second = error message (empty string if success)
    std::pair<std::string, std::string> result{"", ""};
    if (!lastReload.has_value()) {
        return result;
    }
    if (reloadFailed) {
        result.first = "! FAILED !";
        result.second = collectErrorLogs();
    } else {
        auto tm = *std::localtime(&lastReload.value());
        result.first = std::format("-- last: {:02d}:{:02d}:{:02d}",
                                   tm.tm_hour,
                                   tm.tm_min,
                                   tm.tm_sec);
    }
    return result;
}
