// Author: ocb
// Title: I've seen... 
// https://www.shadertoy.com/view/ll3fz4

/***********************************************************************
Evolution based on Hope shader:  https://www.shadertoy.com/view/MllfDX
Still playing around with grids, (intersecting - overlaping...) to manage
lots of objects in order to generate all kind of cities.
Three intersecting grids generate variable size voxels which contains
cylinder (pipe), or sphere (little dome) or squared building.
One overlaping grid to generate big dome.

Using Shane's functions (the famous "Old nVidia tutorial) to create the decrepit
surface of the building.

Use mouse to look around

Sorry for thr unreadable code.
************************************************************************/

#include "RayTracing.glsl"

#define PI 3.141592653589793
#define PIdiv2 1.57079632679489
#define TwoPI 6.283185307179586
#define INFINI 1000000.
#define MAXSTEP 127
#define TOP_SURFACE 2.81

/*************  GRID ****************************************************/
#define A .13
#define B .41
#define C .47

#define G2 .02
#define GS .003

/*************  Object **************************************************/
#define GND 0
#define SKY 1
#define CYL 2
#define SPH 3
#define SIDE 4
#define SPHBIS 5

#define DUST vec3(1.,.9,.7)



#define moonCtr vec3(.10644925908247,.266123147706175,.958043331742229)
#define moonShad vec3(-0.435448415412873,0.224265278324226,0.87183126948543)
#define moonRefl vec3(0.524134547566281, 0.21940212059613, 0.822888622794975)

#define starLight vec3(0.814613079962557,0.203653269990639,-0.543075386641705)

int hitObj = SKY;
float hitScale = 1.;
bool WINDOW = false;
bool SIDEWIN = false;

float Hsh(in float v) { 						
    return fract(sin(v) * 437585.);
}


float Hsh2(in vec2 st) { 						
    return fract(sin(dot(st,vec2(12.9898,8.233))) * 43758.5453123);
}


float sphereImpact(in vec3 pos, in vec3 ray, in vec3 O, in float R, inout vec3 norm ){
    float d=0., t = INFINI;
    vec3 a = O - pos;
    float b = dot(a, ray);
    
    if (b >= 0.){	// check if object in frontside first (not behind screen)
        float c = dot(a,a) - R*R;
        d = b*b - c;
        if (d >= 0.){
            float sd = sqrt(d);
            t = b - sd;
            norm = normalize(pos + t*ray - O);
        }
    }
    
    return t;
}

float cylinderImpact(in vec2 pos,in vec2 ray, in vec2 O, in float R, inout vec2 norm){
    float t=INFINI;
    vec2 d = pos - O;
    float a = dot(ray,ray);
    float b = dot(d, ray);
    float c = dot(d,d) - R*R;
    float discr = b*b - a*c;
    if (discr >= 0.){
        t = (-b - sqrt(discr))/a;
        if (t < 0.001) t = (-b + sqrt(discr))/a;
        if (t < 0.001) t=INFINI;

        norm = normalize(pos + t*ray - O);
    }
    return t;
}


vec4 voxInfo(in vec2 xz){
    vec2 dtp = fract(xz*A)/A;
    vec2 dtq = fract((xz)*B)/B;
    vec2 dtr = fract((xz)*C)/C;
    vec2 dm1 = min(min(dtp,dtq),dtr);
    
    dtp = 1./A-dtp;
    dtq = 1./B-dtq;
    dtr = 1./C-dtr;
    vec2 dm2 = min(min(dtp,dtq),dtr);

    vec2 size = dm1+dm2;
    vec2 ctr = xz+.5*(dm2-dm1);
    
    return vec4(ctr,size);
    
}

vec4 voxInfoBis(in vec2 xz){
    vec2 size = vec2(1./G2);
    vec2 ctr = (floor(xz*G2)+.5)*size;
    return vec4(ctr,size);
    
}

vec4 getNextPlan(in vec2 xz, in vec2 v){
    vec2 s = sign(v);
    vec2 d = step(0.,s);
    vec2 dtp = (d-fract(xz*A))/A/v;
    vec2 dtq = (d-fract((xz)*B))/B/v;
    vec2 dtr = (d-fract((xz)*C))/C/v;

    vec2 dmin = min(min(dtp,dtq),dtr);
    float tmin = min(dmin.x, dmin.y);
    
    s *= -step(dmin,vec2(tmin));
    
    return vec4(s.x,0.,s.y,tmin);
}

vec4 getNextPlanBis(in vec2 xz, in vec2 v){
    vec2 s = sign(v);
    vec2 d = step(0.,s);
    vec2 dtp = (d-fract(xz*G2))/G2/v;
    
    float tmin = min(dtp.x, dtp.y);
    
    s *= -step(tmin,vec2(tmin));
    
    return vec4(s.x,0.,s.y,tmin);
}


float gndFactor(in vec2 p)		// use for cam anti-collision
{
    vec2 f = 1.5-3.*abs(fract(p.xy*G2)-.5);
    f=smoothstep(.9,1.8,f);
    return 1.5*(f.x+f.y);   
}

float map(in vec2 xz)
{   
    vec2 p = floor(xz*A)/A;
    vec2 q = floor((xz)*B)/B;
    vec2 r = floor((xz)*C)/C;
    
    vec2 f = 1.5-abs(fract(p*G2)-.5)-abs(fract(q*G2)-.5)-abs(fract(r*G2)-.5);
    f=smoothstep(.9,1.8,f);
    float c = 1.5*(f.x+f.y);
    
    float Hp = c*Hsh2(p), Hq = c*Hsh2(q), Hr = c*Hsh2(r);
    float Pp = step(.6,Hp), Pq = step(.6,Hq), Pr = step(.5,Hr);
    
    float tex = 1.*Hp*Pp + .5*Hq*Pq +.3*Hr*Pr;	  
    hitScale = Pp + 2.5*Pq + 5.*Pr;
    
    return tex;
}


vec4 trace(in vec3 pos, in vec3 ray, inout vec4 flam)
{
    float dh = 0.;
    float t = 0.;
    
    if(pos.y > TOP_SURFACE){	// navigating directly to the top surface (perfos)
        if(ray.y >= 0.) return vec4(vec3(0.),INFINI);
        t = (TOP_SURFACE - pos.y)/ray.y + 0.00001;
    }
    
    vec4 wall = vec4(0.);	// wall.xyz is the normal of the wall. wall.w is the t parameter.
    
    for(int i = 0;i<MAXSTEP;i++){	// entering the voxel run
        
        vec3 p = pos+t*ray;
        if(p.y > TOP_SURFACE) break;	// when "looking" up, you may fly out before hiting
                                        // break immediately (perfos)
        float h = map(p.xz);
        float dh = p.y - h;
        if(dh<.0) return vec4(wall.xyz,t-.00002);	// you are suddenly below the floor?
                                                    // you hit the previous wall
        
        wall = getNextPlan(p.xz,ray.xz);	// find the next wall
        float tt = 0.;
        
        float th = dh/(-ray.y);			// find the floor
        th += step(th,0.)*INFINI;
        
        vec4 vox = voxInfo(p.xz);		// get voxel info: center and size
        float H = Hsh2(floor(100.*vox.xy));
        
        vec3 normsph = vec3(0.);		// Little dome
        float ts = INFINI;
        if(bool(step(H,.6)) && t<150.){
            ts = sphereImpact(p, ray, vec3(vox.x,h-.5,vox.y), H*min(vox.z,vox.w), normsph );
        }
           
        vec3 normcyl = vec3(0.);		// Pipe
        float tc = INFINI;
        if(bool(step(H,.2)) && t<100.){
            vec2 c = vox.xy+ (4.*H-.4)*vox.zw;
            float r = .025*min(vox.z,vox.w);
            float top = 2.5*H*TOP_SURFACE;
            tc = cylinderImpact(p.xz, ray.xz, c, r, normcyl.xz );
            tc += step(top,p.y+tc*ray.y)*INFINI;
            
            float Hc = (H-.15)*20.;
            float ti = iTime*.4;
                                        // Flam calculation
            if(bool(step(.15,H)*step(.95,Hsh(.0001*(vox.x+vox.y)*floor(Hc+ti))) )){
                float fti = fract(Hc+ti);
                vec2 len = c-pos.xz;
                float h = length(len)*ray.y/length(ray.xz)+pos.y;
                float dh = h - top;
                float d = abs(ray.z*len.x - ray.x*len.y);
                float mvt = 1.+cos(5.*dh);
                flam.rgb += min(1.,min(1.,min(vox.z,vox.w))*.2*mvt*(r+dh)/d-.1)
                       *step(0.,dh)
                       *((.8-abs(fti-.5))/(dh+.2) -.3 )
                       *texture(iChannel3,vec2(.2*h-.5*ti+Hc,d*(1.+fti)*dh*mvt+Hc)).rgb;
                flam*=flam*(2.-fti);
                flam.w = t;
            }
        }
            

        tt = min(min(ts,th),tc);
        tt = min(wall.w,tt);			// keep the min between floor and wall distance
        if(tt==th) return vec4(0.,1.,0.,t+tt);	// if first hit = floor return hit info (floor)
        else if(tt==ts){hitObj = SPH; return vec4(normsph,t+tt);}
        else if(tt==tc){hitObj = CYL; return vec4(normcyl,t+tt);}
        else tt = wall.w;		// else keep the local t parameter to the next wall
        
        t+= tt+.00001;			// update global t and do again
        if(t>250.) break;		// not necessary to go too far (perfos)
    }
    
    
    return vec4(0.,0.,0.,INFINI);
}

vec4 traceBis(in vec3 pos, in vec3 ray)
{
    float dh = 0.;
    float t = 0.;
    
    if(pos.y > TOP_SURFACE){	// navigating directly to the top surface (perfos)
        if(ray.y >= 0.) return vec4(vec3(0.),INFINI);
        t = (TOP_SURFACE - pos.y)/ray.y + 0.00001;
    }
    
    vec4 wall = vec4(0.);	// wall.xyz is the normal of the wall. wall.w is the t parameter.
    
    for(int i = 0;i<MAXSTEP;i++){	// entering the voxel run
        
        vec3 p = pos+t*ray;
        if(p.y > TOP_SURFACE) break;	// when "looking" up, you may fly out before hiting
                                        // break immediately (perfos)
        
        wall = getNextPlanBis(p.xz,ray.xz);	// find the next wall
                
        vec4 vox = voxInfoBis(p.xz);
        vec3 normsph;
        float radius = 25.;//.5*min(vox.z,vox.w);
        float tt = sphereImpact(p, ray, vec3(vox.x,TOP_SURFACE-radius-1.,vox.y), radius , normsph );
        
        if(tt<INFINI) return vec4(normsph,t+tt);
        else tt = wall.w;		// else keep the local t parameter to the next wall
        
        t+= tt+.00001;			// update global t and do again
        if(t>250.) break;		// not necessary to go too far (perfos)
    }
    
    
    return vec4(0.,0.,0.,INFINI);
}


float mapGnd(in vec3 p){return .5*texture(iChannel3, GS*p.xz).r +.2*texture(iChannel3, 2.*GS*p.xz).r;}

vec4 traceGround(in vec3 pos, in vec3 ray)
{
    if(ray.y>0.) return vec4(0.,0.,0.,INFINI);

    float t = (mapGnd(pos) - pos.y)/ray.y;
        
    for(int i = 0;i<MAXSTEP;i++){	
        
        vec3 p = pos+t*ray;
        float dh = p.y-mapGnd(p);
        
        if(abs(dh) < .003){
            vec2 e = vec2(-.2,.2);   
            vec3 norm = normalize(e.yxx*mapGnd(p + e.yxx) + e.xxy*mapGnd(p + e.xxy) + 
                             e.xyx*mapGnd(p + e.xyx) + e.yyy*mapGnd(p + e.yyy) );   
            
            return vec4(norm,t);
        }
        
        t += dh;
        
        if(t>250.) break;
    }
    
    return vec4(0.,0.,0.,INFINI);
}


vec4 boxImpact( in vec3 pos, in vec3 ray, in vec3 ctr, in vec3 dim) 
{
    vec3 m = 1.0/ray;
    vec3 n = m*(ctr-pos);
    vec3 k = abs(m)*dim;
    
    vec3 t1 = n - k;
    vec3 t2 = n + k;

    float tmax = max( max( t1.x, t1.y ), t1.z );
    float tmin = min( min( t2.x, t2.y ), t2.z );
    
    if( tmax > tmin || tmin < 0.0) return vec4(vec3(0.),INFINI);

    vec3 norm = -sign(ray)*step(t2, vec3(tmin));
    return vec4(norm, tmin);
}


bool checkWindow(in vec3 ctr){
    float hash = Hsh2(ctr.xz+ctr.yy);
    float a = step(.4,hash)*step(mod(ctr.y,10.),0.);
    float b = step(.7,hash)*step(mod(ctr.y-1.,10.),0.);
    return bool(a+b);
}

vec4 traceWindow(in vec3 pos, in vec3 ray, in float t, in vec3 norm){
    float d, sh, sv;
    if(hitObj == SPHBIS){d=.03; sh=2.5; sv=1.;}
    else if(hitObj == SPH){d=.03; sh=.5; sv=.75;}
    else if(hitObj == CYL){d=.01; sh=.1; sv= 1.;}
    else {d=.1; sh=1.; sv=1.;}
    
    vec3 p = pos + t*ray;
    vec4 info = vec4(norm,t);

    vec3 boxDim = vec3(sh*.25,sv*.025,sh*.25);
    vec3 boxCtr;
    
    for(int i=0; i<5; i++){
        boxCtr = vec3(floor(p.x*2./sh),floor(p.y*20./sv),floor(p.z*2./sh));
        if(checkWindow(boxCtr)){
            SIDEWIN = true;
            float tf = t + d/dot(ray,-norm);
            info = boxImpact(pos, ray, (boxCtr+.5)*vec3(sh*.5,sv*.05,sh*.5), boxDim);
            if(tf < info.w){
                WINDOW = true;
                info = vec4(norm,tf);
                break;
            } 
            p = pos + (info.w+.001)*ray;
        }
        else break;
    }
    return info;
}

vec3 starGlow(in vec3 ray, in float a){
    vec3 col = vec3(.001/(1.0001-a));
    return col;
}

vec3 moonGlow(in vec3 ray, in float a){
    float dl = dot(moonRefl,ray);
    float moon = smoothstep(.9,.93,a);
    float shad = 1.-smoothstep(.7,.9,dot(moonShad, ray));
    float refl = .001/(1.0013-dl);
    float clouds = .5*texture(iChannel3,ray.xy).r+.3*texture(iChannel3,3.*ray.xy).r;
    vec3 col = .8*(vec3(.5,.3,.0)+clouds*vec3(.3,.4,.1))*moon+vec3(1.,1.,.7)*refl;
    col += vec3(.3,.5,.8)*smoothstep(.89,.90,a)*(1.-smoothstep(.89,.99,a))*(dl-.9)*15.;
    col *= shad;
    col -= vec3(.1,.3,.6)*(1.-moon*shad);
    col = clamp(col,0.,1.);
    return col;
}

vec3 stars(in vec3 ray){
    vec3 col = vec3(0.);
    float az = atan(.5*ray.z,-.5*ray.x)/PIdiv2;
    vec2 a = vec2(az,ray.y);
    
    float gr = -.5+a.x+a.y;
    float milky = 1.-smoothstep(0.,1.2,abs(gr));
    float nebu = 1.-smoothstep(0.,.7,abs(gr));

    vec3 tex = texture(iChannel3,a+.3).rgb;
    vec3 tex2 = texture(iChannel3,a*.1).rgb;
    vec3 tex3 = texture(iChannel3,a*5.).rgb;
    float dark = 1.-smoothstep(0.,.3*tex.r,abs(gr));
    
    vec2 dty =a*12.;
    col += step(.85,Hsh2(floor(dty)))*(tex+vec3(.0,.1,.1))*max(0.,(.01/length(fract(dty)-.5)-.05));
    
    dty =a*30.;
    col += step(.8,Hsh2(floor(dty)))*tex*max(0.,(.01/length(fract(dty)-.5)-.05))*milky;
    
    dty =a*1000.;
    col += max(0.,Hsh2(floor(dty))-.9)*3.*tex3*milky;
    
    col += (.075+.7*smoothstep(.1,1.,(tex+vec3(.15,0.,0.))*.3))*nebu;
    col += .5*smoothstep(0.,1.,(tex2+vec3(0.,.2,.2))*.2)*milky;
    col -= .15*(tex3 * dark);
    
    return col;
}

vec3 fewStars(in vec3 ray){
    vec3 col = vec3(0.);
    float az = atan(.5*ray.z,-.5*ray.x)/PIdiv2;
    vec2 a = vec2(az,ray.y);
    
    vec3 tex = texture(iChannel3,a+.3).rgb;
    vec2 dty =a*14.;
    col += step(.85,Hsh2(floor(dty)))*(tex+vec3(.0,.1,.1))*max(0.,(.01/length(fract(dty)-.5)-.05));

    return col;
}

bool shadTrace(in vec3 pos, in vec3 v){
    float dh = 0.;
    float t = 0.;
    vec4 wall = vec4(0.);
    
    for(int i = 0;i<10;i++){       
        vec3 p = pos + t*v;
        if(p.y > TOP_SURFACE) break;
        
        float h = map(p.xz);
        float dh = p.y - h;
        if(dh<.0) return true;
        
        vec4 vox = voxInfo(p.xz);
        float H = Hsh2(floor(100.*vox.xy));
        vec3 normsph;
        float ts = sphereImpact(p, v, vec3(vox.x,h-.5,vox.y), step(H,.6)*H*min(vox.z,vox.w), normsph );
        if(ts<INFINI) return true;
        
        vec3 normcyl = vec3(0.);
        float tc = cylinderImpact(p.xz, v.xz, vec2(vox.x,vox.y)+ (4.*H-.4)*vox.zw, .025*min(vox.z,vox.w), normcyl.xz );
        tc += step(step(H,.2)*2.5*H*TOP_SURFACE,p.y+tc*v.y)*INFINI;
        if(tc<INFINI) return true;
        
        wall = getNextPlan(p.xz,v.xz);       
        t+= wall.w + .0001 ;
    }   
    return false;   
}

bool shadTraceBis(in vec3 pos, in vec3 v){
    float dh = 0.;
    float t = 0.;
    vec4 wall = vec4(0.);
    
    for(int i = 0;i<10;i++){       
        vec3 p = pos + t*v;
        if(p.y > TOP_SURFACE) break;
        
        vec4 vox = voxInfoBis(p.xz);
        vec3 normsph;
        float radius = 25.;
        float ts = sphereImpact(p, v, vec3(vox.x,TOP_SURFACE-radius-1.,vox.y), radius , normsph );
        if(ts<INFINI) return true;
        
        wall = getNextPlanBis(p.xz,v.xz);       
        t+= wall.w + .0001 ;
    }   
    return false;   
}

float shadow(in vec3 p){
    p += .00001*starLight;
    if(shadTrace(p,starLight)) return .1;
    if(shadTraceBis(p,starLight)) return .1;
    return 1.;
}

vec3 winGlow(in vec2 uv){
    uv.x *= .2;
    uv.y *= .5;
    vec2 k1 = (uv-.05*sin(uv*10.))*10.,
         k2 = (uv-.02*sin(uv*25.))*25.,
         k3 = (uv-.01*sin(uv*50.))*50.;
    
    
    vec2 p = floor(k1)/10.,
         q = floor(k2)/25.,
         s = floor(k3)/50.;
    
    vec2 bp = abs(fract(k1)-.5)
            + abs(fract(k2)-.5)
            + abs(fract(k3)-.5);
    bp /= 1.5;
    bp*=bp*bp;
    
    vec3 tex = texture(iChannel2,p).rgb
             + texture(iChannel2,q).rgb
             + texture(iChannel2,s).rgb;
    
    tex += .5*(bp.x+bp.y);
    tex *= smoothstep(1.,2.8,tex.r);
    
    return tex;
}


float metalPlate(in vec2 st){
    float coef = 0.;
    
    vec2 p = floor(st);
    float hp = Hsh2(p*0.543234); hp *= step(.2,abs(hp-.5));
    vec2 fp = fract(st)-.5;
    vec2 sfp = smoothstep(.475,.5,abs(fp));
    
    st *= vec2(.5,1.);
    vec2 q = floor(st*4.-.25);
    float hq = Hsh2(q*0.890976); hq *= step(.35,abs(hq-.5));
    vec2 fq = fract(st*4.-.25)-.5;
    vec2 sfq = smoothstep(.45,.5,abs(fq));
    
    st *= vec2(5.,.1);
    vec2 r = floor(st*8.-.25);
    float hr = Hsh2(r*0.123456); hr *= step(.47,abs(hr-.5));
    vec2 fr = fract(st*8.-.25)-.5;
    vec2 sfr = smoothstep(.4,.5,abs(fr));
    
    float h = max(max(hp,hq),hr);
    if(bool(h)){
        vec2 plate =    step(h,hp)*sfp*sign(fp)
                      + step(h,hq)*sfq*sign(fq) 
                      + step(h,hr)*sfr*sign(fr);
        
        coef += .2*h+.8;
        coef += .5*min(1.,plate.x+plate.y);
    }
    else coef = 1.;
    
    return coef;
}


float flare(in vec3 ref, in vec3 ctr){
    float c = 0.;
    vec3 
    s = normalize(ref);
    float sc = dot(s,-starLight);
    c += .6*smoothstep(.9995,1.,sc);
    
    s = normalize(ref+.9*ctr);
    sc = dot(s,-starLight);
    c += .7*smoothstep(.99,1.,sc);
    
    s = normalize(ref-.7*ctr);
    sc = dot(s,-starLight);
    c += .2*smoothstep(.7,1.,sc);
    
    return c;
}

vec3 lensflare3D(in vec3 ray, in vec3 ctr)
{
    vec3 red = vec3(1.,.6,.3);
    vec3 green = vec3(.6,1.,.6);
    vec3 blue = vec3(.6,.3,.6);
    vec3 col = vec3(0.);
    vec3 ref = reflect(ray,ctr);

    col += red*flare(ref,ctr);
    col += green*flare(ref-.07*ctr,ctr);
    col += blue*flare(ref-.14*ctr,ctr);
    
    ref = reflect(ctr,ray);
    col += red*flare(ref,ctr);
    col += green*flare(ref+.07*ctr,ctr);
    col += blue*flare(ref+.14*ctr,ctr);
    
    float d = dot(ctr,starLight);
    return .4*col*max(0.,d*d*d*d*d);
}

vec3 lava(in vec2 p){
    float tex = abs(texture(iChannel3,p*.05).r-.5+.3*sin(iTime*.2));
    vec3 lav = vec3(0.);
    lav.r += .5*(1.-smoothstep(.0,.2,tex));
    lav.rg += (1.-smoothstep(.0,.02,tex));
    return lav;
}

vec3 getCamPos(in vec3 camTarget){
    float 	rau = 15.,
            alpha = 0.0, //iMouse.x/iResolution.x*4.*PI,
            theta = 0.0; //iMouse.y/iResolution.y*PI+(PI/2.0001);	
            
            // to start shader
            //if (iMouse.xy == vec2(0.)){
                float ts = smoothstep(18.,22.,iTime)*(iTime-20.);
                alpha = iTime*.04;
                theta = .3*(1.-cos(iTime*.1));
            //}
    return rau*vec3(-cos(theta)*sin(alpha),sin(theta),cos(theta)*cos(alpha))+camTarget;
}

vec3 getRay(in vec2 st, in vec3 pos, in vec3 camTarget){
    float 	focal = 1.;
    vec3 ww = normalize( camTarget - pos);
    vec3 uu = normalize( cross(ww,vec3(0.0,1.0,0.0)) ) ;
    vec3 vv = cross(uu,ww);
    // create view ray
    return normalize( st.x*uu + st.y*vv + focal*ww );
}

// 2 functions from Shane
// the famous "Old Nvidia tutorial"!
vec3 tex3D( sampler2D tex, in vec3 p, in vec3 n ){
  
    n = max((abs(n) - 0.2)*7., 0.001);
    n /= (n.x + n.y + n.z );      
    vec3 tx = (texture(tex, p.yz)*n.x + texture(tex, p.zx)*n.y + texture(tex, p.xy)*n.z).xyz;   
    return tx*tx;
}

vec3 texBump( sampler2D tx, in vec3 p, in vec3 n, float bf, in float t, in vec3 ray){   
    //const vec2 e = vec2(0.001, 0);    
    vec2 e = vec2(.2*t/iResolution.x/dot(-ray,n),0.);    // AA purpose
    //vec2 e = vec2(.5*t/iResolution.x,0.);
    mat3 m = mat3( tex3D(tx, p - e.xyy, n), tex3D(tx, p - e.yxy, n), tex3D(tx, p - e.yyx, n));    
    vec3 g = vec3(0.299, 0.587, 0.114)*m; 
    g = (g - dot(tex3D(tx,  p , n), vec3(0.299, 0.587, 0.114)) )/e.x; g -= n*dot(n, g);                      
    return normalize( n + g*bf );
}


vec4 RayTrace (const Ray inRay, const vec2 fragCoord)
{
    vec2 st = ( fragCoord.xy
               //-.5*vec2(mod(fragCoord.y,2.),mod(fragCoord.x,2.))
               - .5*iResolution.xy ) / iResolution.y; 
    
    // camera def
    vec3 camTarget = vec3(100.*sin(iTime*.01),1.,-60.*sin(iTime*.015));
    
    vec3 pos = getCamPos(camTarget);
    
    pos.y = max(pos.y,1.5*gndFactor(pos.xz)+1.);	// Cam anti-collision
    
    vec3 ray = getRay(st, pos,camTarget);

#if 1
    pos += inRay.origin;
    ray = inRay.dir;
#endif
    
    float moonArea = dot(ray,moonCtr);
    float starArea = dot(ray,starLight);
    bool starside = bool(step(0.,starArea));
    bool moonside = bool(step(0.,moonArea));
    
    vec3 color = vec3(.0);
    vec4 flamInfo = vec4(0.);
    float t = INFINI;
    vec3 norm = vec3(0.);

    vec4 info = trace(pos, ray, flamInfo);
    float sc = hitScale;
    t = info.w;
    norm = info.xyz;
    
    info = traceBis(pos, ray);
    if(info.w < t){
        t=info.w;
        norm = info.xyz;
        hitObj = SPHBIS;
        if(flamInfo.w > t) flamInfo.rgb = vec3(0.);
    }
    
    info = traceGround(pos,ray);
    if(info.w < t){
        t=info.w;
        norm = info.xyz;
        hitObj = GND;
    }
    
    float shadow = shadow(pos+t*ray);
    
    if(t==INFINI){
        float moonMask = 1.-smoothstep(.925,.93,moonArea);
        float starMask = 1.-smoothstep(.4,.9,starArea);
        if(starside) color += .7*starGlow(ray,starArea);
        if(moonside) color += moonGlow(ray,moonArea);
        
        color += fewStars(ray)*moonMask*starMask;
        color += stars(ray)*moonMask*starMask;
        color *= min(1.,abs(10.*ray.y*ray.y));
        color += .006/(ray.y*ray.y+.015)*DUST;
    }
    else{
        if(norm.y < .99 && hitObj != GND) {
            info = traceWindow(pos ,ray, t, norm);
            if(bool(info.w)) {
                norm = info.xyz;
                t = info.w;
            }
        }
        
        vec3 p = pos + t*ray;

        if(hitObj == SPH){
            if(WINDOW){
                vec2 d = p.xz - voxInfo(p.xz).xy;
                vec3 window = winGlow(vec2(.5*atan(d.x,d.y),.8*length(d)));
                vec3 refl = reflect(ray,norm);
                color += smoothstep(.95,1.,dot(starLight,refl))*norm.z*step(1.,shadow);
                color += window*min(1., 30./t);
            }	
            else{
                vec2 ang;
                if(SIDEWIN) ang = p.xz;
                else ang = vec2(atan(norm.x,norm.z), atan(norm.y,length(norm.xz)));
                color += texture(iChannel0,ang).rgb;
                color *= metalPlate(4.*ang);
                color += .015*vec3(.5*sc,abs(sc-4.),8.-sc) * min(1.,10./t);
                norm = texBump( iChannel3,.2*norm, norm, .005, t, ray);
                color *= max(dot(norm, starLight),.0);
                if(SIDEWIN) color += vec3(.1,.05,.0);
                else color *= shadow;
            }
        }
        else if(hitObj == CYL){
            if(WINDOW){
                vec3 window = winGlow(2.*p.zy);
                vec3 refl = reflect(ray,norm);
                color += smoothstep(.95,1.,dot(starLight,refl))*norm.z*step(1.,shadow);
                color += window*min(1., 30./t);
            }	
            else{
                vec2 ang;
                if(SIDEWIN) ang = p.xz;
                else ang = vec2(atan(norm.x,norm.z), 2.*p.y);
                color += texture(iChannel0,ang).rgb;
                color *= metalPlate(2.*ang);
                color += .015*vec3(.5*sc,abs(sc-4.),8.-sc) * min(1.,10./t);
                color *= max(dot(norm, starLight),.0);
                if(SIDEWIN) color += vec3(.1,.05,.0);
                else color *= shadow;
            }
        }
        else if(hitObj == SPHBIS){
            if(WINDOW){
                vec2 d = p.xz - voxInfoBis(p.xz).xy;
                vec3 window = .5*winGlow(vec2(3.*atan(d.x,d.y),.25*length(d)));
                vec3 refl = reflect(ray,norm);
                color += smoothstep(.95,1.,dot(starLight,refl))*norm.z*step(1.,shadow);
                color += window*min(1., 30./t);
            }	
            else{
                vec2 ang;
                if(SIDEWIN) ang = .5*p.xz;
                else ang = vec2(atan(norm.x,norm.z), atan(norm.y,length(norm.xz)));
                color += texture(iChannel0,ang).rgb;
                color *= metalPlate(4.*ang);
                sc =1.;
                color += .015*vec3(.5*sc,abs(sc-4.),8.-sc) * min(1.,10./t);
                norm = texBump( iChannel3,2.*norm, norm, .002, t, ray);
                color *= max(dot(norm, starLight),.0);
                if(SIDEWIN) color += vec3(.1,.05,.0);
                else color *= shadow;

            }
        }    
        else if(hitObj == GND){
            vec3 dirt = texture(iChannel3,GS*p.xz).rgb;
            color = 1.5*p.y*dirt*dirt;
            
            color *= max(0.,dot(norm, -starLight));
            color *= shadow;
            
            vec2 e = t/iResolution.x/ray.y*ray.xz;		// 2 lines for AA purpose
            vec3 lavaCol = (lava(p.xz)+lava(p.xz+e)+lava(p.xz-e))/3.;
            
            color += .8*lavaCol*(1.-smoothstep(.0,.1,dirt.r));
        }
        else{
            if(p.y <.01) color = vec3(0.);
            else{
                if(WINDOW){
                    vec3 window = winGlow( (p.xy+p.z)*norm.z + (p.zy+p.x)*norm.x);
                    vec3 refl = reflect(ray,norm);
                    color += smoothstep(.95,1.,dot(starLight,refl))*norm.z*step(1.,shadow);
                    color += window*min(1., 30./t);
                }
                else{
                    vec2 side = .5*vec2(p.x,-p.z)*norm.y + .5*vec2(-p.x,-p.y)*norm.z + .5*vec2(p.y,-p.z)*norm.x;
                    color += texture(iChannel0,side).rgb;
                    
                    vec2 e = vec2(2.*t/iResolution.x/dot(-ray,norm),0.);		// 2 lines for AA purpose
                    float mp =  ( metalPlate(4.*side)
                                + metalPlate(4.*side+e)
                                + metalPlate(4.*side-e)
                                + metalPlate(4.*side+e.yx)
                                + metalPlate(4.*side-e.yx)) /5.;
                    
                    color *= mp;
                    color += .015*vec3(.5*sc,abs(sc-4.),8.-sc) * min(1.,10./t);
                    
                    norm = texBump( iChannel3, .2*p, norm, .003, t, ray);

                    color *= clamp(dot(norm, starLight)+.2,.3,1.);
                    if(SIDEWIN) color += vec3(.1,.05,.0);
                    else color *= shadow;

                    vec3 refl = reflect(ray,norm);
                    color += .3*smoothstep(.9,1.,dot(starLight,refl))*norm.z*step(1.,shadow);;
                    color = clamp(color,0.,1.);
                }
            }
        }
        color *= min(1., 20./t); 
        color += min(.003*t,.35)*DUST;
    }
    
    color += flamInfo.rgb*flamInfo.rgb;

    //if(starside)
        //if(!shadTrace(pos,starLight))
    //		color += lensflare3D(ray, getRay(vec2(0.), pos,camTarget));
    
    return vec4(1.5*color,1.);
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

    