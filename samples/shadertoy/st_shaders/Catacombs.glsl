// from https://www.shadertoy.com/view/lsf3zr
// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

#define AA 1

#define SC 15.0

// http://iquilezles.org/www/articles/distfunctions/distfunctions.htm
float udBox( in vec3 p, in vec3 abc )
{
	return length(max(abs(p)-abc,0.0));
}
// http://iquilezles.org/www/articles/distfunctions/distfunctions.htm
float sdBox( in vec3 p, in vec3 b ) 
{
    vec3 q = abs(p) - b;
    return min(max(q.x,max(q.y,q.z)),0.0) + length(max(q,0.0));
}

float sdSqSegmentX( vec3 p, float l )
{
	float h = clamp( p.x/l, 0.0, 1.0 );
    p.x -= l*clamp( p.x/l, 0.0, 1.0 );
	return dot(p,p);
}


// http://iquilezles.org/www/articles/smin/smin.htm
float smin( float a, float b, float k )
{
    float h = max(k-abs(a-b),0.0);
    return min(a, b) - h*h*0.25/k;
}
// http://iquilezles.org/www/articles/smin/smin.htm
float smax( float a, float b, float k )
{
    float h = max(k-abs(a-b),0.0);
    return max(a, b) + h*h*0.25/k;
}

float roundcube( vec3 p, vec3 n )
{
    n = abs(n);
	float x = texture( iChannel0, p.yz ).x;
	float y = texture( iChannel0, p.zx ).x;
	float z = texture( iChannel0, p.xy ).x;
	return (x*n.x + y*n.y + z*n.z)/(n.x+n.y+n.z);
}

#define ZERO (min(iFrame,0))

//------------------------------------------

vec3 column( in float x, in float y, in float z )
{
    float y2=y-0.25;
    float y3=y-0.25;
    float y4=y-1.0;

    float dsp = abs( min(cos(1.5*0.75*6.283185*x/0.085), cos(1.5*0.75*6.283185*z/0.085)));
    dsp *= 1.0-smoothstep(0.8,0.9,abs(x/0.085)*abs(z/0.085));
    float di1=sdBox( vec3(x,mod(y+0.08,0.16)-0.08,z), vec3(0.10*0.85+dsp*0.03*0.25,0.079,0.10*0.85+dsp*0.03*0.25)-0.008 )-0.008;
    float di2=sdBox( vec3(x,y,z), vec3(0.12,0.29,0.12)-0.007 )-0.007;
    float di3=sdBox( vec3(x,y4,z), vec3(0.14,0.02,0.14)-0.006 )-0.006;
    float nx = max( abs(x), abs(z) );
    float nz = min( abs(x), abs(z) );	
    float di4=sdBox( vec3(nx, y, nz), vec3(0.14,0.3,0.05)-0.004 )-0.004;
	float di5=smax(-(y-0.291),sdBox( vec3(nx, (y2+nz)*0.7071, (nz-y2)*0.7071), vec3(0.12, 0.16*0.7071, 0.16*0.7071)-0.004)-0.004,0.007 + 0.0001);
    float di6=sdBox( vec3(nx, (y3+nz)*0.7071, (nz-y3)*0.7071), vec3(0.14, 0.10*0.7071, 0.10*0.7071)-0.004)-0.004;

    float dm1 = min(min(di5,di3),di2);
    float dm2 = min(di6,di4);
	vec3 res = vec3( dm1, 3.0, 1.0 );
	if( di1<res.x ) res = vec3( di1, 2.0, dsp );
    if( dm2<res.x ) res = vec3( dm2, 5.0, 1.0 );
    
	return res;
}

float wave( in float x, in float y )
{
    return sin(x)*sin(y);
}

float monster( in vec3 pos )
{
    pos -= vec3(0.64,0.5,1.5);

    float r2 = dot(pos,pos);

	float sa = smoothstep(0.0,0.5,r2);
	float fax = 0.75 + 0.25*sa;
	float fay = 0.80 + 0.20*sa;
    pos *= vec3( fax, fay, fax );
    
    r2 = dot(pos,pos);

	float r = sqrt(r2);

    {
        float a1 = 0.60;
        a1 += 0.1*sin(iTime);
        a1 *= 1.0-smoothstep( 0.0, 0.75, r );
        float si1 = sin(a1);
        float co1 = cos(a1);
        pos.xy = mat2(co1,si1,-si1,co1)*pos.xy;
	}


	//#define TENTACURA 0.04f
	#define TENTACURA 0.045
	float mindist2 = 1000000.0;

	float rr = 0.05+length(pos.xz);
    rr += 0.05;
    float ca = (0.5-TENTACURA*0.75) -6.0*rr*exp2(-9.0*rr);
    for( int j=1; j<7; j++ )
	{
		float an = (6.283185/7.0) * float(j);
        float aa = an + 0.3*rr*wave(6.0*rr,float(j)+4.95) + 0.29;
		float rc = cos(aa);
        float rs = sin(aa);
		vec3 q = vec3( pos.x*rc-pos.z*rs, pos.y+ca, 
                       pos.x*rs+pos.z*rc );
        mindist2 = min(mindist2,sdSqSegmentX(q,1.5));
	}


	float c = sqrt(mindist2) - TENTACURA;
    float br = 0.5+0.5*sin(2.0*iTime);
	float d = r-0.30 - 0.02*br*br;
	
    return 0.65*smin(c,d+0.17,0.3);
}

vec3 map( vec3 pos )
{
pos /= SC;

    // floor
    vec2 id = floor((pos.xz+0.1)/0.2 );
    float h = 0.012 + 0.008*sin(id.x*2313.12+id.y*3231.219);
    vec3 ros = vec3( mod(pos.x+0.1,0.2)-0.1, pos.y, mod(pos.z+0.1,0.2)-0.1 );
    vec3 res = vec3( udBox( ros, vec3(0.096,h,0.096)-0.005 )-0.005, 0.0, 0.0 );

    // ceilin
	float x = fract( pos.x+128.0 ) - 0.5;
	float z = fract( pos.z+128.0 ) - 0.5;
    float y = (1.0 - pos.y)*0.6;// + 0.1;
    float dis = 0.4 - smin(sqrt(y*y+x*x),sqrt(y*y+z*z),0.01);
    float dsp = abs(sin(31.416*pos.y)*sin(31.416*pos.x)*sin(31.416*pos.z));
    dis -= 0.02*dsp;

	dis = max( dis, y );
    if( dis<res.x )
    {
        res = vec3(dis,1.0,dsp);
    }

    // columns
	vec2 fc = fract( pos.xz+128.5 ) - 0.5;
	vec3 dis2 = column( fc.x, pos.y, fc.y );
    if( dis2.x<res.x )
    {
        res = dis2;
    }
    
    fc = fract( pos.xz+128.5 )-0.5;
    dis = length(vec3(fc.x,pos.y,fc.y)-vec3(0.0,-0.565,0.0))-0.6;
    dis -= texture(iChannel0,1.5*pos.xz).x*0.02;
    if( dis<res.x ) res=vec3(dis,4.0,1.0);
    
    
    #if 1
    dis = monster( pos );
    if( dis<res.x )
    {   
        res=vec3(dis,7.0,1.0);
    }
    #endif
    
	res.x*=SC;
    return res;
}

vec4 calcColor( in vec3 pos, in vec3 nor, in float sid, out float ke )
{
	vec3 col = vec3( 1.0 );
	float ks = 1.0;
    ke = 0.0;

    float kk = 0.2+0.8*roundcube( 1.0*pos, nor );
	
    if( sid<0.5 )
	{
        col = texture( iChannel1, 6.0*pos.xz ).xyz;
		vec2 id = floor((pos.xz+0.1)/0.2 );
    	col *= 1.0 + 0.5*sin(id.y*2313.12+id.x*3231.219);
	}
    else if( sid>0.5 && sid<1.5 )
	{
		float fx = fract( pos.x+128.0 ); 
	    float fz = fract( pos.z+128.0 ); 
		col = vec3(0.7,0.6,0.5)*1.3;
		float p = 1.0;
		p *= smoothstep( 0.02, 0.03, abs(fx-0.1) );
		p *= smoothstep( 0.02, 0.03, abs(fx-0.9) );
		p *= smoothstep( 0.02, 0.03, abs(fz-0.1) );
		p *= smoothstep( 0.02, 0.03, abs(fz-0.9) );
		col = mix( vec3(0.6,0.2,0.1), col, p );
	}
    else if( sid>1.5 && sid<2.5 )
	{
        float id = floor((pos.y+0.08)/0.16);
        col = vec3(0.7,0.6,0.5);
        col *= 1.0 + 0.2*cos(id*312.0 + floor(pos.x+0.5)*33.1 + floor(pos.z+0.5)*13.7);
	}
    else if( sid>2.5 && sid<3.5 )
	{
        col = vec3(0.7,0.6,0.5);
        col *= 0.25 + 0.75*smoothstep(0.0,0.1,pos.y);
	}
    else if( sid>3.5 && sid<4.5 )
	{
        col = vec3(0.2,0.15,0.1)*1.5;
        ks = 0.05;
    }
    else if( sid>4.5 && sid<5.5 )
	{
        col = vec3(0.6,0.2,0.1);
        col *= 0.25 + 0.75*smoothstep(0.0,0.1,pos.y);
        ks = 1.0;
	}
    else if( sid>6.5 && sid<7.5 )
	{
        //col = vec3(0.3,0.15,0.1)*0.3;
        col = vec3(0.25,0.13,0.1)*0.25;
        ks = 1.0*2.0;
        ke = 1.0;
	}
	
    return vec4(col * 1.2 * kk,ks);
}

vec3 castRay( in vec3 ro, in vec3 rd, in float precis, in float maxd )
{
    float t = 0.001;
    float dsp = 0.0;
    float sid = -1.0;
    for( int i=0; i<128; i++ )
    {
	    vec3 res = map( ro+rd*t );
        if( abs(res.x)<(precis*t)||t>maxd ) break;
	    sid = res.y;
		dsp = res.z;
        t += res.x;
    }

    if( t>maxd ) sid=-1.0;
    return vec3( t, sid, dsp );
}

float softshadow( in vec3 ro, in vec3 rd, in float mint, in float maxt, in float k )
{
	float res = 1.0;
    float t = mint;
    for( int i=0; i<64; i++ )
    {
        float h = map( ro + rd*t ).x;
        res = min( res, k*h/t );
        t += clamp(h,0.01,0.5);
		if( res<0.001 || t>maxt ) break;
    }
    return clamp( res, 0.0, 1.0 );
}

vec3 calcNormal( in vec3 pos )
{
#if 0    
	vec3 eps = vec3( 0.001, 0.0, 0.0 );
	vec3 nor = vec3(
	    map(pos+eps.xyy).x - map(pos-eps.xyy).x,
	    map(pos+eps.yxy).x - map(pos-eps.yxy).x,
	    map(pos+eps.yyx).x - map(pos-eps.yyx).x );
	return normalize(nor);
#else
    // inspired by klems - a way to prevent the compiler from inlining map() 4 times
    vec3 n = vec3(0.0);
    for( int i=ZERO; i<4; i++ )
    {
        vec3 e = 0.5773*(2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
        n += e*map(pos+e*0.001).x;
    }
    return normalize(n);
#endif    
}

vec3 doBumpMap( in vec3 pos, in vec3 nor )
{
    const float e = 0.001;
    const float b = 0.005;
    
	float ref = roundcube( 7.0*pos, nor );
    vec3 gra = -b*vec3( roundcube(7.0*vec3(pos.x+e, pos.y, pos.z),nor)-ref,
                        roundcube(7.0*vec3(pos.x, pos.y+e, pos.z),nor)-ref,
                        roundcube(7.0*vec3(pos.x, pos.y, pos.z+e),nor)-ref )/e;
	
	vec3 tgrad = gra - nor*dot(nor,gra);
    
    return normalize( nor-tgrad );
}

float calcAO( in vec3 pos, in vec3 nor )
{
    float ao = 0.0;
    float sca = 15.0;
    for( int i=ZERO; i<5; i++ )
    {
        float hr = SC*(0.01 + 0.015*float(i*i));
        float dd = map( pos + hr*nor ).x;
        ao += (hr-dd)*sca/SC;
        sca *= 0.5;
    }
    return 1.0 - clamp( ao, 0.0, 1.0 );
}

vec3 getLightPos( in int i )
{
    vec3 lpos;
    
    float la = 1.0;
    lpos.x = 0.5 + 2.2*cos(0.22+0.1*iTime + 17.0*float(i) );
    lpos.y = 0.25;
    lpos.z = 1.5 + 2.2*cos(2.24+0.1*iTime + 13.0*float(i) );

    // make the lights avoid the columns
    vec2 ilpos = floor( lpos.xz );
    vec2 flpos = lpos.xz - ilpos;
    flpos = flpos - 0.5;
    if( length(flpos)<0.2 ) flpos = 0.2*normalize(flpos);
    lpos.xz = ilpos + flpos;

    return lpos*SC;
}

vec4 getLightCol( in int i )
{
    float li = sqrt(0.5 + 0.5*sin(2.0*iTime+ 23.1*float(i)));
    float h = float(i)/8.0;
    vec3 c = mix( vec3(1.0,0.8,0.6), vec3(1.0,0.3,0.05), 0.5+0.5*sin(40.0*h) );
    return vec4( c, li );
}

const int kNumLights = 9;

vec3 render( in vec3 ro, in vec3 rd )
{ 
    vec3 col = vec3(0.0);
    vec3 res = castRay(ro,rd,0.00001*SC,10.0*SC);
    float t = res.x;
    if( res.y>-0.5 )
    {
        vec3 pos = ro + t*rd;
        vec3 nor = calcNormal( pos );

        float ao = calcAO( pos, nor );
        ao *= 0.7 + 0.6*res.z;

        pos /= SC;
		t /= SC;
        
        float ke = 0.0;
        vec4 mate = calcColor( pos, nor, res.y, ke );
        col = mate.xyz;
        float ks = mate.w;

        nor = doBumpMap( pos, nor );

        
        // lighting
        float fre = clamp(1.0+dot(nor,rd),0.0,1.0);
        vec3 lin = 0.03*ao*vec3(0.25,0.20,0.20)*(0.5+0.5*nor.y);
		vec3 spe = vec3(0.0);
        for( int i=0; i<kNumLights; i++ )
        {
            vec3 lpos = getLightPos(i);
            vec4 lcol = getLightCol(i);
            
            vec3 lig = lpos/SC - pos;
            float llig = dot( lig, lig);
            float im = inversesqrt( llig );
            lig = lig * im;
            float dif = clamp( dot( nor, lig ), 0.0, 1.0 );
			float at = 2.0*exp2( -2.3*llig )*lcol.w;
            dif *= at;
            float at2 = exp2( -0.35*llig );

			float sh = 0.0;
			if( dif>0.001 ) { sh = softshadow( pos*SC, lig, 0.02*SC, sqrt(llig)*SC, 32.0 ); dif *= sh; }

            float dif2 = clamp( dot(nor,normalize(vec3(-lig.x,0.0,-lig.z))), 0.0, 1.0 );
            lin += 0.20*dif2*vec3(0.35,0.20,0.10)*at2*ao;
            lin += 2.50*dif*lcol.xyz;
			
            lin += (0.7*fre*fre*fre*col+
                    0.3*fre*(0.5+col)
                   )*ao*(0.5+dif*0.5)*ke*0.2;
            
            
            vec3 hal = normalize(lig-rd);
            float pp = clamp( dot(nor,hal), 0.0, 1.0 );
            pp = pow(pp,1.0+ke*3.0);
            spe += ks*(5.0)*lcol.xyz*at*sh*dif*(0.04+0.96*pow(1.0-clamp(dot(hal,-rd),0.0,1.0),5.0))*(pow(pp,16.0) + 0.5*pow(pp,4.0));
            
        }
    
        col = col*lin + 2.0*spe + 1.0*ke*fre*fre*col*col*ao;
    }
	else
    {
		t /= SC;
    }
    
	col *= exp( -0.055*t*t );

    //col = col*1.6/(1.0+col);
    col = col*1.2/(1.0+col);

    
    // lights
	for( int i=0; i<kNumLights; i++ )
	{
        vec3 lpos = getLightPos(i);
        vec4 lcol = getLightCol(i);
        
        vec3 lv = (lpos - ro)/SC;
        float ll = length( lv );
        if( ll<t )
        {
            float dle = clamp( dot( rd, lv/ll ), 0.0, 1.0 );
			dle = 1.0-smoothstep( 0.0, 0.2*(0.7+0.3*lcol.w), acos(dle)*ll );
            col += dle*dle*6.0*lcol.w*lcol.xyz*exp( -0.07*ll*ll );
        }
    }

    
	return col;
}

#include "RayTracing.glsl"

vec4 RayTrace (const Ray ray, const vec2 fragCoord)
{
    vec2 p = (2.0*fragCoord-iResolution.xy)/iResolution.y;
    float time = iTime;

    // camera	
    vec3 ce = vec3( 0.5, 0.25, 1.5 );
    vec3 ro = ce + vec3( 1.3*cos(0.11*time), 0.65 - 0.4, 1.3*sin(0.11*time) );
    vec3 ta = ce + vec3( 0.95*cos(1.2+.08*time), 0.4*0.25+0.75*ro.y- 0.2, 0.95*sin(2.0+0.07*time) );
    ro *= SC;
    ta *= SC;
    float roll = -0.15*sin(0.1*time);

    // camera tx
    vec3 cw = normalize( ta-ro );
    vec3 cp = vec3( sin(roll), cos(roll),0.0 );
    vec3 cu = normalize( cross(cw,cp) );
    vec3 cv = normalize( cross(cu,cw) );
    vec3 rd = normalize( p.x*cu + p.y*cv + 1.5*cw );

#if 1 // manual camera
    rd = ray.dir;
    ro += ray.origin;
#endif

    vec3 col = render( ro, rd );

    col = pow( col, vec3(0.4545) );
    
    return vec4( col, 1.0 );
}
//-----------------------------------------------------------------------------


void mainVR (out vec4 fragColor, in vec2 fragCoord, in vec3 fragRayOri, in vec3 fragRayDir)
{
    Ray	ray = Ray_Create( fragRayOri, fragRayDir, 0.1 );
    fragColor = RayTrace( ray, fragCoord );
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    Ray	ray = Ray_From( iCameraFrustumLB, iCameraFrustumRB, iCameraFrustumLT, iCameraFrustumRT,
                        iCameraPos, 0.1, fragCoord / iResolution.xy );
    fragColor = RayTrace( ray, fragCoord );
}
