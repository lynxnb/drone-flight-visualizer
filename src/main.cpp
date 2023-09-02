#include <iostream>
#include <chrono>
#include "vulkan/context.h"
#include "vulkan/renderer.h"
#include "utils/data_reader.h"

using namespace dfv;

namespace {
    using clock = std::chrono::steady_clock;
    using time_point = std::chrono::time_point<clock>;
}

/*
 * Command line usage: drone_flight_visualizer $1
 * $1: Drone CSV data
 */
int main(int argc, char **argv) {
    std::cout << "drone_flight_visualizer" << std::endl;

    if (argc < 2)
        std::cerr << "Source drone data VulkanCSV Context vkContext{}; path not supplied" << std::endl;

    try {
        vulkan::VulkanContext vkContext{};
        vulkan::Renderer renderer{vkContext};
        time_point start{clock::now()};

//        auto data = utils::fetchOsmData("46.09680990990991, 9.719069961104724, 46.276990090090095, 9.979330038895275");
//        utils::populateElevation(data.nodes);
//        auto flightData = utils::readFlightData(R"(C:\Users\milob\Documents\dev\drone-flight-visualizer\data\DJIFlightRecord_2023-06-10_11-54-232.csv)");

        while (!glfwWindowShouldClose(vkContext.window)) {
            glfwPollEvents();

            vkContext.drawFrame();
        }

    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
