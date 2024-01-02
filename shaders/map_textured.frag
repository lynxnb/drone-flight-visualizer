//glsl version 4.5
#version 450

//shader input
layout(location = 0) in vec3 fragNorm;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec2 fragUV;

layout(set = 2, binding = 0) uniform sampler2D tex1;

//output write
layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 0) uniform SceneData {
    vec4 ambientColor;
    vec3 sunlightDirection; //w for sun power
    vec4 sunlightColor;
    vec3 eyePos; // position of the camera
} sceneData;

// Map color: grass green #29a415
vec3 droneColor = vec3(0.16, 0.64, 0.08);

vec3 BRDF(vec3 V, vec3 N, vec3 L, vec3 Md, vec3 Ms, float gamma) {
    // vec3 V  - direction of the viewer
    // vec3 N  - normal vector to the surface
    // vec3 L  - light vector (from the light model)
    // vec3 Md - main color of the surface
    // vec3 Ms - specular color of the surface
    // float gamma - Exponent for power specular term

    // Oren-Nayar diffuse value
    float sigma = 1.2f;
    float omega_i = acos(dot(L, N));
    float omega_r = acos(dot(V, N));
    float alpha = max(omega_i, omega_r);
    float beta = min(omega_i, omega_r);
    float A = 1 - 0.5 * (sigma * sigma / (sigma * sigma + 0.33));
    float B = 0.45 * (sigma * sigma / (sigma * sigma + 0.09));

    vec3 v_i = normalize(L - dot(L, N) * N);
    vec3 v_r = normalize(V - dot(V, N) * N);
    float G = max(0, dot(v_i, v_r));
    vec3 Lo = Md *clamp(dot(L, N), 0, 1.0);
    vec3 diffuse = Lo*(A + B * G * sin(alpha) * tan(beta));

    //vec3 r = 2*(N * dot(L, N)) - L;
    vec3 r = -reflect(L,N); // glsl has a built in function for this

    float specularFactor = 0.1;  // the specular factor adjuts the reflectivity of the surface
    vec3 specular = specularFactor * Ms * pow(clamp(dot(V,r),0,1.0f), gamma);
    return diffuse + specular;
}


void main() {
    // phong shading implementation
    vec3 Norm = normalize(fragNorm);
    vec3 EyeDir = normalize(sceneData.eyePos - fragPos);

    vec3 lightDir = sceneData.sunlightDirection;
    vec3 lightColor = sceneData.sunlightColor.rgb;

    vec3 DiffSpec = BRDF(EyeDir, Norm, lightDir, texture(tex1, fragUV).xyz, lightColor, 100.0f);

    outFragColor = vec4(clamp(0.95 * (DiffSpec)*lightColor.rgb, 0.0, 1.0), 1.0f);
}
