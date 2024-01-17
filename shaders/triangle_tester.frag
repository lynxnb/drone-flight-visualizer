//glsl version 4.5
#version 450

//shader input
layout(location = 0) in vec3 fragNorm;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in flat int id;

//output write
layout (location = 0) out vec4 outFragColor;


layout (set = 0, binding = 0) uniform SceneData {
    vec4 ambientColor;
    vec3 sunlightDirection;//w for sun power
    vec4 sunlightColor;
    vec3 eyePos;// position of the camera
} sceneData;

void main() {
    // Determine the index of the color based on gl_VertexID
    int colorIndex = id % 10;

    // Color lookup table of 10 colors
    // @formatter:off
    vec3 colors_lut[10] = {
        vec3(1.0, 0.0, 0.0), // Red
        vec3(0.0, 1.0, 0.0), // Green
        vec3(0.0, 0.0, 1.0), // Blue
        vec3(1.0, 1.0, 0.0), // Yellow
        vec3(1.0, 0.0, 1.0), // Magenta
        vec3(0.0, 1.0, 1.0), // Cyan
        vec3(1.0, 0.5, 0.0), // Orange
        vec3(0.5, 0.0, 1.0), // Purple
        vec3(0.5, 0.5, 0.5), // Gray
        vec3(0.0, 0.5, 0.5), // Teal
    };
    // @formatter:on

    // Select the color based on the colorIndex
    vec3 droneColor = colors_lut[colorIndex];
    outFragColor = vec4(droneColor, 1.0f);
}
