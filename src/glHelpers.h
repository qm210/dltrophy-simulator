//
// Created by qm210 on 13.05.2025.
//

#ifndef DLTROPHY_SIMULATOR_GLHELPERS_H
#define DLTROPHY_SIMULATOR_GLHELPERS_H

#include <utility>
#include <variant>
#include <utility>
#include <optional>
#include <fstream>
#include <filesystem>
#include <string>
#include <map>
#include "FileHelper.h"
#include "geometryHelpers.h"

#include <glad/gl.h>
#include <glm/glm.hpp>


struct ShaderMeta {
    GLenum type;
    std::string source;
    GLuint id = 0;
    std::string error;
    std::string filePath;
    std::filesystem::file_time_type fileTime;

    explicit ShaderMeta(GLenum type): type(type) {}

    operator GLuint() const {
        return id;
    }

    void read(const std::string& path) {
        FileHelper::ensure(path);
        filePath = path;
        read();
    }

    void read() {
        std::ifstream file(filePath);
        std::stringstream buffer;
        buffer << file.rdbuf();
        source = buffer.str();
        fileTime = std::filesystem::last_write_time(filePath);
    }

    void takeEmbedded(const char embeddedSource[]) {
        filePath = "";
        source = std::string(embeddedSource);
    }

    void compile() {
        error = "";
        id = glCreateShader(type);
        const char* source_str = source.c_str();
        glShaderSource(id, 1, &source_str, nullptr);
        glCompileShader(id);
        GLint status;
        glGetShaderiv(id, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint length;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
            error.assign(length, ' ');
            glGetShaderInfoLog(id, length, nullptr, &error[0]);
            glDeleteShader(id);
        }
    }

    [[nodiscard]]
    bool fileHasChanged() const {
        if (usesEmbedded()) {
            return false;
        }
        auto time = std::filesystem::last_write_time(filePath);
        return fileTime != time;
    }

    [[nodiscard]]
    bool usesEmbedded() const {
        return filePath.empty();
    }
};

struct ProgramMeta {
    GLuint id;
    std::string error;

    operator GLuint() const {
        return id;
    }

    [[nodiscard]]
    bool works() const {
        return id > 0 && error.empty();
    }
};

template <typename T>
struct Uniform {
private:
    GLint location = -1;
    std::string name;
    bool triedLoadLocation = false;
    bool hasLocationWarned = false;

public:
    T value;

    explicit Uniform(std::string name)
            : name(std::move(name))
            {}

    void loadLocation(GLuint program) {
        location = glGetUniformLocation(program, name.c_str());
        triedLoadLocation = true;
    }

    void set() {
        if (location < 0) {
            if (!triedLoadLocation && !hasLocationWarned) {
                std::cerr << "Uniform was never initialized: " << name << std::endl;
                hasLocationWarned = true;
            }
            return;
        }
        if constexpr (std::is_same_v<T, float>) {
            glUniform1f(location, value);
        } else if constexpr (std::is_same_v<T, int>) {
            glUniform1i(location, value);
        } else if constexpr (std::is_same_v<T, glm::vec2>) {
            glUniform2f(location, value.x, value.y);
        } else if constexpr (std::is_same_v<T, glm::vec3>) {
            glUniform3f(location, value.x, value.y, value.z);
        } else if constexpr (std::is_same_v<T, glm::vec4>) {
            glUniform4f(location, value.x, value.y, value.z, value.w);
        } else {
            throw std::runtime_error("Uniform.readFrom() called for undefined type");
        }
    }

    void set(T to) {
        value = to;
        set();
    }

    std::string to_string() {
        std::string result;
        if constexpr (std::is_same_v<T, glm::vec4>) {
            result = std::format("vec4({}, {}, {}, {})", value.x, value.y, value.z, value.w);
        } else {
            // add other cases when they actually occur
            result = std::format("{}", value);
        }
        if (triedLoadLocation) {
            return std::format("{} = {} (location {})", name, result, location);
        } else {
            return std::format("{} = {} (no location)", name, result);
        }
    }

    void printDebug() const {
        std::string printValue = to_string();
        std::cout << "[Debug Uniform] " << to_string() << ";" << std::endl;
    }
};

struct Framebuffer {
    GLuint fbo;
    GLuint texture;
    GLenum status;
    GLenum attachment = GL_COLOR_ATTACHMENT0;
    std::string debugLabel;

    virtual void initialize() {
        teardown();
        glGenFramebuffers(1, &fbo);
        glGenTextures(1, &texture);
    }

    virtual void teardown() {
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &texture);
    }

    void assertStatus() {
        status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error(std::format(
                    "Error in Framebuffer \"{}\": {} ({})",
                    debugLabel, StatusMessages.at(status), status
            ));
        }
    }

    static inline const std::map<GLenum, std::string> StatusMessages = {
            {GL_FRAMEBUFFER_COMPLETE, "Framebuffer complete"},
            {GL_FRAMEBUFFER_UNDEFINED, "Framebuffer undefined"},
            {GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT, "Framebuffer incomplete attachment"},
            {GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT, "Framebuffer incomplete missing attachment"},
            {GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER, "Framebuffer incomplete draw pbo"},
            {GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER, "Framebuffer incomplete read pbo"},
            {GL_FRAMEBUFFER_UNSUPPORTED, "Framebuffer unsupported"},
            {GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE, "Framebuffer incomplete multisample"},
            {GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS, "Framebuffer incomplete layer targets"}
    };
};

struct FramebufferPingPong {
    static const int N = 2;
    std::array<GLuint, N> fbo{};
    std::array<GLuint, N> texture{};
    std::array<GLenum, N> status{};
    GLenum attachment = GL_COLOR_ATTACHMENT0;
    std::array<GLuint, N> pbo{};
    int pingCursor = 0;

    FramebufferPingPong() = default;

    void teardown() {
        glDeleteFramebuffers(N, &fbo[0]);
        glDeleteTextures(N, &texture[0]);
        glDeleteBuffers(N, &pbo[0]);
    }

    void initialize() {
        glGenTextures(N, &texture[0]);
        glGenFramebuffers(N, &fbo[0]);
        glGenBuffers(N, &pbo[0]);
    }

    std::pair<GLuint, GLuint> getOrder() const {
        auto pongCursor = (pingCursor + 1) % N;
        return {pingCursor, pongCursor};
    }

    std::pair<GLuint, GLuint> getOrderAndAdvance() {
        auto result = getOrder();
        pingCursor = result.second;
        return result;
    }

    void assertStatus(int i, const std::string& debugLabel = "") {
        status[i] = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status[i] != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error(std::format(
                    "Error in Ping Pong Framebuffer {}: {} ({}) {}",
                    i, Framebuffer::StatusMessages.at(status[i]), status[i], debugLabel
            ));
        }
    }
};

static inline void initFloatTexture(GLuint texture, Size size) {
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA32F,
                 size.width,
                 size.height,
                 0,
                 GL_RGBA,
                 GL_FLOAT,
                 nullptr);
    // Note: GL_TEXTURE_2D stays bound for now
}

static inline void attachFramebufferFloatTexture(GLuint texture, GLuint attachment, Size size) {
    initFloatTexture(texture, size);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           attachment,
                           GL_TEXTURE_2D,
                           texture,
                           0);
}


#endif //DLTROPHY_SIMULATOR_GLHELPERS_H
