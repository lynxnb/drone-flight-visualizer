//glsl version 4.5
#version 450

//shader input
layout(location = 0) in vec3 fragNorm;
layout(location = 1) in vec3 fragPos;

//output write
layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 0) uniform SceneData {
    vec4 ambientColor;
    vec3 sunlightDirection; //w for sun power
    vec4 sunlightColor;
    vec3 eyePos; // position of the camera
} sceneData;

vec3 droneColor = vec3(0.3225f, 0.3322f, 0.3453f); // Pantone gray 10 C


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

    vec3 DiffSpec = BRDF(EyeDir, Norm, lightDir, droneColor, lightColor, 100.0f);

    vec3 Ambient = droneColor * 0.05f;

    outFragColor = vec4(clamp(0.95 * (DiffSpec)*lightColor.rgb + Ambient, 0.0, 1.0), 1.0f);
}
