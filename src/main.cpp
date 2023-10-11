#include <chrono>
#include <iostream>

#include "utils/data_reader.h"
#include "vulkan/vk_engine.h"

using namespace dfv;

namespace {
    using clock = std::chrono::steady_clock;
    using time_point = std::chrono::time_point<clock>;
} // namespace

/*
 * Command line usage: drone_flight_visualizer $1
 * $1: Drone CSV data
 */
int main(int argc, char **argv) {
    std::cout << "drone_flight_visualizer" << std::endl;

    VulkanEngine engine;
    try {
        engine.init();
        engine.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    engine.cleanup();
    return 0;
}
