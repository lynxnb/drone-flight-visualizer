# Build the executable
add_executable(drone_flight_visualizer
        main.cpp
        vulkan/vk_initializers.cpp
        vulkan/vk_engine.cpp
        utils/data_reader.cpp
        objects/world_object.h
        objects/drone_object.h
)
target_include_directories(drone_flight_visualizer PRIVATE ".")

# Link the executable to the libraries
target_link_libraries(drone_flight_visualizer PRIVATE
        cpr::cpr
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
        glfw
        glm
)

add_dependencies(drone_flight_visualizer Shaders)
