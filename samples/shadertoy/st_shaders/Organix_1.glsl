// from https://www.shadertoy.com/view/ldByDh

// Created by inigo quilez - iq/2008/2018
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord / iResolution.xy;
    vec3 src = texture(iChannel0,uv).xyz;
    
    const int SunNumS = 100;
    const float SunINumS = (1.0/float(SunNumS));
    
    vec2  di = (vec2(0.95,0.72) - uv) * SunINumS;
    float ef = exp2( -3.0*length(di) );
    float e = 0.0351857 * SunINumS;

    vec3 col = vec3(0.0);
    for( int k=0; k<SunNumS; k++ )
    {
        vec3 kk = texture(iChannel0,uv).xyz;
        float gg = kk.x+kk.y+kk.z;
        gg = gg*gg;
        gg = gg*gg;
        col += kk*e*gg;//*0.3;
        e *= ef;
        uv += di;
    }
    
    col = col*col*(1.0-src) + src;

	fragColor = vec4(col,1.0);
}