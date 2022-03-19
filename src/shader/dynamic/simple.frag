#version 450

layout(location = 0) in vec3 v2fColor;
layout(location = 1) in vec2 v2fTexCoord;

layout(binding = 2) uniform sampler2D tTex;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(tTex, v2fTexCoord);
}
