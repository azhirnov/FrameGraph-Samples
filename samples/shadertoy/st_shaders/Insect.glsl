// from https://www.shadertoy.com/view/Mss3zM

// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

float hash( float n )
{
    return fract(sin(n)*158.5453);
}

float noise( in float x )
{
    float p = floor(x);
    float f = fract(x);

    f = f*f*(3.0-2.0*f);

    return mix( hash(p+0.0), hash(p+1.0),f);
}

float noise( in vec2 x )
{
    vec2 p = floor(x);
    vec2 f = fract(x);

    f = f*f*(3.0-2.0*f);
    float a = textureLod(iChannel1,(p+vec2(0.5,0.5))/64.0, 0.0).x;
	float b = textureLod(iChannel1,(p+vec2(1.5,0.5))/64.0, 0.0).x;
	float c = textureLod(iChannel1,(p+vec2(0.5,1.5))/64.0, 0.0).x;
	float d = textureLod(iChannel1,(p+vec2(1.5,1.5))/64.0, 0.0).x;
    float res = mix(mix( a, b,f.x), mix( c, d,f.x),f.y);

	return 2.0*res;
}




vec3 texturize( sampler2D sa, vec3 p, vec3 n, in vec3 gx, in vec3 gy )
{
	vec3 x = textureGrad( sa, p.yz, gx.yz, gy.yz ).xyz;
	vec3 y = textureGrad( sa, p.zx, gx.zx, gy.zx ).xyz;
	vec3 z = textureGrad( sa, p.xy, gx.xy, gy.xy ).xyz;

	return x*abs(n.x) + y*abs(n.y) + z*abs(n.z);
}

#define ZERO (min(iFrame,0))

//----------------------------------------------------------------

float terrainSoft( vec2 x )
{
	x += 100.0;
	x *= 0.6;

	vec2 p = floor(x);
    vec2 f = fract(x);

    f = f*f*(3.0-2.0*f);
    float a = textureLod(iChannel0,0.0+(p+vec2(0.5,0.5))/1024.0,0.0).x;
	float b = textureLod(iChannel0,0.0+(p+vec2(1.5,0.5))/1024.0,0.0).x;
	float c = textureLod(iChannel0,0.0+(p+vec2(0.5,1.5))/1024.0,0.0).x;
	float d = textureLod(iChannel0,0.0+(p+vec2(1.5,1.5))/1024.0,0.0).x;
	float r = mix(mix( a, b,f.x), mix( c, d,f.x), f.y);
	
	return 12.0*r;

}

float terrain( vec2 x )
{
	float f = terrainSoft( x );

	float h = smoothstep( 0.4, 0.8, noise( 2.0*x ) );
	f -= 0.2*h;

    float d = noise( 35.0*x.yx*vec2(0.1,1.0) );
	f += 0.003*d * h*h;

	return f;
}


vec2 sdSegment2( vec3 a, vec3 b, vec3 p, float ll )
{
	vec3 pa = p - a;
	vec3 ba = b - a;
	float h = clamp( dot(pa,ba)*ll, 0.0, 1.0 );
	
	return vec2( length( pa - ba*h ), h );
}

vec3 solve( vec3 p, float l1, float l2, vec3 dir )
{
	vec3 q = p*( 0.5 + 0.5*(l1*l1-l2*l2)/dot(p,p) );
	
	float s = l1*l1 - dot(q,q);
	s = max( s, 0.0 );
	q += sqrt(s)*normalize(cross(p,dir));
	
	return q;

}

vec3 solve( vec3 a, vec3 b, float l1, float l2, vec3 dir )
{
	return a + solve( b-a, l1, l2, dir );
}

float smin( float a, float b )
{
    float k = 0.1;
	float h = clamp( 0.5 + 0.5*(b-a)/k, 0.0, 1.0 );
	return mix( b, a, h ) - k*h*(1.0-h);
}

struct Monster
{
	vec3 center;
	vec3 mww;
	vec3 ne[6];
	vec3 f0b[6];
};

	
Monster monster;

vec2 sdMonster( in vec3 p )
{
	vec3 q = p - monster.center;
	
	if( dot(q,q)>25.0 ) return vec2(32.0);
	
	vec3 muu = vec3(1.0,0.0,0.0);
	vec3 mvv = normalize( cross(monster.mww,muu) );

	q = vec3( q.x, dot(mvv,q), dot(monster.mww,q) );

    // body
	float ab = (0.5 + 0.5*cos( 1.0 + 40.0*pow(0.5-0.5*q.z,2.0) ))*(0.5+0.5*q.z);
	float d1 = length( q*vec3(1.5,2.2,1.0) ) - 1.0 - 0.3*ab;
	d1 += 0.03*sin(20.0*q.z)*(0.5+0.5*clamp(2.0*q.y,0.0,1.0));
	float f = 0.5 - 0.5*q.z;
	d1 += f*0.04*sin(40.0*q.y)*sin(40.0*q.x)*sin(40.0*q.z)*clamp(2.0*q.y+0.5,0.0,1.0);
	float ho = 1.0-clamp( 3.0*abs(q.x), 0.0, 1.0 );
	d1 += 0.1*(1.0-sqrt(1.0-ho*ho))*smoothstep( 0.0,0.1,-q.z );
	
	// legs
	for( int i=ZERO; i<6; i++ )
	{
	    float s = -sign( float(i)-2.5 );
		float h = mod( float(i), 3.0 )/3.0;
		
		vec3 bas = monster.center + muu*s*0.5 + monster.mww*1.0*(h-0.33) ;

		vec3 n1 = monster.ne[i];
		vec2 hh = sdSegment2( bas, n1, p, 1.0/(1.6*1.6) );
		d1 = smin( d1, hh.x-mix(0.15,0.05,hh.y) + 0.05*sin(6.2831*hh.y) );
		hh = sdSegment2( n1, monster.f0b[i], p, 1.0/(1.2*1.2) );
		d1 = smin( d1, hh.x-mix(0.06,0.02,hh.y) + 0.01*cos(2.0*6.2831*hh.y) );
	}
	
	
	vec2 res = vec2( 0.5*d1, 1.0 );

	// eyes
	q.x = abs(q.x);
	float d3 = length( q - vec3(0.3,0.05,0.9) ) - 0.3;
	if( d3<res.x ) res = vec2( d3, 0.0 );

	return res;
}


vec2 map( in vec3 p )
{
    // monster
    vec2 res = sdMonster( p );

    // terrain
	float d2 = 0.5*(p.y - terrain(p.xz));
	if( d2<res.x ) res=vec2(d2,2.0);
	
	return res;
}

vec3 intersect( in vec3 ro, in vec3 rd )
{
	float mind = 0.1;
	float maxd = 70.0;
	
    float t = mind;
	float d = 0.0;
    float m = 1.0;
    for( int i=0; i<150; i++ )
    {
	    vec2 res = map( ro+rd*t );
        float h = res.x;
		d = res.y;
		m = res.y;
        if( abs(h)<(0.0005*t)||t>maxd ) break;
        t += h;
    }

    if( t>maxd ) m=-1.0;
    return vec3( t, d, m );
}

vec3 calcNormal( in vec3 pos )
{
#if 0    
    vec3 eps = vec3(0.002,0.0,0.0);

	return normalize( vec3(
           map(pos+eps.xyy).x - map(pos-eps.xyy).x,
           map(pos+eps.yxy).x - map(pos-eps.yxy).x,
           map(pos+eps.yyx).x - map(pos-eps.yyx).x ) );
#else
    // inspired by klems - a way to prevent the compiler from inlining map() 4 times
    vec3 n = vec3(0.0);
    for( int i=ZERO; i<4; i++ )
    {
        vec3 e = 0.5773*(2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
        n += e*map(pos+e*0.002).x;
    }
    return normalize(n);
#endif    
    }

float softshadow( in vec3 ro, in vec3 rd, float mint, float k )
{
    float res = 1.0;
    float t = mint;
	float h = 1.0;
    for( int i=0; i<48; i++ )
    {
        h = map(ro + rd*t).x;
        res = min( res, smoothstep(0.0,1.0,k*h/t) );
		t += clamp( h, 0.025, 1.0 );
		if( h<0.001 ) break;
    }
    return clamp(res,0.0,1.0);
}


vec3 lig = normalize(vec3(-1.0,0.4,0.2));

vec3 path( float t )
{
    vec3 pos = vec3( 0.0 );
    pos.z += t*0.4;
	pos.y = 1.0 + terrainSoft( pos.xz );
	return pos;
}


// compute screen space derivatives of positions analytically without dPdx()
void calcDpDxy( in vec3 ro, in vec3 rd, in vec3 rdx, in vec3 rdy, in float t, in vec3 nor, out vec3 dpdx, out vec3 dpdy )
{
    dpdx = t*(rdx*dot(rd,nor)/dot(rdx,nor) - rd);
    dpdy = t*(rdy*dot(rd,nor)/dot(rdy,nor) - rd);
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 q = fragCoord.xy / iResolution.xy;
    vec2 p = -1.0 + 2.0 * q;
    p.x *= iResolution.x/iResolution.y;
    vec2 m = vec2(0.5);
	if( iMouse.z>0.0 ) m = iMouse.xy/iResolution.xy;


    //-----------------------------------------------------
    // animate
    //-----------------------------------------------------
	
	float ctime = 15.0 + iTime;

	// mobe body
	float atime = 2.0*ctime;
	float ac = noise( 0.5*ctime );
	atime += 4.0*ac;
    monster.center = path( atime );
	vec3 centerN = path( atime+2.0 );
	monster.mww = normalize( centerN - monster.center );
    monster.center.y -= 0.25;

	
	// move legs
	for( int i=0; i<6; i++ )
	{
		float s = -sign( float(i)-2.5 );
		float h = mod( float(i), 3.0 )/3.0;

		float z = 0.5*atime + 1.0*h + 0.25*s;
		float iz = floor(z);
		float fz = fract(z);
	    float az = clamp((fz-0.66)/0.34,0.0,1.0);
		
		vec3 fo = vec3(s*1.5, 0.7*az*(1.0-az), (iz + az + (h-0.3)*4.0)*0.4*2.0 );
		fo.y += terrain( fo.xz );
        monster.f0b[i] = fo;
		
		vec3 ba = monster.center + vec3(1.0,0.0,0.0)*s*0.5 + monster.mww*1.0*(h-0.33) ;

		monster.ne[i] = solve( ba, fo, 1.6, 1.2, s*vec3(0.0,0.0,-1.0) );
	}

	
    //-----------------------------------------------------
    // camera
    //-----------------------------------------------------
	
    // follow the monster
	float an = 0.0 + 0.1*ctime - 6.28*m.x;
	float cr = 0.3*cos(0.2*ctime);
    vec3 ro = monster.center + vec3(4.0*sin(an),0.2,4.0*cos(an));
    vec3 ta = monster.center;
	ro.y = 0.5 + terrainSoft( ro.xz );
	
    // shake
	ro += 0.04*sin(4.0*ctime*vec3(1.1,1.2,1.3)+vec3(3.0,0.0,1.0) );
	ta += 0.04*sin(4.0*ctime*vec3(1.7,1.5,1.6)+vec3(1.0,2.0,1.0) );
	
    // camera matrix
    vec3 ww = normalize( ta - ro );
    vec3 uu = normalize( cross(ww,vec3(sin(cr),cos(cr),0.0) ) );
    vec3 vv = normalize( cross(uu,ww));
	
    // barrel distortion	
    float r2 = p.x*p.x*0.32 + p.y*p.y;
    p *= (7.0-sqrt(37.5-11.5*r2))/(r2+1.0);
	
	// create view ray
	vec3 rd = normalize( p.x*uu + p.y*vv + 3.0*ww );
    
    // ray differentials
    vec2 px = (-iResolution.xy+2.0*(fragCoord.xy+vec2(1.0,0.0)))/iResolution.y;
    r2 = px.x*px.x*0.32 + px.y*px.y;
    px *= (7.0-sqrt(37.5-11.5*r2))/(r2+1.0);
    vec2 py = (-iResolution.xy+2.0*(fragCoord.xy+vec2(0.0,1.0)))/iResolution.y;
    r2 = py.x*py.x*0.32 + py.y*py.y;
    py *= (7.0-sqrt(37.5-11.5*r2))/(r2+1.0);
    vec3 rdx = normalize( px.x*uu + px.y*vv + 3.0*ww );
    vec3 rdy = normalize( py.x*uu + py.y*vv + 3.0*ww );
    

    //-----------------------------------------------------
	// render
    //-----------------------------------------------------
	float nds = clamp(dot(rd,lig),0.0,1.0);
	vec3 bgc = vec3(0.9+0.1*nds,0.95+0.05*nds,1.0)*(0.7 + 0.3*rd.y)*0.98;
    vec3 col = bgc;

	// raymarch
    vec3 tmat = intersect(ro,rd);
    if( tmat.z>-0.5 )
    {
        float t = tmat.x;
        
        // geometry
        vec3 pos = ro + t*rd;
        vec3 nor = calcNormal(pos);
		vec3 ref = reflect( rd, nor );

        // derivatives        
        vec3 dpdx, dpdy;
        calcDpDxy( ro, rd, rdx, rdy, t, nor, dpdx, dpdy );

    
        // materials
        float mocc = 1.0;
		vec4 mate = vec4(0.0);
		
		// eyes
		if( tmat.z<0.5 )
		{
			mate = 3.0*vec4(0.002,0.002,0.002,10.0);
			mate.xyz *= 0.8 + 0.2*sin(2.0*ref);
			mate.w *= 0.8 + 0.2*sin(20.0*ref.x)*sin(20.0*ref.y)*sin(20.0*ref.z);
			mate.xyz += 0.005*vec3(1.0,0.1,0.0);
		}
		// body
		else if( tmat.z<1.5 )
		{
            // do shading in mosnter space			
			vec3 q = pos - monster.center;
			vec3 muu = vec3(1.0,0.0,0.0);
			vec3 mvv = normalize( cross(monster.mww,muu) );
			q = vec3( q.x, dot(mvv,q), dot(monster.mww,q) );
			vec3 n = vec3( nor.x, dot(mvv,nor), dot(monster.mww,nor) );

            vec3 dqdx = vec3( dpdx.x, dot(mvv,dpdx), dot(monster.mww,dpdx) );
            vec3 dqdy = vec3( dpdy.x, dot(mvv,dpdy), dot(monster.mww,dpdy) );

			q.x = abs( q.x );
            // base color			
            mate.xyz = 0.3*vec3(1.0,0.7,0.4);			
			mate = mix( mate, 0.3*vec4(1.0,0.9,0.6,1.0), smoothstep(0.0,1.0,-n.y) );
			
            // texture
			mate.xyz *= 0.4+0.6*sqrt(smoothstep(0.0,0.7,texturize(iChannel0,0.5*q,n,0.5*dqdx, 0.5*dqdy).xyz));
			
            // stripes
			float ss = smoothstep( 0.5, 0.8, textureGrad( iChannel1, 0.5*q.xz*vec2(1.0,0.1), 0.5*dqdx.xz, 0.5*dqdy.xz*0.1 ).x )*smoothstep( 0.0, 0.3, nor.y );
			mate.xyz += 0.15*ss;

			// color adjustment
			mate.w = 0.5;
			mate.xyz *= 0.8;
			mate.xyz *= 0.2+2.3*mate.xzy*vec3(1.0,1.6,1.4);
			
            // occlusion			
			float ho = 1.0-clamp( 5.0*abs(q.x), 0.0, 1.0 );
            mocc *= 1.0-(1.0-sqrt(1.0-ho*ho))*smoothstep( 0.0,0.1,-q.z );
			
			// bump
			vec3 bnor = -1.0 + 2.0*texturize( iChannel2, 3.0*q, n, 3.0*dqdx, 3.0*dqdy ).xyz;
			bnor.y = abs(bnor.y);
			nor = normalize( nor + (1.0-ss)*0.2*normalize(bnor) );

		}
        // terrain		
		else if( tmat.z<2.5 )
		{
			mate = vec4(1.0, 0.9, 0.5, 0.0);
            float nn =  noise( 2.0*pos.xz );

            mate.xyz = mix( 0.7*mate.xyz, mate.xyz*0.65*vec3(0.8,0.9,1.0), 1.0-smoothstep( 0.4, 0.9, nn ) );
			mate.xyz *= 0.45;

			vec3 ff  = 0.05+0.95*textureGrad( iChannel0, 0.08*pos.xz, 0.08*dpdx.xz, 0.08*dpdy.xz ).xyz;
			     ff *= 0.05+0.95*textureGrad( iChannel0, 0.52*pos.xz, 0.52*dpdx.xz, 0.52*dpdy.xz ).xyz;
			     ff *= 0.05+0.95*textureGrad( iChannel0, 4.03*pos.xz, 4.03*dpdx.xz, 4.03*dpdy.xz ).xyz;
			
			float aa = mix( 1.0, 0.3, smoothstep( 0.65, 0.8, nn ) );
            mate.xyz *= (1.0-aa) + aa*sqrt(ff)*3.0;
			
            float d = smoothstep( 0.0, 0.5, abs(nn-0.75) );
            mate.xyz *= 0.6+0.4*d;
            d = smoothstep( 0.0, 0.2, abs(nn-0.75) );
            mocc *= 0.7+0.3*d;
			mocc *= 0.03+0.97*pow( clamp( 0.5*length( pos.xz - monster.center.xz ), 0.0, 1.0 ), 2.0 );

			vec3 bnor = -1.0 + 2.0*textureGrad( iChannel2, 3.0*pos.xz, 3.0*dpdx.xz, 3.0*dpdy.xz ).xyz;
			bnor.y = abs(bnor.y);
			nor = normalize( nor + 0.1*normalize(bnor) );
		}


		// lighting
		float occ = (0.5 + 0.5*nor.y)*mocc;
        float amb = 0.5 + 0.5*nor.y;
        float dif = max(dot(nor,lig),0.0);
        float bac = max(dot(nor,-lig),0.0);
		float sha = 0.0; if( dif>0.0 ) sha=softshadow( pos, lig, 0.05, 32.0 );
        float fre = pow( clamp( 1.0 + dot(nor,rd), 0.0, 1.0 ), 2.0 );
        float spe = pow( clamp( dot(lig,reflect(rd,nor)), 0.0, 1.0), 6.0 );
		
		// lights
		vec3 lin = vec3(0.0);
        lin += 3.0*dif*vec3(1.50,1.00,0.65)*pow(vec3(sha),vec3(1.0,1.2,1.5));
		lin += 5.0*amb*vec3(0.12,0.11,0.10)*occ;
		lin += 3.0*bac*vec3(0.30,0.20,0.15)*occ;
        lin += 1.0*fre*vec3(0.50,0.50,0.50)*occ*15.0*mate.w*(0.05+0.95*dif*sha);
		lin += 1.0*spe*vec3(1.0)*6.0*occ*mate.w*dif*sha;

		// surface-light interacion
		col = mate.xyz * lin;

		// fog		
		col = mix( col, 0.8*bgc, clamp(1.0-1.2*exp(-0.013*tmat.x ),0.0,1.0) );
    }
	else
	{
        // sun		
	    vec3 sun = vec3(1.0,0.8,0.5)*pow( nds, 24.0 );
	    col += sun;
	}

    // sun scatter
	col += 0.4*vec3(0.2,0.14,0.1)*pow( nds, 7.0 );


    //-----------------------------------------------------
	// postprocessing
    //-----------------------------------------------------
    // gamma
	col = pow( col, vec3(0.45) );

    // desat
    col = mix( col, vec3(dot(col,vec3(0.333))), 0.2 );

    // tint
	col *= vec3( 1.0, 1.0, 1.0*0.9);

	// vigneting
    col *= 0.2 + 0.8*pow( 16.0*q.x*q.y*(1.0-q.x)*(1.0-q.y), 0.1 );

    // fade in	
	col *= smoothstep( 0.0, 2.0, iTime );

    fragColor = vec4( col, 1.0 );
}
