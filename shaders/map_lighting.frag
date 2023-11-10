// glsl version 4.5
#version 450

// shader input
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 3) in vec2 fragUV;

// output write
layout (location = 0) out vec4 outFragColor;


layout (set = 0, binding = 1) uniform SceneData {
    vec4 ambientColor;
    vec3 sunlightDirection; //w for sun power
    vec4 sunlightColor;
    vec3 eyePos; // position of the camera
} sceneData;

layout(binding = 2) uniform sampler2D tex;


vec3 BRDF(vec3 V, vec3 N, vec3 L, vec3 Md, vec3 Ms, float gamma) {
    // vec3 V  - direction of the viewer
    // vec3 N  - normal vector to the surface
    // vec3 L  - light vector (from the light model)
    // vec3 Md - main color of the surface
    // vec3 Ms - specular color of the surface
    // float gamma - Exponent for power specular term
    vec3 diffuse = Md * max(dot(L, N),0.0);
    //vec3 r = 2*(N * dot(L, N)) - L;
    vec3 r = -reflect(L,N); // glsl has a built in function for this

    vec3 specular = Ms * pow(clamp(dot(V,r),0,1.0f), gamma);
    return diffuse + specular;
}

void main() {
    // phong shading implementation
    vec3 Norm = normalize(fragNorm);
    vec3 EyeDir = normalize(sceneData.eyePos - fragPos);

    vec3 lightDir = sceneData.sunlightDirection;
    vec3 lightColor = sceneData.sunlightColor.rgb;

    vec3 DiffSpec = BRDF(EyeDir, Norm, lightDir, texture(tex,fragUV).rgb, lightColor, 100.0f);

    vec3 Ambient = texture(tex, fragUV).rgb * 0.05f;

    outFragColor = vec4(clamp(0.95 * (DiffSpec)*lightColor.rgb + Ambient, 0.0, 1.0), 1.0f);
}
