// from https://www.shadertoy.com/view/ldByDh

// Created by inigo quilez - iq/2008/2018
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.


// This is "Organix", a procedural graphics I made in 2008, which was my 2nd ever
// raymarched image (I was still investigating and learning the technique).
//
// I just copy pasted the code which was C++ at the time, and it just worked -
// I only tweaked minor things to make it work faster in WebGL. But I kept the
// variable names and all the original ugliness from 2008
//
// Link to the original piece: https://iquilezles.org/prods/index.htm#organix


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord / iResolution.xy;
    vec3 src = texture(iChannel0,uv).xyz;

    float fra = clamp( 4.0*abs(uv.x-0.5)-1.1, 0.0, 1.0 );

    vec3 col = vec3(0.0);
    for( int m=-4; m<4; m++ )
    for( int n=-4; n<4; n++ )
    {
        col += texture(iChannel0, uv + vec2(m,n)/800.0).xyz;
    }
    col /= 81.0;
    
    col = mix( src, col, 0.25+0.75*fra );
    
    // vigneting
	//col *= 0.75 + uv.x*(1.0-uv.x);

	fragColor = vec4(col,1.0);
}