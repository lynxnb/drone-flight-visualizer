# Build the executable
add_executable(drone_flight_visualizer
        main.cpp
        utils/data_reader.cpp
        vulkan/vk_engine.cpp
        vulkan/vk_initializers.cpp
        vulkan/vk_pipeline.cpp
        vulkan/deletion_queue.cpp
        vulkan/vk_mesh.cpp
)
target_include_directories(drone_flight_visualizer PRIVATE ".")

# Link the executable to the libraries
target_link_libraries(drone_flight_visualizer PRIVATE
        cpr::cpr
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
        glfw
        glm
        VulkanMemoryAllocator
        tinyobjloader
)

add_dependencies(drone_flight_visualizer Shaders)
