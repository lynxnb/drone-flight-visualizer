set(DFV_SOURCE_COMMON
        context.cpp
        renderer/renderer.cpp
        vulkan/vk_engine.cpp
        vulkan/vk_initializers.cpp
        vulkan/vk_pipeline.cpp
        vulkan/deletion_queue.cpp
        vulkan/vk_mesh.cpp
        vulkan/vk_engine_utils.cpp
        vulkan/vk_engine_init.cpp
        input/input_handler.cpp
)


# drone_flight_visualizer executable
add_executable(drone_flight_visualizer
        ${DFV_SOURCE_COMMON}
        main.cpp
        glfw/glfw_surface.cpp
        map/data_reader.cpp
        map/data_fetcher.cpp
        objects/drone.cpp
)

target_include_directories(drone_flight_visualizer PRIVATE ".")

# Link the executable to the libraries
target_link_libraries(drone_flight_visualizer PRIVATE
        cpr::cpr
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
        glfw
        glm
        tinyobjloader
)

add_dependencies(drone_flight_visualizer Shaders)
add_dependencies(drone_flight_visualizer Assets)
