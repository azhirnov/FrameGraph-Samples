// from https://www.shadertoy.com/view/Mtj3DG

// Ancient Ruins
// Scene from Karla Quintero's modern dance performance "If Mars"
//
// Mashup of heavy RMF, Kali's fractal (and stars I believe) and Nimitz's fog trick.
// Added IQ's noise for posting on shadertoy.
// Apologies for the brute force detail and hence slowness.
// 
// @rianflo


float smin(float a, float b, float k)
{
	return -(log(exp(k*-a)+exp(k*-b))/k);
}

float hash( float n )
{
    return fract(sin(n)*43758.5453);
}

#if 1
float noise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 f = fract(x);
	f = f*f*(3.0-2.0*f);
	
	vec2 uv = (p.xy+vec2(37.0,17.0)*p.z) + f.xy;
	vec2 rg = textureLod( iChannel0, (uv+ 0.5)/256.0, 0.0 ).yx;
	return mix( rg.x, rg.y, f.z );
}
#else
float noise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 f = fract(x);

    f = f*f*(3.0-2.0*f);
    float n = p.x + p.y*57.0 + 113.0*p.z;
    return mix(mix(mix( hash(n+  0.0), hash(n+  1.0),f.x),
                   mix( hash(n+ 57.0), hash(n+ 58.0),f.x),f.y),
               mix(mix( hash(n+113.0), hash(n+114.0),f.x),
                   mix( hash(n+170.0), hash(n+171.0),f.x),f.y),f.z);
}
#endif
mat3 rotationMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat3(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c);
}


const float PI = 3.1415926535897932384626433832795;


// ----------------------------------------------------------EDIT ME----------------------------------------------------------------------
float rmf(vec3 p)
{
    float signal = 0.0;
    float value  = 0.0;
    float weight = 1.0;
    float h = 1.0;
    float f = 1.0;

    for (int curOctave=0; curOctave < 11; curOctave++) 
    {
        signal = noise(p)*2.0-0.4;
        signal = pow(1.0 - abs(signal), 2.0) * weight;
        weight = clamp(0.0, 1.0, signal * 16.0);
        value += (signal * pow(f, -1.0));
        f *= 2.0;
        p *= 2.0;
    }
    
    return (value * 1.25) - 1.0;
}

const int Iterations=15;

const float Scale=2.1;

float de(vec3 pos) 
{
	vec3 p=pos;
    p.y +=4.76-iTime*0.005;
	p.xz=abs(.5-mod(pos.xz,1.))+.01;
	float DEfactor=1.;
	float ot=1000.;
	for (int i=0; i<Iterations; i++) 
    {
		p = abs(p)-vec3(0.,2.,0.);  
		float r2 = dot(p, p);
		ot = min(ot,abs(length(p)));
		float sc=Scale/clamp(r2,0.4,1.);
		p*=sc; 
		DEfactor*=sc;
		p = p - vec3(0.5,1.,0.5);
	}
    float rr=length(pos+vec3(0.,-3.03,1.85))-.017;
    float d=length(p)/DEfactor-.0005;
	
    return d;
}

float sdf(vec3 p)
{
	float sd = p.y;
    
    
	sd = smin(sd, de(p), 60.0);
    //sd = min(sd, length(p)-1.0-rmf(p+time*0.01, 8.0, 1.8, coneR)*0.06);
    sd -= rmf(p*8.0+2.0)*0.015;       // ceiling
    
    return sd;
}

vec4 getColorAndRoughness(vec3 p, vec3 N, float ambo)
{
    return vec4(1.0);
}

vec3 grad(vec3 p, float coneR)
{
    coneR*=3.0;
    vec3 f = vec3(sdf(p));
    vec3 g = vec3(sdf(p+vec3(coneR, 0.0, 0.0)),
                  sdf(p+vec3(0.0, coneR, 0.0)),
                  sdf(p+vec3(0.0, 0.0, coneR)));
    return (g-f) / coneR;
}

vec3 setupRayDirection(float camFov)
{
	vec2 coord = vec2(gl_FragCoord.xy);
    vec2 v = vec2(coord / iResolution.xy) * 2.0 - 1.0;
    float camAspect = iResolution.x/iResolution.y;
    float fov_y_scale = tan(camFov/2.0);
    vec3 raydir = vec3(v.x*fov_y_scale*camAspect, v.y*fov_y_scale, -1.0);
    return normalize(raydir);
}

float ambientOcclusion( in vec3 pos, in vec3 nor, float coneR )
{
  float occ = 0.0;
  float sca = 1.1;
  for ( int i=0; i<5; i++ )
  {
    float hr = 0.02 + 0.11*float(i) / 4.0;
    vec3 aopos =  nor * hr + pos;
    float dd = sdf(aopos);
    occ += -(dd-hr)*sca;
    sca *= 0.95;
  }
  return clamp( 1.0 - 1.5*occ, 0.0, 1.0 );
}


vec3 shade(vec3 ro, vec3 rd, float t, float coneR, vec3 ld)
{
    vec3 g = grad(ro+rd*t, coneR);
    vec3 N = normalize(g);
    float ambo = ambientOcclusion(ro+rd*t, N, coneR);
    float ndl = clamp(dot(N, ld), 0.0, 1.0);
    vec3 color = mix(vec3(0.782, 0.569, 0.45), vec3(0.8, 0.678, 0.569), clamp(1.5-length(g), 0.0, 1.0));
    return vec3(mix(ambo*color, ambo*color*ndl, 0.5));
}

float fogmap(in vec3 p, in float d)
{
    vec3 q = p;
    q.z-=0.0;
    p.x -= iTime*0.05;
    vec3 turb = vec3(noise(80.0*p.xyz+iTime*0.51)*0.5, noise(160.0*p.xzy+iTime*0.2)*0.3, noise(60.0*p.zyx+iTime*0.1)*0.2);
    p += turb;
    float fog = (max(noise(p*64.0+0.1)-0.1, 0.0)*noise(p*16.0))*0.03;
    
    return fog;
}


#define iterations 14
#define formuparam 0.530

#define volsteps 3
#define stepsize 0.2

#define zoom   1.20
#define tile   0.850
#define speed  0.1

#define brightness 0.0015
#define darkmatter 0.400
#define distfading 0.160
#define saturation 0.400

vec3 space(vec2 uv)
{
    vec3 v=vec3(0.4);
    vec3 dir=vec3(uv*zoom,1.);
	
	//float a2=2.0;
	float a1=4.0;//+time*0.001;
	mat2 rot1=mat2(cos(a1),sin(a1),-sin(a1),cos(a1));
	mat2 rot2=rot1;//mat2(cos(a2),sin(a2),-sin(a2),cos(a2));
	dir.xz*=rot1;
	dir.xy*=rot2;
	
	//from.x-=time;
	//mouse movement
	vec3 from=vec3(170.0, 170.2,0.01);
	from+=vec3(0.0,0.0,-2.);
	
	//from.x-=iMouse.x;
	//from.y-=iMouse.y;
	
	from.xz*=rot1;
	from.xy*=rot2;
	
	//volumetric rendering
	float s=.4,fade=.2;
	
    
    for (int r=0; r<volsteps; r++) 
    {
        vec3 q = vec3(0.3, 0.5, -3.0)+s*dir;
        
		vec3 p=from+s*dir*.5;
		p = abs(vec3(tile)-mod(p,vec3(tile*2.))); // tiling fold
		float pa,a=pa=0.;
		for (int i=0; i<iterations; i++) 
        { 
			p=abs(p)/dot(p,p)-formuparam; // the magic formula
			a+=abs(length(p)-pa); // absolute sum of average change
			pa=length(p);
		}
        
		float dm=max(0.,darkmatter-a*a*.001); //dark matter
		a*=a*a*2.; // add contrast
		if (r>3) fade*=1.-dm; // dark matter, don't render near
		//v+=vec3(dm,dm*.5,0.);
		v+=fade;
		v+=vec3(s,s*s,s*s*s*s)*a*brightness*fade; // coloring based on distance
		fade*=distfading; // distance fading
		s+=stepsize;

	}
	v=mix(vec3(length(v)),v,saturation); //color adjust
    return v;
}

#include "RayTracing.glsl"

vec4 RayTrace (const Ray ray, const vec2 fragCoord)
{
    vec2 p = (-iResolution.xy + 2.0 * fragCoord.xy) / iResolution.y;
    
    vec3 lightDir = normalize(vec3(0.0, 0.4, -0.3));
    //float v = mouse.y;
	vec3 rayOrigin = vec3(0.0, 0.01, 0.7);
    mat3 rm = rotationMatrix(vec3(0.0, 0.8, -0.15), -2.1);
    vec3 rayDir = rm*setupRayDirection(radians(60.0));

#if 1
    rayOrigin += ray.origin * 0.1;
    rayDir = ray.dir;
#endif
    
    rayOrigin += vec3(-0.0171305817, -0.201323241, 1.36527121) * 0.1;
    //rayOrigin += vec3(-0.3, -0.01, 2.05) * 0.1;
    rayOrigin += vec3(0.0, iTime*0.002, -iTime*0.008);

    float travel = 0.0;
    
    vec3 sp = vec3(0.0);
    float coneR;
    vec3 col = space(p)*0.01;
    float fog = 0.0;
    
    for (int i=0; i<256; i++)
    {
        coneR = travel * tan(0.5*radians(60.0)/iResolution.y);
        float sd = sdf(rayOrigin+rayDir*travel);
        
        if (sd < coneR)
        {
            col = shade(rayOrigin, rayDir, travel, coneR, lightDir);
            col = mix(col, vec3(0.4, 0.4, 0.5), smoothstep(3.0, 4.0, travel));
            break;
        }
        
        fog += fogmap(rayOrigin+rayDir*travel, travel);
        if (travel > 4.0 )
        {
            col = mix(col, vec3(0.4, 0.4, 0.5), smoothstep(1.0, 0.0, p.y));
            break;
        }
        
        travel += min(sd + travel * .001, 8.0);
    }
    
    fog = min(fog, 1.0);
    
    col = mix(col, vec3(0.97, .95, .9), fog);
    col = pow(col, vec3(1.2));
    
    //vec2 q = fragCoord.xy/iResolution.xy;
    //col *= pow(16.0*q.x*q.y*(1.0-q.x)*(1.0-q.y),0.1);
    
    return vec4(col, 1.0);
}
//-----------------------------------------------------------------------------


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
