#include <iostream>
#include "vulkan/context.h"
#include "utils/fetcher.h"
#include "objects/objects.h"

using namespace dfv;

int main() {
    std::cout << "drone_flight_visualizer" << std::endl;

    try {
        dfv::vulkan::VulkanContext vkContext{};
        objects::point point1{40.0, 40.0};
        objects::point point2{41.0, 41.0};
        std::vector<objects::point> points{point1, point2};
        auto pointsWithElevation = utils::FetchElevation(points);
        while (!glfwWindowShouldClose(vkContext.window)) {
            glfwPollEvents();
        }

    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
