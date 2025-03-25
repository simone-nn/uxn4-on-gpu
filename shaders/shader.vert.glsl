#version 450
layout(location = 6) in vec4 vertex;

layout(location = 0) out vec2 fragUV;

void main() {
    fragUV = vertex.zw;
    gl_Position = vec4(vertex.xy, 0.0, 1.0);
}
