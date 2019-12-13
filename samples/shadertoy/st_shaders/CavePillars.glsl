// from https://www.shadertoy.com/view/Xsd3z7

#include "RayTracing.glsl"

float MAX = 120.0;
float PRE = 0.01;
vec3 L = normalize(vec3(0.1,-0.3,-0.8));

//Camera Variables
vec3 P, A, D, X, Y, Z;

//Tri-planar Texturing Function
vec3 tritex(sampler2D tex, vec3 p)
{
 	return  (texture(tex,p.xy).rgb
            +texture(tex,p.zy).rgb
            +texture(tex,p.xz).rgb)/3.0;
}
//Smooth Tri-planar Texturing Function (https://www.shadertoy.com/view/Xd3XDS)
vec3 tritex(sampler2D tex, vec3 p, vec3 n)
{
 	return  (texture(tex,p.xy).rgb*n.z*n.z
            +texture(tex,p.zy).rgb*n.x*n.x
            +texture(tex,p.xz).rgb*n.y*n.y);
}
//Main Distance Field Function
float model(vec3 p)
{
    vec3 n = p * vec3(1,1,0.5);
    float V = (tritex(iChannel0,n/64.0).r-0.4)*4.0;
          V += (tritex(iChannel0,n/23.0).r-0.5)/2.0;
    return V;
}
// Grey scale by Shane
float grey(vec3 p)
{ 
    return dot(p, vec3(.299, .587, .114)); 
}
// Texture bump mapping by Shane also.
vec3 bump( sampler2D tex, vec3 p, vec3 n, float bumpfactor)
{
 	const vec2 pre = vec2(0,1)*0.001; // Change to suit needs.
	vec3 nor = vec3(grey(tritex(tex, p-pre.yxx,n)),
 					 grey(tritex(tex,p-pre.xyx,n)),
 					 grey(tritex(tex,p-pre.xxy,n)))/pre.y;

 	nor -= grey(tritex(tex,p,n))/pre.y; 
 	nor -= n*dot(n,nor); 

 	return normalize(n+nor*bumpfactor);
}

//Normal Function
vec3 normal(vec3 p)
{
 	vec3 N = vec3(-8,8,0) * PRE;
 	N = normalize(model(p+N.xyy)*N.xyy+model(p+N.yxy)*N.yxy+model(p+N.yyx)*N.yyx+model(p+N.xxx)*N.xxx);
 	return bump(iChannel2,p/4.0,N,0.01);
}
//Color/Material Function
vec3 color(vec3 p, vec3 d)
{
    vec3 N = normal(p);
    vec3 C = (tritex(iChannel1,p/4.0,N)
        	  +tritex(iChannel1,p,N))/2.0;
 	float TL = (dot(N,L)*0.5+0.5);
    TL *= clamp(1.0-model(p-L*4.0),0.1,1.0);
    return C*TL+pow(abs((dot(reflect(N,L),-d)*0.5+0.5)),16.0);
}
//Simple Raymarcher
vec4 raymarch(vec3 p, vec3 d)
{
    float S = 0.0;
    float T = S;
    vec3 D = normalize(d);
    vec3 P = p+D*S;
    for(int i = 0;i<240;i++)
    {
        S = model(P);
        T += S;
        P += D*S;
        if ((T>MAX) || (S<PRE)) {break;}
    }
    return vec4(P,min(T/MAX,1.0));
}

vec4 RayTrace (const Ray ray, const vec2 fragCoord)
{
    P = vec3(iTime*2.0, 0.0, 0.0);
    A = vec3(-0.5, -0.5,0.0);
	D = mix(vec3(1.0,0.0,0.0),vec3(cos(A.x)*cos(A.y),sin(A.x)*cos(A.y),sin(A.y)), 0.0);

	X = normalize(D);
	Y = cross(X,vec3(0,0,1));
	Z = cross(X,Y);
    
	vec2 UV = (fragCoord.xy-iResolution.xy*0.5)/iResolution.y;
    
    D = normalize(mat3(X,Y,Z) * vec3(1.0,UV));

#if 1 // manual camera
    P += ray.origin;
    D = ray.dir.xzy * vec3(1,1,-1);
#endif
    
    vec4 M = raymarch(P,D);
    float fog = clamp(max(M.w,M.z/9.0),0.0,1.0);
	return vec4(mix(color(M.xyz,D)*max(1.0-M.w*2.0,0.0),vec3(0.5,0.56,0.6),fog),1.0);
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
