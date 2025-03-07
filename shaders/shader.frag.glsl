#version 450
layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 4) uniform sampler2D background;
layout(set = 0, binding = 5) uniform sampler2D foreground;

void main() {
    vec4 foregroundColor = texture(foreground, fragUV);
    if (foregroundColor.z != 0.0) {
        outColor = foregroundColor;
    } else {
        outColor = texture(background, fragUV);
    }
}
