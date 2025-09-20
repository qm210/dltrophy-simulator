//
// Created by qm210 on 10.05.2025.
//

#include <stdexcept>
#include <iostream>
#include <variant>

#include "SimulatorApp.h"
#include "UdpInterpreter.h"
#include "timeFormat.h"
#include "PerformanceMonitor.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_stdlib.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif


SimulatorApp::SimulatorApp(Config config)
: config(config) {

    window = initializeWindow();
    gladLoadGL(glfwGetProcAddress);

    const char* glVersion = (const char*)glGetString(GL_VERSION);
    std::cout << "Check OpenGL Version: " << glVersion << std::endl;

    if ((void*)glCreateShader == nullptr) {
        throw std::runtime_error("OpenGL not actually loaded - bad.");
    }

    glfwGetFramebufferSize(window, &area.width, &area.height);

    trophy = new Trophy();
    state = new ShaderState(trophy);
    config.restore(state);

    shader = new TrophyShader(config, state);
    shader->assertSuccess(showError);

    udpListener = new UdpListener(config.udpPort);

    // WIP: for measuring the FPS drops (goes to 1-2... then resizing the window solves the problem)
    // monitor = new PerformanceMonitor("perf.measure");

    prototyper = new Prototyper(config.usePrototyper);

    initializeKeyMap();
}

SimulatorApp::~SimulatorApp() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

GLFWwindow* SimulatorApp::initializeWindow() {
    if (!glfwInit()) {
        throw std::runtime_error("Cannot rebuild GLFW – and thus – anything.");
    }

    glfwSetErrorCallback(handleWindowError);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

    window = glfwCreateWindow(
            config.windowSize.width,
            config.windowSize.height,
            awesomeTitle.c_str(),
            nullptr,
            nullptr
    );

    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Could not create GLFW window.");
    }

    config.restore(window);

    glfwMakeContextCurrent(window);

    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto app = static_cast<SimulatorApp*>(
                glfwGetWindowUserPointer(window)
        );
        app->handleKeyInput(key, scancode, action, mods);
    });

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    imguiFlags = ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_AlwaysAutoResize;

    return window;
}

void SimulatorApp::handleWindowError(int error, const char* description) {
    std::cerr << "Error: " << description << " (" << error << ")" << std::endl;
}

void SimulatorApp::run() {
    startTimestamp = static_cast<float>(glfwGetTime());
    currentTime = 0;
    currentFrame = 0;
    currentFps = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        handleResize();
        buildControlPanel();
        handleMouseInput();

        handleTime();

        shader->use();
        shader->iTime.set(currentTime);
        shader->iFrame.set(currentFrame);
        shader->iFPS.set(averageFps);
        shader->iMouse.set();
        shader->render();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        if (prototyper->service()) {
            state->setFrom(prototyper->buildLeds());
        }
        else {
            handleMessages();
        }

        // monitor-> ... // TODO

        shader->mightHotReload(config);
    }

    config.store(window);
}

void SimulatorApp::handleTime() {
    currentFrame++;
    previousTime = currentTime;
    latestTimestamp = static_cast<float>(glfwGetTime());
    currentTime = latestTimestamp - startTimestamp;
    currentFps = 1.f / (currentTime - previousTime);

    int f = currentFrame % FPS_SAMPLES;
    lastFps[f] = currentFps;
    averageFps = currentFrame < FPS_SAMPLES ? currentFps : calcAverageFps();
}

float SimulatorApp::calcAverageFps() {
    float sum = 0.;
    for (float lastFp : lastFps) {
        sum += lastFp;
    }
    return sum / static_cast<float>(FPS_SAMPLES);
}

// there should be many implementations, but this one is enough for now
#ifdef _WIN32
    void SimulatorApp::showError(const std::string &message) {
        MessageBoxA(nullptr, message.c_str(), "Error", MB_OK | MB_ICONERROR);
        std::cerr << message << std::endl;
        std::exit(1);
    }
#else
    void SimulatorApp::showError(const std::string &message) {
        std::cerr << message << std::endl;
    }
#endif

void SimulatorApp::handleKeyInput(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS && !keyDown[key]) {
        keyDown[key] = true;
    }
    else if (action == GLFW_RELEASE && keyDown[key]) {
        try {
            auto& callback = keyMap.at(key);
            callback(mods);
        }
        catch (const std::out_of_range&) {}
    }
}

void SimulatorApp::initializeKeyMap() {
    keyMap = {{
        GLFW_KEY_ESCAPE,
        [this](int mods) {
            glfwSetWindowShouldClose(window, true);
        }
    }, {
        GLFW_KEY_F1,
        [this](int mods) {
            toggle(state->verbose);
        }
    }, {
        GLFW_KEY_F8,
        [this](int mods) {
            toggle(config.useLogoDevelShader);
            TrophyShader* newShader = new TrophyShader(config, state);
            if (newShader->assertSuccess(showError)) {
                delete shader;
                shader = newShader;
            }
        }
    }, {
        GLFW_KEY_F9,
        [this](int mods) {
            printDebug();
        }
    }, {
        GLFW_KEY_F10,
        [this](int mods) {
             prototyper->toggle();
        }
    }, {
        GLFW_KEY_G,
        [this](int mods) {
            toggle(state->options.showGrid);
        }
    }, {
        GLFW_KEY_A,
        [this](int mods) {
            toggle(state->options.accumulateForever);
        }
    }, {
        GLFW_KEY_S,
        [this](int mods) {
            toggle(state->options.noStochasticVariation);
        }
    }, {
        GLFW_KEY_F,
        [this](int mods) {
            toggle(state->options.onlyPyramidFrame);
        }
    }};
}

void SimulatorApp::handleMouseInput() {
    const float meansUnset = -1.;

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    // make window convention match shader coordinates
    mouseX = mouseX;
    mouseY = area.height - mouseY;

    if (outsideVec4Rect(mouseX, mouseY, shader->iRect.value)) {
        mouseX = mouseY = meansUnset;
    }
    shader->iMouse.value.x = mouseX;
    shader->iMouse.value.y = mouseY;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (shader->iMouse.value.z == meansUnset || shader->iMouse.value.w == meansUnset) {
            shader->iMouse.value.z = mouseX;
            shader->iMouse.value.w = mouseY;

            if (mouseX != meansUnset && mouseY != meansUnset) {
                shader->shouldReadExtraOutputs = true;
            }
        }
    } else {
        if (!shader->shouldReadExtraOutputs && shader->readingFromPingIndex < 0) {
            // TODO: for now, this keeps the coordinates until extraOutputs are read.
            shader->iMouse.value.z = meansUnset;
            shader->iMouse.value.w = meansUnset;
        }
    }
}

void SimulatorApp::handleMessages() {
    if (!udpListener->runsOn(config.udpPort)) {
        delete udpListener;
        udpListener = new UdpListener(config.udpPort);
    }
    auto udpPacket = udpListener->listen();

    if (!udpPacket.has_value()) {
        return;
    }

    auto message = UdpInterpreter::interpret(*udpPacket);

    std::visit([this](auto&& msg) {
        using T = std::decay_t<decltype(msg)>;

        if constexpr (std::is_same_v<T, ProtocolMessage>) {
            state->setFrom(msg.mapping);
            this->lastUdpMessage = msg;

        } else if constexpr (std::is_same_v<T, UnreadableMessage>) {
            if (this->state->verbose) {
                msg.printDebug(std::cout);
            }
        }

    }, message);
}

void SimulatorApp::handleResize() {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    if (width == area.width && height == area.height) {
        return;
    }
    area.width = width;
    area.height = height;
    shader->onRectChange(area, config);
}

void SimulatorApp::buildControlPanel() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    float width = static_cast<float>(area.width);
    float height = static_cast<float>(area.height);
    float panelMargin = 0.013 * width;
    float panelWidth = config.relativeRemainingWidth() * width - panelMargin;
    ImGui::SetNextWindowPos(
            ImVec2(width - panelWidth - panelMargin, panelMargin),
            ImGuiCond_Always
    );
    ImGui::SetNextWindowSize(
            ImVec2(panelWidth, height - 2. * panelMargin),
            ImGuiCond_Always
    );

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
    ImGui::Begin("Deadline Trophy Smiuluator", NULL, imguiFlags);
    ImGui::PopStyleVar();

    const int globalStyleVars = 4;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(6, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 18);

    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(210.f, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(210.f, 0.75f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(210.f, 0.8f, 0.9f));
    if (ImGui::Button("Reload Shader")) {
        shader->reload(config);
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::Checkbox("Hot Reload", &config.hotReloadShaders);

    auto reloaded = shader->lastReloadInfo();
    ImGui::SameLine();
    if (reloaded.second.empty()) {
        ImGui::Text(reloaded.first.c_str());
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), reloaded.first.c_str());

        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::HSV(210.f, 0.6f, 0.7f));
        ImGui::TextWrapped(reloaded.second.c_str());
        ImGui::PopStyleColor();

        ImGui::Spacing();
    }

    auto stop = 0.24f * panelWidth;
    ImGui::Text("Time:");
    ImGui::SameLine(stop);
    ImGui::Text("%6.2f sec.",
                shader->iTime.value);
    ImGui::Text("FPS:");
    ImGui::SameLine(stop);
    ImGui::Text("%6.2f",
                shader->iFPS.value);
    ImGui::Text("Resolution:");
    ImGui::SameLine(stop);
    ImGui::Text("%.0f x %.0f (Offset: %.0f x %.0f)",
                shader->iRect.value.z,
                shader->iRect.value.w,
                shader->iRect.value.x,
                shader->iRect.value.y);

    ImGui::Text("UDP Port:");
    ImGui::SameLine(stop);
    ImGui::InputInt("##UdpMessagePort",
                    &config.udpPort,
                    0, 0,
                    ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsDecimal);

    ImGuiHelper::JustifiedButtons({
        {"Randomize LEDs", [this]() {
            state->randomize();
        }},
        {"off", [this]() {
            state->setAll(0, 0, 0);
        }},
        {"Print Debug", [this]() {
            printDebug();
        }},
        {"Save", [this]() {
            config.store(window, state);
        }}
    });

    ImGui::PushItemWidth(0.32f * panelWidth);

    if (ImGui::CollapsingHeader("Trophy LEDs Definition")) {
        bool touched = false;
        touched |= ImGuiHelper::SlidersVec3(
                "Logo Center",
                &state->trophy->logoCenter.x, -.25f, +.25f,
                &state->trophy->logoCenter.y, 0.f, +.5f,
                &state->trophy->logoCenter.z, -.25f, +.25f,
                0.15f * panelWidth
        );
        touched |= ImGuiHelper::SlidersVec3(
                "Base Center",
                &state->trophy->baseCenter.x, -1.f, +1.f,
                &state->trophy->baseCenter.y, -1.f, +1.f,
                &state->trophy->baseCenter.z, -1.f, +1.f,
                0.15f * panelWidth
        );
        touched |= ImGuiHelper::SlidersVec3(
                "Logo/Base Dimensions",
                &state->trophy->logoSize.x, 0.1f, +2.f,
                &state->trophy->logoSize.y, 0.1f, +2.f,
                &state->trophy->baseSize, 0.f, +2.f,
                0.15f * panelWidth
        );
        if (touched) {
            state->trophy->rebuild();
            shader->updateLedPositions();
        }

        ImGui::SliderFloat("LED size",
                           &state->params.ledSize,
                           1.e-3, 3.e-2);
        ImGui::SliderFloat("LED glow",
                           &state->params.ledGlow,
                           0.01, 30.);
    }

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {

        ImGuiHelper::SlidersVec3(
                "Camera Origin",
                &state->params.camX, -1.f, +1.f,
                &state->params.camY, -1.f, +1.f,
                &state->params.camZ, -5.f, -1.f,
                0.15f * panelWidth
        );

        ImGui::SliderFloat("Camera FOV",
                           &state->params.camFov,
                           0.1f, 4.f);
        ImGui::SliderFloat("Camera Tilt",
                           &state->params.camTilt,
                           -60.f, 60.f);
    }

    if (ImGui::CollapsingHeader("Background Scenery")) {

        ImGui::SliderFloat("Fog Scaling",
                           &state->params.fogScaling,
                           1e-4, 1e-2);
        ImGui::SliderFloat("Fog Grading",
                           &state->params.fogGrading,
                           0.001f, 10.f);
        ImGui::SliderFloat("Background Spin",
                           &state->params.backgroundSpin,
                           0.f, 10.f);
        ImGui::SliderFloat("Synthwave Floor Level",
                           &state->params.floorLevel,
                           -10.f, 5.f);
        ImGui::SliderFloat("Synthwave Floor Graytone",
                           &state->params.floorGraytone,
                           0.f, 1.f);
        ImGui::SliderFloat("Synthwave Grid Brightness",
                           &state->params.floorLineBrightness,
                           0.f, 1.f);
        ImGui::SliderFloat("Synthwave Grid Thickness",
                           &state->params.floorLineWidth,
                           0.001f, 1.f);
        ImGui::SliderFloat("Synthwave Grid Exponent",
                           &state->params.floorExponent,
                           0.001f, 100.f);
        ImGui::SliderFloat("Synthwave Grid Glow",
                           &state->params.floorGrading,
                           0.001f, 20.f);
    }

    if (ImGui::CollapsingHeader("Pyramid (Trophy)", ImGuiTreeNodeFlags_DefaultOpen)) {

        ImGui::PushItemWidth(0.15f * panelWidth);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
        ImGuiHelper::SlidersVec3(
                "Pyramid Position",
                &state->params.pyramidX, -1.f, +1.f,
                &state->params.pyramidY, -1.f, +1.f,
                &state->params.pyramidZ, -1.f, +1.f
        );
        ImGui::PopStyleVar();
        ImGui::PopItemWidth();

        ImGui::SliderFloat("Pyramid Scale",
                           &state->params.pyramidScale,
                           1e-3f, 2.f);
        ImGui::SliderFloat("Pyramid Height",
                           &state->params.pyramidHeight,
                           0.01f, 2.f);
        ImGui::SliderFloat("Pyramid Rotation Angle",
                           &state->params.pyramidAngle,
                           -180.f, 180.f);
        ImGui::SliderFloat("Pyramid Angular Velocity",
                           &state->params.pyramidAngularVelocity,
                           -210.f, 210.f);
        ImGui::SameLine();
        if (ImGui::Button("0")) {
            state->params.pyramidAngle = 0;
            state->params.pyramidAngularVelocity = 0;
        }

        ImGui::SliderFloat("Pyramid Epoxy Permittivity",
                           &state->params.epoxyPermittivity,
                           0.01f, 10.f);
    }

    if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {

        ImGui::PushItemWidth(0.24f * panelWidth);

        ImGui::SliderFloat("##MaxDistance",
                           &state->params.traceMaxDistance,
                           1., 1000.);
        ImGui::SameLine();
        ImGui::SliderInt("##MaxRecursions",
                         &state->params.traceMaxRecursions,
                         1, 32);
        ImGui::SameLine();
        ImGui::Text("max. Depth & Recursions");

        ImGui::PopItemWidth();

        ImGui::SliderFloat("Previous Image Blend Factor",
                           &state->params.blendPreviousMixing,
                           0.f, 1.f);
        ImGui::Checkbox("Accumulate Forever (ignores Blend Factor)",
                        &state->options.accumulateForever);
        ImGui::Checkbox("No Stochastic Variation (indeed a bit nonsense)",
                        &state->options.noStochasticVariation);
        ImGui::Checkbox("Only Pyramid Frame",
                        &state->options.onlyPyramidFrame);
    }

    ImGui::PopItemWidth();

    ImGui::PopStyleVar(globalStyleVars);
    ImGui::End();
}

void SimulatorApp::printDebug() const {
    trophy->printDebug();

    auto options = *reinterpret_cast<const int32_t*>(&state->options);
    std::cout << "[Debug State] int options = " << options << " -> 32 bit: ";
    for (int i = 31; i >= 0; --i) {
        auto bit = (options >> i) & 1;
        std::cout << bit;
        if (i % 8 == 0) {
            std::cout << " ";
        }
    }
    std::cout << std::endl;

    std::cout << "[UdpListener] # Packages Received: " << udpListener->receivedPackages() << std::endl;

    if (!lastUdpMessage) {
        std::cout << "[UdpListener] got no interpretable Message so far." << std::endl;
        return;
    }

    std::cout << "[UdpListener] last Message from "
              << formatTime(lastUdpMessage->timestamp) << std::endl;
    for (auto const& [index, led]: lastUdpMessage->mapping) {
        std::cout << "    Index " << std::setw(3) << static_cast<int>(index)
                  << ": " << led.toString() << std::endl;
    }
    std::cout << "    Source: " << lastUdpMessage->source << std::endl;

    std::cout << " === iRect  = " << shader->iRect.to_string() << std::endl;
    std::cout << " === iMouse = " << shader->iMouse.to_string() << std::endl;
}
