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
    vec3 sunlightDirection; //w for sun power
    vec4 sunlightColor;
    vec3 eyePos; // position of the camera
} sceneData;

uint lowbias32(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

void main() {
    // Determine the index of the color based on gl_VertexID
    int colorIndex = id %10;

    // Define an array of 10 colors
    vec3 colors[10];
    colors[0] = vec3(1.0, 0.0, 0.0);  // Red
    colors[1] = vec3(0.0, 1.0, 0.0);  // Green
    colors[2] = vec3(0.0, 0.0, 1.0);  // Blue
    colors[3] = vec3(1.0, 1.0, 0.0);  // Yellow
    colors[4] = vec3(1.0, 0.0, 1.0);  // Magenta
    colors[5] = vec3(0.0, 1.0, 1.0);  // Cyan
    colors[6] = vec3(1.0, 0.5, 0.0);  // Orange
    colors[7] = vec3(0.5, 0.0, 1.0);  // Purple
    colors[8] = vec3(0.5, 0.5, 0.5);  // Gray
    colors[9] = vec3(0.0, 0.5, 0.5);  // Teal

    // Select the color based on the colorIndex
    vec3 droneColor = colors[colorIndex];

    vec3 Ambient = droneColor;
    outFragColor = vec4(Ambient, 1.0f);
}
