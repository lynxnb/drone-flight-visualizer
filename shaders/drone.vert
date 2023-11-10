#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;



layout (location = 0) out vec3 outPos;
layout (location = 1) out vec3 outNorm;



layout (set = 0, binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
} cameraData;

struct ObjectData{
    mat4 model;
};

layout (std140, set = 1, binding = 0) readonly buffer ObjectBuffer {
    ObjectData objects[];
} objectBuffer;

// Push constants block
layout (push_constant) uniform constants {
    vec4 data;
    mat4 renderMatrix;
} PushConstants;

void main() {
    // Index into the object buffer using vkCmdDraw `firstInstance` parameter
    mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model;
    mat4 transformMatrix = (cameraData.viewproj * modelMatrix);
    gl_Position = transformMatrix * vec4(vPosition, 1.0f);
    outPos = (modelMatrix * vec4(vPosition, 1.0f)).xyz;
    outNorm = vNormal;
}