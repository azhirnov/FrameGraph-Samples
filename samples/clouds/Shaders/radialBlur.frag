
layout(binding = 0) uniform sampler2D texColor;

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

// Uniform buffers - need camera and sun parameters

layout(binding = 1) uniform UniformCameraObject {

    mat4 view;
    mat4 proj;
    vec3 cameraPosition;
    vec3 cameraParams;
} camera;

// all of these components are calculated in SkyManager.h/.cpp
layout(binding = 2) uniform UniformSunObject {
    
    vec4 location;
    vec4 direction;
    vec4 color;
    mat4 directionBasis;
    float intensity;
} sun;

// References:
// https://forum.unity.com/threads/radial-blur.31970/
// https://stackoverflow.com/questions/4579020/how-do-i-use-a-glsl-shader-to-apply-a-radial-blur-to-an-entire-scene

#define NUM_SAMPLES 10

void main() {
    vec2 scrPt = fragUV * 2.0 - 1.0;

    const vec4 currentFragment = texture(texColor, fragUV);

    if(sun.direction.y < 0.0) {
        outColor = vec4(currentFragment.xyz, 1.0); return;
    }

    const float samples[NUM_SAMPLES] = { -0.08, -0.05, -0.03, -0.02, -0.01, 0.01, 0.02, 0.03, 0.05, 0.08 };

    // Let's just take some samples towards the sun position
    vec4 sunPos = camera.proj * camera.view * sun.location;
    sunPos /= sunPos.w;

    vec2 lightVec = sunPos.xy - scrPt;
    float dist = length(lightVec);
    lightVec /= dist;
    float accumSampleAmt = 0.0;

    // Sample the image along the light vector
    for(int i = 0; i < NUM_SAMPLES; ++i) {
        accumSampleAmt += texture(texColor, (scrPt + samples[i] * lightVec * 1.5 * dist) * 0.5 + 0.5).a * 1.1;
    }
    accumSampleAmt /= float(NUM_SAMPLES);

    outColor = vec4(sun.color.xyz * sun.intensity * accumSampleAmt + 0.5 * currentFragment.xyz, 1.0);
}
