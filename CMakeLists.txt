cmake_minimum_required(VERSION 3.22)
project(dltrophy_simulator)
set(CMAKE_CXX_STANDARD 20)

option(LINK_STATIC "Build with static linkage" OFF)

find_package(OpenGL REQUIRED)
include(FetchContent)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        8e15281d34a8b9ee9271ccce38177a3d812456f8
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        docking
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.11.0
)
FetchContent_MakeAvailable(json)

set(USE_ZLIB OFF CACHE BOOL "Enable zlib support" FORCE)
FetchContent_Declare(
        ixwebsocket
        GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
        GIT_TAG v11.4.6
)
FetchContent_MakeAvailable(ixwebsocket)

set(BUILD_MinimalCppSocket_SAMPLES OFF CACHE BOOL "Build MinimalSocket samples" FORCE)
FetchContent_Declare(
        min_sock
        GIT_REPOSITORY https://github.com/andreacasalino/Minimal-Socket
        GIT_TAG        master
)
FetchContent_MakeAvailable(min_sock)


# copy the shader sources into the build folder (relevant for debug)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
     FILES_MATCHING PATTERN "*.glsl"
)

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/SimulatorApp.cpp
        src/SimulatorApp.h
        src/TrophyShader.cpp
        src/TrophyShader.h
        src/Config.cpp
        src/Config.h
        src/Trophy.h
        src/inputHelpers.h
        src/geometryHelpers.h
        src/debug.h
        src/shaders/embedded.h

        src/messages/WebSocketListener.h
        src/messages/UdpListener.h
        src/messages/UdpInterpreter.h
        src/messages/timeFormat.h
        src/monitor/PerformanceMonitor.h
        src/monitor/Observables.h

        src/prototyper/Prototyper.h
        src/prototyper/fx/FX.h
        src/prototyper/usermods/DEADLINE_TROPHY/DeadlineTrophy.cpp
        src/prototyper/fx/wled/shim.cpp
        src/prototyper/fx/wled/wled_math.cpp
        src/prototyper/fx/wled/const.h
        src/prototyper/fx/lib/pixeltypes.h
        src/prototyper/fx/lib/lib8tion.h
        src/prototyper/fx/lib/hsv2rgb.h
        src/prototyper/fx/lib/fastled_defines.h
        src/prototyper/fx/lib/hsv2rgb.cpp
        src/prototyper/fx/lib/colorutils.cpp
        src/prototyper/Prototyper.cpp
)

target_sources(${PROJECT_NAME} PRIVATE
        thirdparty/glad/src/gl.c
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC
        src/messages
        src/monitor
        thirdparty/glad/include
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${imgui_SOURCE_DIR}/misc/cpp/

        src/prototyper
        src/prototyper/fx
)
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_sources(${PROJECT_NAME} PRIVATE
        thirdparty/freegetopt/getopt.c
    )
    target_include_directories(${PROJECT_NAME} PRIVATE
        thirdparty/freegetopt/
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        OpenGL::GL
        glm::glm
        nlohmann_json::nlohmann_json
        ixwebsocket
        MinimalSocket
)


if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(X11 REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_LIBRARIES})
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
        $<$<NOT:$<CONFIG:Debug>>:USE_EMBEDDED_SHADERS>
)

if(LINK_STATIC)
    message(STATUS "Linking statically, user should not need libstdc++-6.dll or libgcc_s_seh-1.dll on their own")
    target_link_options(${PROJECT_NAME} PRIVATE -static -static-libgcc -static-libstdc++ -lwinpthread)
    # <-- is for gcc, for msvc it would need to be something along - but I never tested:
    # set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# also, some tools for testing, debugging or wholesome jux
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools")
    add_subdirectory(tools)
endif()
