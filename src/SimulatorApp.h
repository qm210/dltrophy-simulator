//
// Created by qm210 on 10.05.2025.
//

#ifndef DLTROPHY_SIMULATOR_SIMULATORAPP_H
#define DLTROPHY_SIMULATOR_SIMULATORAPP_H

// these two are interconnected, keep in that order
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <string>
#include <imgui.h>
#include "ShaderState.h"
#include "TrophyShader.h"
#include "Config.h"
#include "inputHelpers.h"
#include "UdpListener.h"
#include "UdpInterpreter.h"
#include "prototyper/Prototyper.h" // <-- WIP
#include "PerformanceMonitor.h" // <-- not finished

class SimulatorApp {
public:
    const std::string awesomeTitle = "QM's DL Trophy Smiuluator";

    explicit SimulatorApp(Config config);
    ~SimulatorApp();

    void run();

    void printDebug() const;

private:
    Config config;

    GLFWwindow* window;
    GLFWwindow* initializeWindow();
    static void handleWindowError(int error, const char* description);
    Size area;
    void handleResize();
    ImGuiWindowFlags imguiFlags = 0;
    void buildControlPanel();

    static void showError(const std::string & message);

    KeyMap keyMap;
    void initializeKeyMap();
    void handleKeyInput(int key, int scancode, int action, int mods);
    std::unordered_map<KeyCode, bool> keyDown;
    void handleMouseInput();

    TrophyShader* shader;

    float startTimestamp = 0;
    float latestTimestamp = 0;
    float currentTime = 0;
    float previousTime = 0;
    int currentFrame = 0;
    float currentFps = 0;
    void handleTime();
    void resetTime();

    static const int FPS_SAMPLES = 10;
    float lastFps[FPS_SAMPLES];
    float averageFps;
    float calcAverageFps();

    Trophy* trophy;
    ShaderState* state;

    UdpListener* udpListener;
    std::optional<ProtocolMessage> lastUdpMessage;
    void handleMessages();

    Prototyper* prototyper;

    PerformanceMonitor* monitor;
};

#endif //DLTROPHY_SIMULATOR_SIMULATORAPP_H
