#pragma once

#include "context.h"

namespace dfv::vulkan {
    class Renderer {
        public:
            explicit Renderer(const VulkanContext &context);

            void renderDrone(int a);

            // Disallow copy and assignment
            Renderer(const Renderer &) = delete;

            Renderer &operator=(const Renderer &) = delete;

            Renderer(Renderer &&) = delete;

            Renderer &operator=(Renderer &&) = delete;

        private:
            const VulkanContext &vkContext;
    };
}