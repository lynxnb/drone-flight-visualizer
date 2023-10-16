#pragma once

#include "input/input_handler.h"
#include "vulkan/vk_engine.h"

namespace dfv {

    /**
     * A RAII class holding the global state of the application.
     */
    struct Context {
        GLFWwindow *glfwWindow = nullptr;
        VulkanEngine engine;
        InputHandler inputHandler;

        /**
         * Initializes the context and creates a GLFW window.
         */
        Context();

        /**
         * Cleans up the context and destroys the GLFW window.
         */
        ~Context();

        /**
         * Sets the exit flag for the entire application.
         */
        void setExit();

        /**
         * Returns whether the application should exit.
         */
        bool shouldExit();

        /**
         * Waits until the application should exit.
         */
        void waitExit();

      private:
        std::atomic_flag exitApplication; //!< A boolean flag to indicates whether the application should exit.

        /**
         * Sets the input callbacks for the GLFW window.
         */
        void setInputCallbacks() const;

        /**
         * Sets this instance of the context to the GLFW window.
         * @param window The GLFW window to set this context to.
         */
        void setContext(GLFWwindow *window);

        /**
         * Retrieve the context associated with the GLFW window.
         * @param window The GLFW window to get the context from.
         * @return The context pointer.
         */
        static Context *getContext(GLFWwindow *window);
    };

} // namespace dfv
