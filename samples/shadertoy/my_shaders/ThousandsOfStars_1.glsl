// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
// from https://www.shadertoy.com/view/4ldSz4

vec3 Blur (vec2 uv)
{
    const int radius = 1;
    
    vec3 color = vec3(0.0);
    float kernel = 0.0;
    vec2 uvScale = 1.75 / iChannelResolution[0].xy;
    
    for (int y = -radius; y <= radius; ++y)
    {
        for (int x = -radius; x <= radius; ++x)
        {
            float k = 1.0 / pow( 2.0, abs(float(x)) + abs(float(y)) );
            
            color += texture( iChannel0, uv + vec2(x,y) * uvScale ).rgb * k;
            kernel += k;
        }
    }
    
    return color / kernel;
}

void mainImage (out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    
    //fragColor = texture( iChannel0, uv );
    fragColor = vec4( Blur( uv ), 1.0 );
}
