#version 450

layout(location = 0) in vec2 v2fTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    float v1 = float((int(v2fTexCoord.x * 10.0) ^ int(v2fTexCoord.y * 10.0)) & 1);
    float v2 = float((int(v2fTexCoord.x * 50.0) ^ int(v2fTexCoord.y * 50.0)) & 1);
    float v = v1 * 0.15 + v2 * 0.05 + 0.4;
    outColor = vec4(v, v, v, 1.0);
}