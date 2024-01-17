#include <iostream>

#include "flight_data/drone_flight_data.h"
#include "glfw/glfw.h"
#include "glfw/glfw_surface.h"
#include "visualizer.h"

void setupInput(dfv::raii::Glfw &glfw, dfv::Visualizer &visualizer);

/*
 * The main entrypoint of the drone flight visualizer.
 *
 * Command line usage: drone_flight_visualizer $1
 * $1: Drone CSV flight_data
 */
int main(const int argc, char **argv) {
    std::filesystem::path path;
    if (argc > 1) {
        path = argv[1];

        // Warn about extra arguments
        if (argc > 2) {
            std::cout << "Unused arguments: ";
            for (int i = 2; i < argc; i++)
                std::cout << std::format("'{}' ", argv[i]);
        }
    } else {
        std::cout << "Usage: drone_flight_visualizer $1\n"
                  << "$1: Drone CSV data filepath" << std::endl;
        return 1;
    }

    // Initialize the flight data object
    dfv::DroneFlightData data{path};

    // GLFW initialization
    dfv::raii::Glfw glfw{"Drone Flight Visualizer"};
    dfv::GlfwSurface surface{glfw.window()};

    const dfv::VisualizerCreateInfo createInfo{.surface = surface,
                                               .flightData = data,
                                               .objectModelPath = "assets/models/model.obj",
                                               .objectScale = 0.04f};

    dfv::Visualizer visualizer{createInfo};
    visualizer.start();
    setupInput(glfw, visualizer);

    auto lastFrameStart = dfv::clock::now();

    while (!glfwWindowShouldClose(glfw.window())) {
        // Poll input events
        glfwPollEvents();

        auto frameStart = dfv::clock::now();
        auto deltaTime = frameStart - lastFrameStart;
        lastFrameStart = frameStart;

        visualizer.drawFrame(deltaTime);
    }

    const auto stats = visualizer.getStats();
    const auto updateTimeAvg = duration_cast<dfv::milliseconds_f>(stats.updateTotalTime / stats.frameCount);
    const auto drawTimeAvg = duration_cast<dfv::milliseconds_f>(stats.drawTotalTime / stats.frameCount);
    const auto frameTimeAvg = duration_cast<dfv::milliseconds_f>((stats.updateTotalTime + stats.drawTotalTime) / stats.frameCount);
    const auto fpsAvg = 1000.0f / frameTimeAvg.count();

    std::cout << "Average update time: " << updateTimeAvg << std::endl;
    std::cout << "Average draw time: " << drawTimeAvg << std::endl;
    std::cout << "Average frame time: " << frameTimeAvg << std::endl;
    std::cout << "Average FPS: " << fpsAvg << std::endl;

    return 0;
}

void setupInput(dfv::raii::Glfw &glfw, dfv::Visualizer &visualizer) {
    static dfv::Visualizer &visualizerRef = visualizer;
    static dfv::raii::Glfw &glfwRef = glfw;

    static dfv::CameraMovement movement{};

    glfwSetKeyCallback(glfw.window(), [](GLFWwindow *window, const int key, int /*scancode*/, const int action, int mods) {
        if (action == GLFW_REPEAT) {
            return;
        }

        // clang-format off
        #define CASE_INPUT_AXIS(axis, keyPositive, keyNegative)     \
            case keyPositive:                                       \
                movement.axis += action == GLFW_PRESS ? 1.f : -1.f; \
                break;                                              \
            case keyNegative:                                       \
                movement.axis -= action == GLFW_PRESS ? 1.f : -1.f; \
                break

        #define CASE_PRESS(key)             \
            case key:                       \
                if (action != GLFW_PRESS) { \
                    break;                  \
                } else

        #define CASE_RELEASE(key)             \
            case key:                         \
                if (action != GLFW_RELEASE) { \
                    break;                    \
                } else
        // clang-format on

        switch (key) {
            CASE_INPUT_AXIS(surge, GLFW_KEY_W, GLFW_KEY_S);
            CASE_INPUT_AXIS(sway, GLFW_KEY_D, GLFW_KEY_A);
            CASE_INPUT_AXIS(heave, GLFW_KEY_SPACE, GLFW_KEY_LEFT_CONTROL);
            CASE_INPUT_AXIS(tilt, GLFW_KEY_UP, GLFW_KEY_DOWN);
            CASE_INPUT_AXIS(pan, GLFW_KEY_LEFT, GLFW_KEY_RIGHT);
            CASE_PRESS(GLFW_KEY_ESCAPE) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;
            }
            CASE_PRESS(GLFW_KEY_1) {
                visualizerRef.setCameraMode(dfv::CameraMode::Free);
                break;
            }
            CASE_PRESS(GLFW_KEY_2) {
                visualizerRef.setCameraMode(dfv::CameraMode::LockedOn);
                break;
            }
            CASE_PRESS(GLFW_KEY_3) {
                visualizerRef.setCameraMode(dfv::CameraMode::Follow1stPerson);
                break;
            }
            CASE_PRESS(GLFW_KEY_4) {
                visualizerRef.setCameraMode(dfv::CameraMode::Follow3rdPerson);
                break;
            }
            CASE_PRESS(GLFW_KEY_R) {
                visualizerRef.recenterCamera();
                break;
            }
            CASE_PRESS(GLFW_KEY_J) {
                visualizerRef.addToTimeMultiplier(-1);
                break;
            }
            CASE_PRESS(GLFW_KEY_K) {
                visualizerRef.changeTimeMultiplier(0);
                break;
            }
            CASE_PRESS(GLFW_KEY_L) {
                visualizerRef.addToTimeMultiplier(1);
                break;
            }
            CASE_PRESS(GLFW_KEY_ENTER) {
                if (mods == GLFW_MOD_ALT)
                    glfwRef.toggleFullscreen();
                break;
            }
            default:
                break;
        }

        visualizerRef.setCameraMovement(movement);
    });

    static double lastCursorX, lastCursorY;

    static auto cursorPosCallback = [](GLFWwindow * /*window*/, const double xpos, const double ypos) {
        double xoffset = lastCursorX - xpos; // Reversed since Vulkan used left-handed coordinates
        double yoffset = lastCursorY - ypos; // Reversed since y-coordinates range from bottom to top
        lastCursorX = xpos;
        lastCursorY = ypos;

        const float sensitivity = 0.0005f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        visualizerRef.turnCamera({xoffset, yoffset, 0.f});
    };

    glfwSetInputMode(glfw.window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // We set the callback once to get the initial cursor position, to avoid a camera jump when the cursor first enters the window
    // We then set the callback to the actual callback
    glfwSetCursorPosCallback(glfw.window(), [](GLFWwindow *window, const double xpos, const double ypos) {
        lastCursorX = xpos;
        lastCursorY = ypos;
        glfwSetCursorPosCallback(window, cursorPosCallback);
    });
}
