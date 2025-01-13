#version 450

//vec2 positions[3] = vec2[](
//    vec2(0.0, -0.5),
//    vec2(0.5, 0.5),
//    vec2(-0.5, 0.5)
//);
//vec3 colors[3] = vec3[](
//    vec3(1.0, 0.0, 0.0),
//    vec3(0.0, 1.0, 0.0),
//    vec3(0.0, 0.0, 1.0)
//);
//struct Vertex {
//    vec2 position;
//};
//layout(std140, binding = 0) buffer VertexSSBO {
//    Vertex vertex[ ];
//} ssbo;
//layout(location = 0) in uint inIndex;

layout(location = 0) in vec2 inPosition;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition.xy, 0.0, 1.0);
//    fragColor = colors[gl_VertexIndex];
//    gl_Position = vec4(ssbo.vertex[inIndex].position, 0.0, 0.0);
    fragColor = vec3(0.0, 1.0, 0.0);
}
