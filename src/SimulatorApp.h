//
// Created by qm210 on 10.05.2025.
//

#ifndef DLTROPHY_SIMULATOR_SIMULATORAPP_H
#define DLTROPHY_SIMULATOR_SIMULATORAPP_H

// these two are interconnected, keep in that order
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <string>
#include "UdpReceiver.h"
#include "ShaderState.h"
#include "TrophyShader.h"
#include "Config.h"
#include "inputHelpers.h"
#include "UdpInterpreter.h"
#include <imgui.h>

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
    ImGuiWindowFlags imguiFlags;
    void buildControlPanel();

    KeyMap keyMap;
    void initializeKeyMap();
    void handleKeyInput(int key, int scancode, int action, int mods);
    std::unordered_map<KeyCode, bool> keyDown;
    void handleMouseInput();

    TrophyShader* shader;

    float startTimestamp;
    float latestTimestamp;
    float currentTime;
    float previousTime;
    int currentFrame;
    float currentFps;
    void handleTime();

    static const int FPS_SAMPLES = 10;
    float lastFps[FPS_SAMPLES];
    float averageFps;
    float calcAverageFps();

    Trophy* trophy;
    ShaderState* state;

    UdpReceiver* receiver;
    void handleUdpMessages();
    std::optional<ProtocolMessage> lastMessage;

    static void showError(const std::string & message);
};

#endif //DLTROPHY_SIMULATOR_SIMULATORAPP_H
