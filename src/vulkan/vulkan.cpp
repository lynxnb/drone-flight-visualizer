#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

bool init_vulkan() {
    VkApplicationInfo applicationInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "drone_flight_visualizer",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_1,
    };

    VkInstanceCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = 0,
            .enabledExtensionCount = 0,
    };

    VkInstance vkInstance;
    vkCreateInstance(&createInfo, nullptr, &vkInstance);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(640, 480, "Window Title", nullptr, nullptr);
    VkSurfaceKHR surface;
    glfwCreateWindowSurface(vkInstance, window, nullptr, &surface);

    return true;
}
