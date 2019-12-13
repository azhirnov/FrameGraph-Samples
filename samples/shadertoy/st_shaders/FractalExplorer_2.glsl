// from https://www.shadertoy.com/view/MdyGRW

// Fractal Explorer DOF. January 2016
// by David Hoskins
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// https://www.shadertoy.com/view/4s3GW2


//-------------------------------------------------------------------------------------------

const float GA =2.399; 
const mat2 rot = mat2(cos(GA),sin(GA),-sin(GA),cos(GA));

vec3 dof(vec2 uv,float rad, float zed)
{
	vec3 acc=vec3(0);
    vec2 pixel=vec2(iResolution.y/iResolution.x,1.)*0.003, angle=vec2(0,rad);;
    vec3 central = texture(iChannel0, uv, -99.0).xyz;
    rad=1.;
	for (int j=0;j<80;j++)
    {  
        rad += 1./rad;
	    angle*=rot;
        vec4 col=texture(iChannel0,uv+pixel*(rad-1.)*angle);
        acc+= (col.w >= zed) ? col.xyz: central;

    }
	return acc/80.;
}

//-------------------------------------------------------------------------------------------
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
    uv.y = 1.0 - uv.y;

    float zed = texture(iChannel0, uv).w;
	float radius = abs(zed-1.)*.1;
	fragColor = vec4(dof(uv, radius, zed-2.), 1.0);
}