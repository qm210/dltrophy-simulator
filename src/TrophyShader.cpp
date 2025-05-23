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
    glUseProgram(program);
    iRect.loadLocation(program);
    iTime.loadLocation(program);
    iFPS.loadLocation(program);
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
}

void TrophyShader::onRectChange(Size resolution, const Config& config) {
    auto rect = config.shaderRect(resolution);
    iRect.value = glm::vec4(rect.x, rect.y, rect.width, rect.height);
    glViewport(rect.x, rect.y, rect.width, rect.height);
    initVertices();
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
    std::cout << "lastReload: " << lastReload.value() << std::endl;

    if (vertexShaderChanged) {
        std::cout << "Hot Reload due to Vertex Shader change: "
                  << vertex.filePath << std::endl;
    }
    if (fragmentShaderChanged) {
        std::cout << "Hot Reload due to Fragment Shader change: "
                  << fragment.filePath << std::endl;
    }
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

void TrophyShader::initUniformBuffers() {
    // pass LEDs as Uniform Buffers, but separated in
    // - the Definition (is set once)
    // - the RGB State (is updated frequently)

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
    glBufferSubData(GL_UNIFORM_BUFFER,
                    state->trophy->alignedSizeOfNumber(),
                    state->trophy->alignedSizeOfPositions(),
                    state->trophy->position.data()
    );

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

void TrophyShader::use() {
    if (!program.works()) {
        throw std::runtime_error("Cannot use program, because linking failed.");
    }
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(program);
}

void TrophyShader::render() {
    if (debugFlag) {
        debugFlag = false;
    }

    iRect.set();

    glBindBuffer(GL_UNIFORM_BUFFER, stateBufferId);
    int offset = 0;
    glBufferSubData(GL_UNIFORM_BUFFER,
                    offset,
                    state->alignedSizeForLeds(),
                    state->leds.data()
    );
    offset += state->alignedSizeForLeds();
    glBufferSubData(GL_UNIFORM_BUFFER,
                    offset,
                    sizeof(state->params),
                    &state->params
    );
    offset += sizeof(state->params),
    glBufferSubData(GL_UNIFORM_BUFFER,
                    offset,
                    sizeof(state->options),
                    &state->options
    );

    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

const std::pair<std::string, std::string> TrophyShader::lastReloadInfo() const {
    // first: message (last success time / fail), second: error message (empty string if success)
    std::pair<std::string, std::string> result{"", ""};
    if (!lastReload.has_value()) {
        return result;
    }
    if (reloadFailed) {
        result.first = "! FAILED !";
        result.second = collectErrorLogs();
    } else {
        auto tm = *std::localtime(&lastReload.value());
        result.first = std::format("-- last: {:02d}:{:02d}:{:02d}", tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
    return result;
}
