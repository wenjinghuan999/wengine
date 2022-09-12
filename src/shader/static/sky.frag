#version 450

layout(binding = 0) uniform CameraUniform {
    mat4 view;
    mat4 proj;
    vec3 position;
    vec2 fov;
} uCamera;

layout(location = 0) in vec2 v2fScreenCoord;

layout(binding = 2) uniform samplerCube tTex;

layout(location = 0) out vec4 outColor;

vec3 CubeMapSwizzle(vec3 direction) {
    return vec3(-direction.y, direction.z, direction.x);
}

void main() {
    vec2 screen_vec = vec2(v2fScreenCoord * 2.0 - 1.0) * vec2(tan(uCamera.fov.x / 2.0), tan(uCamera.fov.y / 2.0));
    mat4 view_t = transpose(uCamera.view);
    vec3 right = view_t[0].xyz;
    vec3 up = view_t[1].xyz;
    vec3 forward = -view_t[2].xyz;
    vec3 direction = normalize(forward + screen_vec.x * right - screen_vec.y * up);
    outColor = texture(tTex, CubeMapSwizzle(direction));
}