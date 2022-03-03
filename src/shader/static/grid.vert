#version 450

layout(binding = 0) uniform CameraUniform {
    mat4 view;
    mat4 proj;
} uCamera;
layout(binding = 1) uniform ModelUniform {
    mat4 model;
} uModel;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 v2fTexCoord;

void main() {
    gl_Position = uCamera.proj * uCamera.view * uModel.model * vec4(inPosition, 1.0);
    v2fTexCoord = inTexCoord;
}