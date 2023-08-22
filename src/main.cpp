#include <iostream>
#include "vulkan/context.h"
#include "utils/fetcher.h"


int main() {
    std::cout << "drone_flight_visualizer" << std::endl;

    try {
        dfv::vulkan::VulkanContext vkContext{};
        dfv::utils::Fetcher fetcher;
        point point1{40.0, 40.0};
        point point2{41.0, 41.0};
        std::vector<point> points{point1, point2};
        auto points_w_elevation = fetcher.FetchElevation(points);
        while (!glfwWindowShouldClose(vkContext.window)) {
            glfwPollEvents();
        }

    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
