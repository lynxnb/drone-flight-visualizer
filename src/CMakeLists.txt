set(DFV_SOURCE_COMMON
        visualizer.cpp
        vulkan/vk_engine.cpp
        vulkan/vk_initializers.cpp
        vulkan/vk_pipeline.cpp
        vulkan/deletion_queue.cpp
        vulkan/vk_mesh.cpp
        vulkan/vk_engine_utils.cpp
        vulkan/vk_engine_init.cpp
)


# drone_flight_visualizer executable
add_executable(drone_flight_visualizer
        ${DFV_SOURCE_COMMON}
        glfw/glfw_surface.cpp
        flight_data/drone_flight_data.cpp
        map/data_fetcher.cpp
        drone_entrypoint.cpp
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


# mock visualizer
add_executable(mock_visualizer
        ${DFV_SOURCE_COMMON}
        glfw/glfw_surface.cpp
        flight_data/mock_flight_data.cpp
        mock_entrypoint.cpp
)
target_include_directories(mock_visualizer PRIVATE ".")

target_link_libraries(mock_visualizer PRIVATE
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
        glfw
        glm
        tinyobjloader
)

add_dependencies(mock_visualizer Shaders)
add_dependencies(mock_visualizer Assets)
