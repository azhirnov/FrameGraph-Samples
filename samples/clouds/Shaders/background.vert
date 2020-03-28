
out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;

void main() {   
	gl_Position = vec4(inPosition.xyz, 1.0);
    fragUV = inUV;
	fragColor = inColor;
}