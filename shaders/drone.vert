#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;

layout (location = 0) out vec3 outPos;
layout (location = 1) out vec3 outNorm;

layout (push_constant) uniform constants {
    vec4 modelTransform;
    mat4 worldTransform;
} pushConstants;

void main() {
    gl_Position = pushConstants.worldTransform * vec4(vPosition, 1.0f);
    outPos = (pushConstants.modelTransform * vec4(vPosition, 1.0f)).xyz;
    outNorm = vNormal;
}
