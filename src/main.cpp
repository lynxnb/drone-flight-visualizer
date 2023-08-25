#include <iostream>
#include "vulkan/context.h"
#include "utils/dataReader.h"
#include "objects/objects.h"

using namespace dfv;

int main() {
    std::cout << "drone_flight_visualizer" << std::endl;

    try {
        dfv::vulkan::VulkanContext vkContext{};
        auto data = utils::FetchOSMData("46.09680990990991, 9.719069961104724, 46.276990090090095, 9.979330038895275");
        utils::PopulateElevation(data.nodes);

        while (!glfwWindowShouldClose(vkContext.window)) {
            glfwPollEvents();
        }

    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
