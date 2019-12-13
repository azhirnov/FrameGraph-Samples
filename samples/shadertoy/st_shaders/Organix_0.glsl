// from https://www.shadertoy.com/view/ldByDh

// Created by inigo quilez - iq/2008/2018
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

#define ZERO (min(iFrame,0))

int ihash( in int n )
{
	n=(n<<13)^n;
    return (n*(n*n*15731+789221)+1376312589) & 0x7fffffff;
}

float fhash( in int n )
{
	return float(ihash(n));
}

float noise2f( in vec2 p )
{
	ivec2 ip = ivec2(floor(p));
    vec2  fp = fract(p);
	vec2 u = fp*fp*(3.0-2.0*fp);

    int n = ip.x + ip.y*113;

	float res = mix(mix(fhash(n+(0+113*0)),
                        fhash(n+(1+113*0)),u.x),
                    mix(fhash(n+(0+113*1)),
                        fhash(n+(1+113*1)),u.x),u.y);

    return 1.0 - res*(1.0/1073741824.0);
}

float noise3f( in vec3 p )
{
	ivec3 ip = ivec3(floor(p));
    vec3  fp = fract(p);
	vec3 u = fp*fp*(3.0-2.0*fp);

    int n = ip.x + ip.y*57 + ip.z*113;

	float res = mix(mix(mix(fhash(n+(0+57*0+113*0)),
                            fhash(n+(1+57*0+113*0)),u.x),
                        mix(fhash(n+(0+57*1+113*0)),
                            fhash(n+(1+57*1+113*0)),u.x),u.y),
                    mix(mix(fhash(n+(0+57*0+113*1)),
                            fhash(n+(1+57*0+113*1)),u.x),
                        mix(fhash(n+(0+57*1+113*1)),
                            fhash(n+(1+57*1+113*1)),u.x),u.y),u.z);

    return 1.0 - res*(1.0/1073741824.0);
}

float fbm( in vec3 p )
{
    float f = 0.0;
    float s = 0.5;
    for( int i=ZERO; i<4; i++ )
    {
        f += s*noise3f(p);
        p *= 2.0;
        s *= 0.5;
    }
    return f;
}

vec2 celular( in vec3 p )
{
	ivec3 ip = ivec3(floor(p));
	vec3 f = fract(p); 

    vec2 dmin = vec2( 1.0, 1.0 );

	for( int k=ZERO-1; k<=1; k++ )
	for( int j=ZERO-1; j<=1; j++ )
	for( int i=ZERO-1; i<=1; i++ )
	{
		int nn = (ip.x+i) + 57*(ip.y+j) + 113*(ip.z+k);
        vec3 ra = vec3( fhash(nn     ),
                        fhash(nn+1217),
                        fhash(nn+2513) )/2147483647.0;

        vec3 di = ra + vec3( float(i), float(j), float(k) ) - f;
        float d2 = dot(di,di);

        if( d2<dmin.x )
        {
            dmin.y = dmin.x;
            dmin.x = d2;
        }
        else if( d2<dmin.y )
        {
            dmin.y = d2;
        }
	}
    return sqrt(dmin)*0.25;
}

float map( in vec3 pos, out vec4 suvw )
{
    float dis;

	float mindist = pos.y;

	suvw = vec4(0.0,1.0,0.0,0.0);

    //-----------------------------
	// rocas
    if( pos.y<0.5 )
    {
    	mindist -= 0.262 + 0.03*smoothstep( 0.0, 0.5, fbm(vec3(24.0*pos.x+7.7,2.0*pos.y,24.0*pos.z)) );
    }

    float chdis = 100.0;

    float px = pos.x + 0.10*noise3f( 3.0*pos + vec3(0.00, 0.0, 0.0) );
    float py = pos.y + 0.10*noise3f( 3.0*pos + vec3(0.70, 2.3, 0.5) );
    float pz = pos.z + 0.10*noise3f( 3.0*pos + vec3(1.10, 7.6, 7.2) );

    //-----------------------------
	// champi
    if( pos.y<0.40 )
	{
        float ischampi = noise2f( 2.0*pos.xz+vec2(0.0,7.6) );
        if( ischampi > 0.0 )
        {
            float fx = mod( px+128.0, 0.15 ) - 0.075; 
            float fz = mod( pz+128.0, 0.15 ) - 0.075;

            chdis = sqrt( fx*fx + fz*fz );

            if( chdis<0.10 )
            {
                float cpy = py - (0.22 + 0.10*ischampi);
                float gy = cpy*100.0;
                if( gy<1.0 )
                {

                    gy = 1.0 - gy;
                    float r = 0.07 + 6.0*gy*exp2(-3.0*gy);

                    float ang = atan(fx,fz);
                    float ani = 0.5+0.5*sin(11.0*ang);
                    float gyb = clamp(0.15*gy,0.0,1.0);
                    gyb = pow( gyb, 16.0 );

                    r += 0.15*gyb;
                    r += 0.20*ani*r;

                    float ath = 0.5 + 0.05*sin(3.0*ang);
                    float by = ((gy-3.0)*ath);
                    if( by>0.0 && by<1.0 )
                    {
                        by = by*by;
                        by = by*by;
                        r += by*0.08*ani;
                    }
                    float sgy = clamp(gy*0.50-1.10,0.0,1.0);
                    r += 0.06*sin(gy*12.0)*sgy*(1.0-sgy);

                    r *= 0.0269775390625;

                    dis = chdis - r;
                    if( dis<mindist)
                    {
                        mindist = dis;
                        suvw.x = 1.0;
                        suvw.z = gy;
                        suvw.w = ang;
                    }
                }
            }
        }
	}

    //-----------------------------
	// arboles
    {
        px += 1.10*noise3f( 0.21*pos + vec3(0.00, 0.00, 0.00) );
        py += 1.10*noise3f( 0.21*pos + vec3(0.70, 2.30, 0.50) );
        pz += 1.10*noise3f( 0.21*pos + vec3(1.10, 7.60, 7.20) );

        float fx = mod( px+128.0, 1.0 ) - 0.5;
        float fz = mod( pz+128.0, 1.0 ) - 0.5;
        float ra = 0.05/py;
        dis = fx*fx + fz*fz - ra*ra;

        if( dis<mindist )
        {
            float noao = smoothstep(0.5,0.6,pos.y);
            suvw.y = clamp(chdis*15.0,0.0,1.0)*(1.0-noao) + noao;
            mindist = dis;
            suvw.x = 4.0;
        }
    }
    
    //-----------------------------
	// piedras voladoras
	{
        vec3 ff = mod( vec3(px,py,pz)+vec3(128.0), 0.7 ) - 0.5;
        dis = dot(ff,ff);
        if( dis<(mindist+0.10)*(mindist+0.10))
        {
            suvw[0] = 3.0;
            mindist = sqrt(dis)-0.10;
            suvw[1] = 1.0;
        }
	}


	// tubos!
    if( pos.y>0.40 && pos.y<0.60 )
	{
        float fx = mod( 2.0*px+128.0, 1.0 );
        float fy = 2.0*pos.y;
        float id = floor(2.0*px);
        float cx = 0.25 + 0.25*noise3f( vec3(id*16.0,pos.z*2.0,2.0) );
        float cy = 1.00 + 0.16*noise3f( vec3(pos.z*6.0,id*2.0,0.0) );
        float wi = 0.05 + 0.01*noise3f( vec3(pos.z*48.0,id*16.0,0.0) );
        dis = (fy-cy)*(fy-cy) + (fx-cx)*(fx-cx);
        if( dis<(mindist+wi)*(mindist+wi) )
        {
            suvw.x = 2.0;
            mindist = sqrt(dis)-wi;
            suvw.y = 1.0;
        }
	}

    return mindist;
}

vec3 calcNormal( in vec3 pos )
{
    const float eps = 0.0002;
    vec4 kk;
    vec3 nor;
#if 0
	nor.x = map( vec3(pos.x+eps, pos.y, pos.z), kk ) - map( vec3(pos.x-eps, pos.y, pos.z), kk );
	nor.y = map( vec3(pos.x, pos.y+eps, pos.z), kk ) - map( vec3(pos.x, pos.y-eps, pos.z), kk );
	nor.z = map( vec3(pos.x, pos.y, pos.z+eps), kk ) - map( vec3(pos.x, pos.y, pos.z-eps), kk );
#else
    nor = vec3(0.0);
    for( int i=ZERO; i<4; i++ )
    {
        vec3 e = 0.5773*(2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
        nor += e*map(pos+eps*e, kk);
    }
#endif    
	return normalize( nor );
}


float cast_ray( in vec3 ro, in vec3 rd, in float to, in float tMax, out vec4 suvw )
{
	suvw = vec4(0.0);

	float t = to;
    for( int i=ZERO; i<512; i++ )
	{
        vec3 pos = ro + t*rd;
		float h = map( pos, suvw );
        if( abs(h)<(0.0005*t) || t>tMax) break;
        t += h*0.4;
	}
    return t;
}

float bolasBump( in vec3 p )
{
    vec2 res = celular( vec3(32.0*p.x, 128.0*p.y, 32.0*p.z) );
	return 1.0-2.0*res.x;
}

vec3 addbumpbolas( in vec3 xnor, float bumpa, in vec3 p )
{
#if 0
    float kes = 0.0005;
    float kk = bolasBump( p );
    xnor.x += bumpa*(bolasBump( p+vec3(kes, 0.0, 0.0) )-kk);
    xnor.y += bumpa*(bolasBump( p+vec3(0.0, kes, 0.0) )-kk);
    xnor.z += bumpa*(bolasBump( p+vec3(0.0, 0.0, kes) )-kk);
    return normalize(xnor);
#else
    // klems' trick to prevent the DX compiler from inlining the bump function
    vec4 n = vec4(0);
    for( int i=ZERO; i<4; i++ )
    {
        vec4 s = vec4(p, 0);
        s[i] += 0.0005;
        n[i] = bolasBump(s.xyz);
    }
    return normalize(xnor + bumpa*(n.xyz-n.w));
#endif    
}

float champyBump( in vec3 p )
{
	float f = fbm( 256.0*p );
	return f*f;
}

vec3 addbumpchampy( in vec3 xnor, float bumpa, in vec3 p )
{
#if 0
    float kes = 0.0005;
    float kk = champyBump( p );
    xnor.x += bumpa*(champyBump( p+vec3(kes, 0.0, 0.0 ) )-kk);
    xnor.y += bumpa*(champyBump( p+vec3(0.0, kes, 0.0 ) )-kk);
    xnor.z += bumpa*(champyBump( p+vec3(0.0, 0.0, kes ) )-kk);
    return normalize(xnor);
#else
    // klems' trick to prevent the DX compiler from inlining the bump function
    vec4 n = vec4(0);
    for( int i=ZERO; i<4; i++ )
    {
        vec4 s = vec4(p, 0);
        s[i] += 0.0005;
        n[i] = champyBump(s.xyz);
    }
    return normalize(xnor + bumpa*(n.xyz-n.w));
#endif    
}

vec3 shade( in vec3 pos, in vec3 nor,
			in vec3 ro,  in vec3 rd,
			float matID, float dis, in vec3 uvw )
{
	vec3 xnor = nor;
    vec3 rgb = vec3(0.0);

	// materials
	float fakeao = 1.0;

    // piedras suelo
    if( matID<0.5 )
    {
		vec2 res = celular( vec3(96.0*pos[0], 12.0*pos[1], 96.0*pos[2]) );
		float ce = 5.0*(res[1]-res[0]);
        rgb = vec3( sqrt(ce) );
		float fb = fbm( 8.0*pos );
        vec3 cosa2 = vec3( 0.60, 0.50, 0.40 );
        rgb = mix( rgb, cosa2, 0.5 + 0.5*fb );

        rgb *= 0.21 + 0.14*fbm( 256.0*pos );

		xnor = addbumpchampy( xnor, -3.0*4.0, pos );
    }
    // champis
    else if( matID<1.5 )
    {
        float fb = fbm( 8.0*pos );
	
		vec2 res = celular( vec3(768.0*pos.x, 12.0*pos.y, 768.0*pos.z) );
		float am = 1.0-smoothstep(0.02,0.15,res.x);
		float cc = smoothstep(-0.75,-0.10,nor.y);
		am *= cc;
		am = 1.0-am;


        vec3 arriba = mix( vec3( 0.50, 0.50, 0.50 ), vec3( 0.34, 0.26, 0.18 ), am );

		cc = 1.0-smoothstep(0.40,1.20,uvw.y);

		rgb = vec3(0.45,0.40,0.30);

		rgb *= 1.0 + 0.75*fbm(192.0*pos);

		xnor = addbumpchampy( xnor, 2.0*clamp(1.75-0.40*uvw[1],0.0,1.0), vec3(0.25*uvw.z, 1.0*pos.y, 0.0) );

		rgb = mix( rgb, arriba, cc );
		float spe = clamp(-dot(rd,xnor ),0.0,1.0);
		spe = 1.0 - spe;
		spe = spe*spe;
		rgb += vec3(spe*0.50);

		fakeao = 1.0-clamp((uvw.y-5.25)*0.75,0.0,1.0);
		
		fakeao *= 1.0 - 0.25*(1.0-sin(11.0*uvw.z))*smoothstep(3.0,4.5,uvw.y);
		float ath = 0.5 + 0.05*sin(3.0*uvw.z);
		float by = (uvw.y-3.0)*ath;
		if( by>1.0 ) 
		fakeao *= 0.20 + 0.80*clamp((by-1.0)*4.0,0.0,1.0);
    }
    // ramas
    else if( matID<2.5 )
    {
		vec2 res = celular( vec3(96.0*pos.x,256.0*pos.y, 96.0*pos.z) );
		float ce = 0.20 + 52.0*res.y*res.y*res.y;

        rgb = vec3(0.80, 0.75, 0.65 ) * ce;

		float spe = clamp( 1.0+dot( rd, nor ), 0.0, 1.0 );
		rgb += vec3(0.30, 0.15, 0.10) * spe;

        xnor = addbumpchampy( xnor, -0.80*-3.0, vec3(1.0*pos.x, 1.0*pos.y, 0.5*pos.z) );
        xnor = addbumpchampy( xnor,  0.60*-3.0, pos*0.5 );

        rgb += vec3(0.07, 0.15,0.00) * (1.0-res.x*4.0);
        rgb += vec3(0.25, 0.22,0.19);

		// verdin
        float fb = smoothstep(-0.20,0.20, 0.30+fbm( vec3(158.0*pos.x, 158.0*pos.y, 1.0*pos.z) ));
		float var = 0.40*noise3f( vec3(4.0*pos.xy,64.0*pos.z));
		fb *= smoothstep(-0.10,0.10,xnor.y+var);
		rgb *= vec3(1.0-0.70*fb,1.0-0.60*fb,1.0-0.70*fb);
	}   
    // aliens
    else if( matID<3.5 )
    {
		float ks = 0.25;
		float ce = 0.00;
		float ss = 0.50;
		for( int i=ZERO; i<8; i++ )
		{
		    vec2 res = celular( 74.0*pos*ks );
		    ce+=ss*res.x;
		    ss*=0.60;
		    ks*=2.0;
		}
		ce = clamp(3.5*ce-0.30, 0.0, 1.0);
        
        rgb = vec3(0.20, 0.10, 0.0 ) + vec3(0.80*ce);

		xnor = addbumpbolas( xnor, -6.0, pos );
        
        float fb = 0.15*clamp( fbm( 8.0*pos ), 0.0, 1.0 );
		rgb.x -= fb;
		rgb.z -= fb;

        fb = fbm( vec3(128.0*pos.x, 4.0*pos.y, 128.0*pos.z) );
		rgb *= 0.75+0.25*fb;

        float ni = smoothstep( -0.10, 0.20, nor.y );

        rgb = mix( rgb, vec3(0.09, 0.05, 0.04), ni );

        // fres
		float spe = clamp( -dot( rd, xnor ), 0.0, 1.0 );
		spe = 1.0 - spe;
		rgb += vec3(spe*spe*0.78);
    }   
    // suelo y arboles
    else if( matID<4.5)
    {
        vec2 res = celular( vec3(96.0*pos.x, 12.0*pos.y, 96.0*pos.z) );
		float ce = 20.0*(res.y-res.x);
        rgb = vec3(0.50, 0.45, 0.40 ) * sqrt(ce);

        rgb = mix( rgb, vec3(0.60, 0.50, 0.40), 0.5 + 0.5*fbm( 8.0*pos ) );
        rgb *= (0.60 + 0.40*fbm( 256.0*pos ));

		ce = fbm( vec3(1024.0*pos.x, 512.0*pos.y, 1024.0*pos.z) );
        float ni = smoothstep( 0.0, 0.30, nor.y );
		vec3 verde2 = vec3( 0.06, 0.05, 0.04 );
        vec3 verde1 = vec3( 0.10, 0.09, 0.04 );
        vec3 verde = mix( verde1, verde2, ce );

		ce = fbm( vec3(1024.0*pos.x+123.789, 512.0*pos.y+71.71, 1024.0*pos.z) );
		float ce2 = smoothstep(-0.05,0.0, fbm( vec3(32.0*pos.x+123.789, 32.0*pos.y+71.71, 32.0*pos.z)) );
        rgb = mix( rgb, verde, ce2*ni*smoothstep(-0.10,0.0,ce) );

		rgb *= 0.60;
		float spe = clamp(-dot(rd,xnor),0.0,1.0);
		spe = 1.0-spe;
		spe = spe*spe;
		spe *= 0.20;

	    //--------------

	    float ks = tan(0.75*pos.z);
	    float kkHier = 0.5 + 0.5*fbm( vec3(4096.0*pos.x, 128.0*pos.y, 4096.0*pos.z*ks) );
	    float hh = 0.75*clamp(-rd.y,0.0,1.0);
	    kkHier = smoothstep( hh, 1.0, kkHier );

        vec3 cesped = vec3(0.10,0.05,0.00);
        cesped += vec3(0.22,0.24,0.25)*(0.5 + 0.5*fbm(64.0*pos));
		cesped += vec3(0.05,0.08,0.10)*(0.5 + 0.5*fbm(vec3(1024.0*pos.x,1024.0*pos.y,1024.0*pos.z)));
		cesped *= kkHier;
		cesped *= vec3(1.10,1.20,1.00);

	    float ce3 = smoothstep(-0.20,-0.10, fbm(32.0*pos+vec3(0.0,11.71,0.0)) );
	    rgb = mix( rgb, cesped, ce3*ni );

        rgb += vec3(spe*(1.0-ni) );
    }
	else
	{
		rgb = vec3( 0.0 );
	}

	float  ao = xnor.y*0.5 + 0.5;
    ao +=  0.25*(1.0 - 1.0/(1.0+pos.y));
	ao += xnor.x*0.20;
	ao *= (uvw.x*0.90+0.10);
	ao *= fakeao;

    vec3 lig = vec3( 0.80, 0.50, 0.10 );

	float ffs = smoothstep( -0.10, 0.10, fbm( vec3(32.0*pos.x,32.0*pos.z,1.70) ) );
	ffs *= clamp( 4.0*dot(xnor,lig), 0.0, 1.0 );
	ffs *= clamp( 4.0*dis-0.10, 0.0, 1.0  );

    vec3 aoc = vec3( 0.67, 0.71, 0.75 );

    aoc = aoc * ao;
    aoc += vec3(1.30, 1.25, 1.20) * ffs*ao;
	
    // lighting
    rgb *= aoc;

    // fog
	rgb *= 1.0/(1.0+0.85*dis);
	rgb += vec3(0.0496, 0.0528, 0.0544) * dis;

    // sun
	float sun = clamp(dot(rd,lig),0.0,1.0);
	sun = sun*sun;
	sun = sun*sun;
	float tsun = sun*0.08;
	sun = sun*sun;
	sun = sun*sun;
	tsun += sun*0.70;
    rgb += vec3(1.30, 1.20, 1.0) * tsun;

    return rgb;
}


vec3 colorCorrect( vec3 rgb, in vec2 xy )
{
	rgb *= max( 4.0-(rgb.x+rgb.y+rgb.z), 1.0 );
	
    rgb = clamp( rgb-0.1, 0.0, 1.0 );

    // vigneting
	rgb *= 0.75 + xy.x*(1.0-xy.x);
    
    return rgb;
}

vec3 calcpixel( in vec2 px )
{
    vec2 sxy = -1.0 + 2.0*px;

    vec2 xy = sxy*vec2(1.75,1.0);
	float r2 = xy.x*xy.x*0.32 + xy.y*xy.y;
    vec2 dxy = xy*(7.0-sqrt(37.5-11.5*r2))/(r2+1.0);

	vec3 rayDir = vec3( dxy.x*0.98 - dxy.y*0.17,
                        dxy.x*0.17 + dxy.y*0.98 + 0.60,
                        1.0 );

	rayDir = normalize( rayDir );

    vec3 rayPos = vec3( 0.35+0.02*sin(6.2831*iTime/20.0), 0.30, 0.00 );


    vec3 rgb = vec3(0.0);
    
    // ray march scene
    vec4 suvw;
    float t = cast_ray( rayPos, rayDir, 0.01, 15.0, suvw );
    if( t>15.0 ) { suvw.x = 512.0; }

    vec3 pos = rayPos + t*rayDir;
    vec3 nor = calcNormal( pos );
        
    // shade
    rgb = shade( pos, nor, rayPos, rayDir, suvw.x, t, suvw.yzw );
  
    // color correct
    rgb = colorCorrect( rgb, 0.5+0.5*sxy );

    return rgb;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord / iResolution.xy;
    vec3 col = calcpixel( uv );
	fragColor = vec4(col,1.0);
}