#version 450

layout (location = 0) in vec3 inColor;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 0) uniform SceneData {
    vec4 ambientColor;// w is for exponent
    vec3 sunlightDirection;//x for min, y for max, zw unused.
    vec4 sunlightColor;
    vec3 eyePos;//w for sun power
} sceneData;

void main() {
    outFragColor = vec4(inColor + sceneData.ambientColor.xyz, 1.0f);
}
