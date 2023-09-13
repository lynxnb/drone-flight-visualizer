# Find the glslangValidator program
find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

# Find all the shader files in this folder
file(GLOB_RECURSE GLSL_SOURCE_FILES
        "*.frag"
        "*.vert"
        "*.comp"
)

# Iterate each shader
foreach (GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${OUTPUT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
    message(STATUS "Registering shader ${GLSL}")
    # Execute glslang command to compile the shader
    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${GLSL_VALIDATOR} --quiet -V ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL}
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach (GLSL)

add_custom_target(
        Shaders
        DEPENDS ${SPIRV_BINARY_FILES}
)
