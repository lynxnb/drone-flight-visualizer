#pragma once

#include "vulkan/vk_engine.h"

namespace dfv {

    /**
     * The global context of the application.
     */
    struct Context {
        VulkanEngine engine;

        Context()
            : engine() {}

        /**
         * Sets the exit flag for the entire application.
         */
        void setExit() {
            exitApplication.test_and_set(std::memory_order_release);
            exitApplication.notify_all();
        }

        /**
         * Returns whether the application should exit.
         */
        bool shouldExit() {
            return exitApplication.test(std::memory_order_acquire);
        }

        /**
         * Waits until the application should exit.
         */
        void waitExit() {
            exitApplication.wait(false, std::memory_order_acquire);
        }

      private:
        std::atomic_flag exitApplication; //!< A boolean flag to indicates whether the application should exit.
    };

} // namespace dfv
