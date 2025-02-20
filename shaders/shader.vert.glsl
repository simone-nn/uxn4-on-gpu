#version 450
layout(location = 0) in vec4 inPosition;

layout(location = 0) out vec2 fragUV;

void main() {
    fragUV = inPosition.zw;
    gl_Position = vec4(inPosition.xy, 0.0, 1.0);
}