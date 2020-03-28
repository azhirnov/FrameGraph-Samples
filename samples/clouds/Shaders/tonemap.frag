
layout(binding = 0) uniform sampler2D texColor;

layout(binding = 1) uniform UniformCameraObject {

    mat4 view;
    mat4 proj;
    vec3 cameraPosition;
    vec3 cameraParams;
} camera;

layout(binding = 2) uniform UniformSunObject {
    
    vec4 location;
    vec4 direction;
    vec4 color;
    mat4 directionBasis;
    float intensity;
} sun;

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

// Uncharted 2 Tonemapping made by John Hable, filmicworlds.com
vec3 uc2Tonemap(vec3 x)
{
   return ((x*(0.15*x+0.1*0.5)+0.2*0.02)/(x*(0.15*x+0.5)+0.2*0.3))-0.02/0.3;
}

vec3 tonemap(vec3 x, float exposure, float invGamma, float whiteBalance) {
    vec3 white = vec3(whiteBalance);
    vec3 color = uc2Tonemap(exposure * x);
    vec3 whitemap = 1.0 / uc2Tonemap(white);
    color *= whitemap;
    return pow(color, vec3(invGamma));
}

void main() {
    vec3 col = texture(texColor, fragUV).xyz;

    float whitepoint = 50.2;
    col = tonemap(col, 0.7, 1.0 / 2.2, whitepoint);

    float vignette = dot(fragUV - 0.5, fragUV - 0.5);

    col = mix(col, vec3(0.1, 0.05, 0.13), vignette);
    outColor = vec4(col, 1.0);
}