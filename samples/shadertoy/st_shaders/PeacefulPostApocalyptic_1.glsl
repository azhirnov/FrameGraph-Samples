// from https://www.shadertoy.com/view/ltSyzd

void mainImage (out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord.xy/iResolution.xy;
    uv.y = 1.0 - uv.y;

    vec4 prev = texture(iChannel1, uv);
    vec4 new = texture(iChannel0, uv);
    
	fragColor = mix(prev, new, 0.2);
}

void mainVR (out vec4 fragColor, in vec2 fragCoord, in vec3 fragRayOri, in vec3 fragRayDir)
{
    mainImage( fragColor, fragCoord );
}
