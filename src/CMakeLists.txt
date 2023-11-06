set(DFV_SOURCE_COMMON
        context.cpp
        renderer/renderer.cpp
        map/data_reader.cpp
        vulkan/vk_engine.cpp
        vulkan/vk_initializers.cpp
        vulkan/vk_pipeline.cpp
        vulkan/deletion_queue.cpp
        vulkan/vk_mesh.cpp
        vulkan/vk_engine_utils.cpp
        vulkan/vk_engine_init.cpp
        vulkan/render_object.cpp
        input/input_handler.cpp
        map/data_fetcher.cpp
        objects/drone.cpp
        objects/drone.h
)

# Build the executable
add_executable(drone_flight_visualizer
        ${DFV_SOURCE_COMMON}
        main.cpp
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


add_executable(engine_test
        ${DFV_SOURCE_COMMON}
        engine_test.cpp
)

target_include_directories(engine_test PRIVATE ".")

target_link_libraries(engine_test PRIVATE
        cpr::cpr
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
        glfw
        glm
        tinyobjloader
)

add_dependencies(drone_flight_visualizer Shaders)
add_dependencies(drone_flight_visualizer Assets)
