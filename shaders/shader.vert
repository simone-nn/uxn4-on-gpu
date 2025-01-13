#version 450

layout(location = 0) in vec2 inPosition;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition.xy, 0.0, 1.0);
//    fragColor = colors[gl_VertexIndex];
//    gl_Position = vec4(ssbo.vertex[inIndex].position, 0.0, 0.0);
    fragColor = vec3(0.0, 1.0, 0.0);
}
