#include "context.h"

#include <config/config.h>

namespace dfv {

    Context::Context()
        : engine(), inputHandler(), exitApplication() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        using namespace window_config;
        glfwWindow = glfwCreateWindow((int) WindowWidth, (int) WindowHeight, WindowTitle, nullptr, nullptr);

        // Set this context to the GLFW window so that it can be accessed from the callbacks
        setContext(glfwWindow);

        // Set the exit flag when the window is closed
        glfwSetWindowCloseCallback(glfwWindow, [](GLFWwindow *window) {
            auto context = getContext(window);
            context->setExit();
        });

        setInputCallbacks();
    }

    Context::~Context() {
        glfwDestroyWindow(glfwWindow);
        glfwTerminate();
    }

    void Context::setInputCallbacks() const {
        // Send key events to the input handler
        glfwSetKeyCallback(glfwWindow, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
            // Ignore key repeat events
            if (action == GLFW_REPEAT)
                return;

            auto context = getContext(window);
            context->inputHandler.handleKey(key, scancode, action, mods);
        });
    }

    void Context::setExit() {
        exitApplication.test_and_set(std::memory_order_release);
        exitApplication.notify_all();
    }

    bool Context::shouldExit() {
        return exitApplication.test(std::memory_order_acquire);
    }

    void Context::waitExit() {
        exitApplication.wait(false, std::memory_order_acquire);
    }

    void Context::setContext(GLFWwindow *window) {
        glfwSetWindowUserPointer(window, this);
    }

    Context *Context::getContext(GLFWwindow *window) {
        return reinterpret_cast<Context *>(glfwGetWindowUserPointer(window));
    }

} // namespace dfv
