
layout(set = 0, binding = 0) uniform sampler2D texColor;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 col = texture(texColor, fragUV);

    outColor = col;
}