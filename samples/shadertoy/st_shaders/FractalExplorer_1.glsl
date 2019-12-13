// from https://www.shadertoy.com/view/MdyGRW

// Fractal Explorer DOF. January 2016
// by David Hoskins
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// https://www.shadertoy.com/view/4s3GW2

//--------------------------------------------------------------------------
#define SUN_COLOUR vec3(1., .9, .85)
#define FOG_COLOUR vec3(.13, 0.13, 0.14)
#define MOD3 .1031
#define STORE_DE

vec2 fcoord;

vec2 camStore = vec2(4.0,  0.0);
vec2 rotationStore	= vec2(1.,  0.);
vec2 mouseStore = vec2(2.,  0.);
vec3 sunLight  = vec3(  0.4, 0.4,  0.3 );

// By TekF...
void BarrelDistortion( inout vec3 ray, float degree )
{
	ray.z /= degree;
	ray.z = ( ray.z*ray.z - dot(ray.xy,ray.xy) );
	ray.z = degree*sqrt(ray.z);
}

//----------------------------------------------------------------------------------------
// From https://www.shadertoy.com/view/4djSRW
float Hash(vec2 p)
{
	vec3 p3  = fract(vec3(p.xyx) * MOD3);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

mat3 RotationMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat3(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c);
}

//----------------------------------------------------------------------------------------
#define CSize vec3(1., 1.7, 1.)
vec3 Colour( vec3 p)
{
    p = p.xzy;
	float col	= 0.0;
    float r2	= dot(p,p);
	for( int i=0; i < 5;i++ )
	{
		vec3 p1= 2.0 * clamp(p, -CSize, CSize)-p;
		col += abs(p.x-p1.z);
		p = p1;
		r2 = dot(p,p);
        //float r2 = dot(p,p+sin(p.z*.3)); //Alternate fractal
		float k = max((2.)/(r2), .5);
		p *= k;
	}
    return texture(iChannel1, vec2(p.x, p.y+p.z)*.2).xyz+vec3(.4, .2, 0.2);
}

//--------------------------------------------------------------------------

float Map( vec3 p )
{
	p = p.xzy;
	float scale = 1.1;
	for( int i=0; i < 8;i++ )
	{
		p = 2.0*clamp(p, -CSize, CSize) - p;
		float r2 = dot(p,p);
        //float r2 = dot(p,p+sin(p.z*.3)); //Alternate fractal
		float k = max((2.)/(r2), .5);
		p     *= k;
		scale *= k;
	}
	float l = length(p.xy);
	float rxy = l - 1.0;
	float n = l * p.z;
	rxy = max(rxy, (n) / 8.);
	return (rxy) / abs(scale);
}

//--------------------------------------------------------------------------
float Shadow( in vec3 ro, in vec3 rd)
{
	float res = 1.0;
    float t = 0.05;
	float h;
	
    for (int i = 0; i < 12; i++)
	{
		h = Map( ro + rd*t );
		res = min(6.0*h / t, res);
		t += h+.02;
	}
    return max(res, 0.0);
}

//--------------------------------------------------------------------------
vec3 DoLighting(in vec3 mat, in vec3 pos, in vec3 normal, in vec3 eyeDir, in float d, in float sh)
{
    vec3 sunLight  = normalize( vec3(  0.4, 0.4,  0.3 ) );
//	sh = Shadow(pos,  sunLight);
    // Light surface with 'sun'...
	vec3 col = mat * SUN_COLOUR*(max(dot(sunLight,normal), 0.0)) *sh;
    //col += mat * vec3(0., .0, .15)*(max(dot(-sunLight,normal), 0.0));
    
    normal = reflect(eyeDir, normal); // Specular...
    col += pow(max(dot(sunLight, normal), 0.0), 12.0)  * SUN_COLOUR * .5 *sh;
    // Abmient..
    col += mat * .2 * max(normal.y, 0.2);
    col = mix(FOG_COLOUR,col, clamp(exp(-d*.05)+.03,0.0, 1.0));
    
	return col;
}

//--------------------------------------------------------------------------
vec3 GetNormal(vec3 p, float sphereR)
{
	vec2 eps = vec2(sphereR, 0.0);
	return normalize( vec3(
           Map(p+eps.xyy) - Map(p-eps.xyy),
           Map(p+eps.yxy) - Map(p-eps.yxy),
           Map(p+eps.yyx) - Map(p-eps.yyx) ) );
}

//--------------------------------------------------------------------------
float SphereRadius(in float t)
{
    t = t * .001*(500./iResolution.y);
    return (t+.001);
}

//--------------------------------------------------------------------------
float Scene(in vec3 rO, in vec3 rD)
{

	float t = .05 * Hash(fcoord);
	
	vec3 p = vec3(0.0);

	for( int j=0; j < 180; j++ )
	{
		if (t > 24.0) break;
		p = rO + t*rD;
		float sphereR = SphereRadius(t);
		float de = Map(p);
		if(abs(de) < sphereR) break;
		t +=  de*.8;
	}

	return t;
}

//--------------------------------------------------------------------------
vec3 PostEffects(vec3 rgb, vec2 xy)
{
	// Gamma first...
	rgb = pow(rgb, vec3(0.45));

	// Then...
	#define CONTRAST 1.4
	#define SATURATION 1.4
	#define BRIGHTNESS 1.3
	rgb = mix(vec3(.5), mix(vec3(dot(vec3(.2125, .7154, .0721), rgb*BRIGHTNESS)), rgb*BRIGHTNESS, SATURATION), CONTRAST);

	// Vignette...
	rgb *= .4+0.6*pow(180.0*xy.x*xy.y*(1.0-xy.x)*(1.0-xy.y), 0.35);	

	return clamp(rgb, 0.0, 1.0);
}

//--------------------------------------------------------------------------
vec3 TexCube( sampler2D sam, in vec3 p, in vec3 n )
{
	vec3 x = texture( sam, p.yz ).xzy;
	vec3 y = texture( sam, p.zx ).xyz;
	vec3 z = texture( sam, p.xy ).yzx;
	return (x*abs(n.x) + y*abs(n.y) + z*abs(n.z))/(abs(n.x)+abs(n.y)+abs(n.z));
}

//--------------------------------------------------------------------------
vec3 Albedo(vec3 pos, vec3 nor)
{
    vec3 col = TexCube(iChannel0, pos*1.3, nor).zxy;
    col *= Colour(pos);
    return col;
}

//----------------------------------------------------------------------------------------

vec2 rot2D(inout vec2 p, float a)
{
    return cos(a)*p - sin(a) * vec2(p.y, -p.x);
}

//--------------------------------------------------------------------------

#include "RayTracing.glsl"

vec4 RayTrace (const Ray ray, const vec2 fragCoord)
{
    fcoord = fragCoord;
    vec2 xy = fragCoord.xy / iResolution.xy;
   
	vec3 col = vec3(1.0);
	vec3 cameraPos = ray.origin;
	vec3 dir = ray.dir;

	cameraPos += vec3(iTime * 0.5 - 0.4, 0.3, 2.14);

    float dis = Scene(cameraPos, dir);
	
    if (dis < 24.0)
    {
	    vec3 pos = cameraPos + dir * dis;
  		float sphereR = SphereRadius(dis);
        vec3 normal = GetNormal(pos, sphereR);

    	float sha = Shadow(pos, sunLight);
    
        vec3 alb = Albedo(pos, normal);
        col = DoLighting(alb, pos, normal, dir, dis, sha);
    }else
    {
   		col = FOG_COLOUR;
        col += pow(max(dot(sunLight, dir), 0.0), 5.0)  * SUN_COLOUR;
    }
    
	col = PostEffects(col, xy);
	
	return vec4(col,dis);
}

//--------------------------------------------------------------------------

void mainVR (out float4 fragColor, in float2 fragCoord, in float3 fragRayOri, in float3 fragRayDir)
{
	Ray	ray = Ray_Create( fragRayOri, fragRayDir, 0.1 );
	fragColor = RayTrace( ray, fragCoord );
}

void mainImage (out float4 fragColor, in float2 fragCoord)
{
	Ray	ray = Ray_From( iCameraFrustumLB, iCameraFrustumRB, iCameraFrustumLT, iCameraFrustumRT,
						iCameraPos, 0.1, fragCoord / iResolution.xy );
	fragColor = RayTrace( ray, fragCoord );
}
