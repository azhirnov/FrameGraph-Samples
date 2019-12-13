// from https://www.shadertoy.com/view/ltSyzd
//Wanted to try to make grass, it consist of a bunch of cone grids, rotated and distorded. 
//It work better than i expected even if there are a lot of artefact due to the heavy distortion.
//The main trick used to make it look ok is the normal sampling for the grass,
//gass normals use the underlying terrain distance field, that make it look smooth and unified,
//and hide the cone shape of the grass.

#include "RayTracing.glsl"

const float epsilon = 0.001;
const float pi = 3.14159265359;

#define LIGHT normalize(vec3(0.0, 0.5, 1.0))
const vec3 color0 = vec3(1.0, 0.5, 0.5);
const vec3 color1 = vec3(0.1, 0.1, 0.4);
const vec3 color2 = vec3(0.9, 0.4, 0.3);
const vec3 lightColor = vec3(0.4, 0.4, 0.3);

const float height = 0.25;
const float heightvar = 1.0;
const float density = 0.5;
const float thickness = 0.1;

const mat2 rot1 = mat2(0.99500416527,0.0998334166,-0.0998334166,0.99500416527);
const mat2 rot2 = mat2(0.98006657784,0.19866933079,-0.19866933079,0.98006657784);
const mat2 rot3 = mat2(0.95533648912,0.29552020666,-0.29552020666,0.95533648912);
const mat2 rot4 = mat2(0.921060994,0.3894183423,-0.3894183423,0.921060994);
const mat2 rot5 = mat2(0.87758256189,0.4794255386,-0.4794255386,0.87758256189);
const mat2 rot6 = mat2(0.82533561491,0.56464247339,-0.56464247339,0.82533561491);
const mat2 rot7 = mat2(0.76484218728,0.64421768723,-0.64421768723,0.76484218728);
const mat2 rot8 = mat2(0.69670670934,0.7173560909,-0.7173560909,0.69670670934);
const mat2 rot9 = mat2(0.62160996827,0.78332690962,-0.78332690962,0.62160996827);
const mat2 rot10 = mat2(0.54030230586,0.8414709848,-0.8414709848,0.54030230586);

//Some hashs function by Dave_Hoskins
//https://www.shadertoy.com/view/4djSRW
vec2 hash21(float p)
{
    vec3 p3 = fract(vec3(p) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.xx+p3.yz)*p3.zy);

}

vec3 hash32(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz+19.19);
    return fract((p3.xxy+p3.yzz)*p3.zyx);
}


mat2 rot(float a) 
{
    vec2 s = sin(vec2(a, a + pi/2.0));
    return mat2(s.y,s.x,-s.x,s.y);
}

vec2 sinNoise(vec2 p)
{
    vec2 p1 = p;
    vec2 p2 = p * rot2 * 0.4;
    vec2 p3 = p * rot6 * 0.7;
    vec2 p4 = p * rot10 * 1.5;
    vec4 s1 = sin(vec4(p1.x, p1.y, p2.x, p2.y));
    vec4 s2 = sin(vec4(p3.x, p3.y, p4.x, p4.y));
    
    return (s1.xy + s1.zw + s2.xy + s2.zw) * 0.25;
}

vec4 hash(vec4 p)
{
    return fract(sin(p)*12345.0);
}

//iq's noise
float iqNoise(vec2 x, float c) 
{
    vec2 p = floor(x);
    vec2 f = fract(x);
    //f = f*f*(3.0-2.0*f);
    float n = p.x + p.y*c;
    
    vec4 h = hash(vec4(n, n+1.0, n+c, n+c+1.0));
    
    return  mix(mix(h.x,h.y,f.x), mix(h.z,h.w,f.x), f.y);
}

//Simplified version of iq's noise, Output two 1D noise
vec2 noise1D(vec2 x) 
{
    vec2 p = floor(x);
    vec2 f = fract(x);
    vec2 n = p; 
    vec4 h = hash(vec4(n.x, n.x+1.0, n.y, n.y+1.0));
    
    return  mix(h.xz,h.yw,f);
}

//Distance Field function by iq :
//http://iquilezles.org/www/articles/distfunctions/distfunctions.htm
float sdBox( vec3 p, vec3 b )
{
  vec3 d = abs(p) - b;
  return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

float sdCylinder( vec3 p, vec3 c )
{
  return length(c.xy - p.xz) - c.z;
}

//https://www.shadertoy.com/view/Xds3zN
float sdCone( in vec3 p, in vec3 c )
{
    vec2 q = vec2( length(p.xz), p.y );
    float d1 = -q.y-c.z;
    float d2 = max( dot(q,c.xy), q.y);
    return length(max(vec2(d1,d2),0.0)) + min(max(d1,d2), 0.);
}

vec3 opRep( vec3 p, vec3 c )
{
    return mod(p,c)-0.5*c;
}

//taken from shane's desert canyon, originaly a modification of the smin function by iq
//https://www.shadertoy.com/view/Xs33Df
float smaxP(float a, float b, float s){
    
    float h = clamp( 0.5 + 0.5*(a-b)/s, 0., 1.);
    return mix(b, a, h) + h*(1.0-h)*s;
}

vec4 geom(vec3 pos)
{
    vec3 rep = vec3(40.0, 0.0, 40.0);
    
    vec3 hash = hash32(floor(pos.xz / rep.xz));
    
    vec3 p = pos + vec3(0.0, 2.0, 0.0);
    vec3 boxPos1 = opRep(p, rep);
    boxPos1.xz *= rot(hash.z*pi);
    float box1 = sdBox(boxPos1, vec3(5.0 + hash.z*2.0, 10.0 - hash.x*3.0, 7.0 + hash.y*2.0));

    p.xz *= rot3;
    p += vec3(0.25, 0.0, 0.0);
    vec3 boxPos2 = opRep(p, vec3(7.0, 0.0, 7.0));
    float box2 = sdBox(boxPos2, vec3(7.0, 100.0, 1.0));
    
    p.xz *= rot7;
    p += vec3(0.25, 0.0, 0.25);
    vec3 boxPos3 = opRep(p, vec3(10.0, 0.0, 10.0));
    float box3 = sdBox(boxPos3, vec3(3.0, 100.0, 10.0));

    float box = max(-box3, max(-box2, box1));
    
    float roofNoise = iqNoise(pos.xz*0.35, 800.0)*3.0 - hash.y*10.0 + 3.0;
    box = max(pos.y + roofNoise, box);
    
    vec2 trenchNoise = sinNoise(pos.xz*0.4);

    vec3 trenchPos = (pos.xzy - vec3(120.0, 0.0, 5.0));
    trenchPos.xy += trenchNoise*2.0;
    float trench = sdCylinder(trenchPos, vec3(0.0, 0.0, 10.0));
    
    float terrain = -textureLod(iChannel0, pos.xz*0.005, 0.0).x*5.0;
    
    terrain += textureLod(iChannel0, pos.xz*0.015, 0.0).x*2.0 + trenchNoise.x*1.5 + trenchNoise.y;

    float ground = smaxP(pos.y, -trench, 3.0);
    ground += terrain;
        
    float geom = min(box, ground);
    
    return vec4(geom, ground, box, terrain);
}

vec3 distfunc(vec3 pos)
{ 
    vec4 baseGeomtry = geom(pos);
       
    pos.y = baseGeomtry.y;
    float hvar = texture(iChannel0, pos.xz*0.075).x;
    float h = height + hvar*heightvar;
    
    vec2 t = iTime * vec2(5.0, 4.3);
    vec2 windNoise = sinNoise(pos.xz*2.5 + t);
    vec2 windNoise2 = sin(vec2(iTime*1.5, iTime + pi) + pos.xz*1.0) * 0.5 + vec2(2.0, 1.0);
    vec2 wind = (windNoise*0.45 + windNoise2*0.3) * (pos.y);

    pos.xz += wind;

    vec3 p1 = opRep(pos, vec3(density));
    p1 = vec3(p1.x, pos.y - h, p1.z);
    float g1 = sdCone(p1, vec3(1.0, thickness, h));
    
    pos.xz *= rot5;
    vec3 p2 = opRep(pos, vec3(density)*0.85);
    p2 = vec3(p2.x, pos.y - h, p2.z);
    float g2 = sdCone(p2, vec3(1.0, thickness, h));
    
    pos.xz *= rot10;
    vec3 p3 = opRep(pos, vec3(density)*0.7);
    p3 = vec3(p3.x, pos.y - h, p3.z);
    float g3 = sdCone(p3, vec3(1.0, thickness, h));
    
    pos.xz *= rot3;
    vec3 p4 = opRep(pos, vec3(density)*0.9);
    p4 = vec3(p4.x, pos.y - h, p4.z);
    float g4 = sdCone(p4, vec3(1.0, thickness, h));
    
    float g = min(min(g1, g2), min(g3, g4));
    
    float id = 1.0;
    
    if(baseGeomtry.z < epsilon)
        id = 0.0;
    
    return vec3(min(g, baseGeomtry.x), id, h);
}

vec4 rayMarch(vec3 rayDir, vec3 cameraOrigin)
{
    const int maxItter = 200;
    const float maxDist = 70.0;
    
    float totalDist = 0.0;
    vec3 pos = cameraOrigin;
    vec3 dist = vec3(epsilon, 0.0, 0.0);
    
    for(int i = 0; i < maxItter; i++)
    {
        dist = distfunc(pos);
        
        totalDist += dist.x; 
        
        pos += dist.x * rayDir;
        
        if(dist.x < epsilon || totalDist > maxDist)
        {
            break;
        }
    }
    
    return vec4(dist.x, totalDist, dist.y, dist.z);
}

//Camera Function by iq :
//https://www.shadertoy.com/view/Xds3zN
mat3 setCamera( in vec3 ro, in vec3 ta, float cr )
{
    vec3 cw = normalize(ta-ro);
    vec3 cp = vec3(sin(cr), cos(cr),0.0);
    vec3 cu = normalize( cross(cw,cp) );
    vec3 cv = normalize( cross(cu,cw) );
    return mat3( cu, cv, cw );
}

//Normal and Curvature Function by Nimitz;
//https://www.shadertoy.com/view/Xts3WM
vec4 norcurv(in vec3 p, float id)
{
    vec2 e = vec2(-epsilon, epsilon)*5.0;   
    vec4 t1 = geom(p + e.yxx), t2 = geom(p + e.xxy);
    vec4 t3 = geom(p + e.xyx), t4 = geom(p + e.yyy);

    float curv = (0.25/e.y)*(t1.z + t2.z + t3.z + t4.z - 4.0 * geom(p).z);
    
    vec3 nGround = normalize(e.yxx*t1.y + e.xxy*t2.y + e.xyx*t3.y + e.yyy*t4.y);
    vec3 nGroundBox = normalize(e.yxx*t1.z + e.xxy*t2.z + e.xyx*t3.z + e.yyy*t4.z);
    
    return vec4(mix(nGroundBox, nGround, id), curv);
}

vec3 lighting(vec3 n, vec3 rayDir, vec3 pos)
{
    float diff = dot(LIGHT, n);
    float rim = (1.0 - max(0.0, dot(-n, rayDir)));
    float spec = dot(reflect(LIGHT, n), rayDir);
    spec = pow(spec, 30.0);
    
    return smoothstep(vec3(0.0, 0.5, 0.0), vec3(1.0, 1.0, 1.0), vec3(diff, rim, spec)); 
}

vec3 sky(vec3 ray)
{
   vec3 diff = ray - LIGHT;
   float sunDist = clamp(length(diff), 0.0, 1.0);
   float at = (atan(diff.x, diff.y) + pi) / (2.0 * pi);
   float rays = textureLod(iChannel2, vec2(iTime*0.01, at), 0.0).x * (1.0 - sunDist);
    
   vec3 sun = smoothstep(vec3(0.05, 0.2, 1.5), vec3(0.0), vec3(sunDist)) * vec3(1.0, 0.2, 1.0) + rays*0.025;
    
   vec3 grad = mix(vec3(1.0, 0.9, 0.9), vec3(0.4, 1.0, 0.95) + sun.z*0.15, smoothstep(0.0, 0.8, ray.y));
    
   vec3 res = mix(vec3(1.0, 0.5, 0.3), grad, smoothstep(-0.5, 0.0, ray.y)) + sun.x + sun.y + sun.z*0.075;
       
   return res;
}

float clouds(vec3 uv)
{
    vec4 s1 = -abs(sin(uv.y*vec4(2.0, 3.0, 12.0, 20.0) + iTime * vec4(0.1,0.2,0.05,0.15)));
    vec4 s2 = -abs(sin(uv.x*vec4(3.0, 5.0, 13.0, 19.0) - iTime * vec4(0.15,0.1,0.2,0.1)));
    
    float res1 = (s1.x*0.5 + 0.4) + (s1.y*0.1 + 0.1) + s1.z*0.04 + s1.w*0.025;
    float res2 = (s2.x*0.3 + 0.1) + s2.y*0.1 + s2.z*0.03 + s2.w*0.015;
    
    float mask = clamp(1.0 - uv.z*0.35, 0.0, 1.0);
    
    return clamp(1.0 - (smoothstep(0.0, 0.65, uv.z + (res1 + res2)/2.0) + mask), 0.0, 1.0);
}

vec3 backGround(vec3 ray)
{  
   vec3 s = sky(ray);
   
    vec2 at = vec2(atan(ray.z, ray.x), atan(ray.x, ray.z));
    vec2 uv = abs(at) / pi;
    
   vec2 mNoise = noise1D(uv.xy * vec2(20.0, 15.0) + vec2(0.0, 1.3)) * vec2(0.1, 0.13);
   mNoise += noise1D(uv.xy * vec2(40.0, 50.0) + vec2(0.2, 1.8)) * vec2(0.05, 0.04);
   float m = mNoise.x + mNoise.y;            
   float mountains = clamp(smoothstep(-0.01, 0.01, ray.y - m + 0.05) + (1.0 - ray.y), 0.0, 1.0);
    
   vec3 coloredMountains = mix(vec3(0.5, 0.5, 1.0), vec3(1.0), mountains);
    
   float c = clouds(vec3(at.x, at.y, ray.y)*2.0)*0.75;
   float c2 = smoothstep(0.5, -0.4, abs(ray.x-0.8 + sin(ray.y*12.0)*0.1))*0.25 * (1.0 - ray.y);
    
   return s.xyz * coloredMountains + c*0.7 + c2*0.5;
}

vec3 TriplanarTexture(vec3 n, vec3 pos)
{
    n = abs(n);
    vec3 t1 = texture(iChannel1, pos.yz).xyz * n.x;
    vec3 t2 = texture(iChannel1, pos.zx).xyz * n.y;
    vec3 t3 = texture(iChannel1, pos.xy).xyz * n.z;
    
    return t1 * n.x + t2 * n.y + t3 * n.z;
}

vec4 RayTrace (const Ray ray, const vec2 fragCoord)
{
    float t = iTime + iDate.z*10.0;
    
    vec2 uv = fragCoord.xy/iResolution.xy;
    vec2 hash = hash21(floor(t*0.1)) * 2.0 - 1.0;
    
    float camX = 120.0;
    float camY = 7.0 + hash.y;
    float camZ = t*0.25 + floor(t*0.1)*50.0;                 
    vec3 cameraOrigin = vec3(camX, camY, camZ);
    
    vec3 cameraTarget = cameraOrigin + vec3(hash.x, 0.0, hash.y);
    
    vec2 screenPos = uv * 2.0 - 1.0;
    
    screenPos.x *= iResolution.x/iResolution.y;
    
    mat3 cam = setCamera(cameraOrigin, cameraTarget, hash.x*0.3);
    
    vec3 rayDir = cam*normalize(vec3(screenPos.xy,1.0));

#if 1 // manual camera
    cameraOrigin += ray.origin;
    rayDir = ray.dir;
#endif

    vec4 dist = rayMarch(rayDir, cameraOrigin);
    
    vec3 res;

    if(dist.x < epsilon)
    {
        vec3 pos = cameraOrigin + dist.y*rayDir;
        vec4 n = norcurv(pos, dist.z);
        vec3 r = reflect(rayDir, n.xyz);
        vec3 t = max(TriplanarTexture(n.xyz, pos*vec3(0.2, 0.05, 0.2)), TriplanarTexture(n.xyz, pos*vec3(0.3, 0.1, 0.3)));
        vec3 l = lighting(n.xyz, rayDir, pos);
        
        float fog = smoothstep(60.0, 20.0, dist.y);
        vec3 s = sky(r);
        
        vec3 col1 = mix(color1, color2, clamp(pos.y*0.1 + 0.5, 0.0, 1.0));
        col1 *= 0.75 + dist.w*0.5;
        col1 += l.x * lightColor + l.z * l.y * lightColor + s * l.y * 0.3;
        
        vec3 col2 = s * t * color0 + t*n.w;
        col2 += lightColor * (l.x + (l.y + l.z) * t);
        
        vec3 col = mix(col2, col1, dist.z);
            
        res = mix(sky(rayDir), col, fog);
    }
    else
    {
        res = backGround(rayDir); 
    }
    
    return vec4(max(vec3(0.0), res), 1.0);
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
