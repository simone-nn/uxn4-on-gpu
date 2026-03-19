#version 450
#extension GL_EXT_shader_explicit_arithmetic_types : require

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(std430, set = 0, binding = 7) readonly buffer Pixel_Buffer {
    uint palette[16];
    uint width;
    uint height;
    uint8_t layers[];
} pixels;

void main() {
    uint w = pixels.width;
    uint h = pixels.height;
    uint x = uint(fragUV.x * w);
    uint y = uint(fragUV.y * h);
    uint idx = y * w + x;

    uint8_t bg = pixels.layers[idx];
    uint8_t fg = pixels.layers[w * h + idx];

    uint rgba = pixels.palette[uint(fg) << 2 | uint(bg)];

    outColor = vec4(
        float((rgba >> 16) & 0xFFu) / 255.0,
        float((rgba >>  8) & 0xFFu) / 255.0,
        float((rgba      ) & 0xFFu) / 255.0,
        float((rgba >> 24) & 0xFFu) / 255.0
    );
}