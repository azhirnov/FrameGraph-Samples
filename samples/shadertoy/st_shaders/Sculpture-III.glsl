// from https://www.shadertoy.com/view/XtjSDK

// Created by inigo quilez - iq/2015
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.


#if HW_PERFORMANCE==0
#define AA 1
#else
#define AA 2  // Set AA to 1 if your machine is too slow
#endif


float hash1( float n )
{
    return fract(sin(n)*43758.5453123);
}

float hash1( in vec2 f ) 
{ 
    return fract(sin(f.x+131.1*f.y)*43758.5453123); 
}


const float PI = 3.1415926535897932384626433832795;
const float PHI = 1.6180339887498948482045868343656;

vec3 forwardSF( float i, float n) 
{
    float phi = 2.0*PI*fract(i/PHI);
    float zi = 1.0 - (2.0*i+1.0)/n;
    float sinTheta = sqrt( 1.0 - zi*zi);
    return vec3( cos(phi)*sinTheta, sin(phi)*sinTheta, zi);
}

vec4 grow = vec4(1.0);

vec3 mapP( vec3 p )
{
    p.xyz += 1.000*sin(  2.0*p.yzx )*grow.x;
    p.xyz += 0.500*sin(  4.0*p.yzx )*grow.y;
    p.xyz += 0.250*sin(  8.0*p.yzx )*grow.z;
    p.xyz += 0.050*sin( 16.0*p.yzx )*grow.w;
    return p;
}

float map( vec3 q )
{
    vec3 p = mapP( q );
    float d = length( p ) - 1.5;
	return d * 0.05;
}

float intersect( in vec3 ro, in vec3 rd )
{
	const float maxd = 7.0;

	float precis = 0.001;
    float h = 1.0;
    float t = 1.0;
    for( int i=0; i<1256; i++ )
    {
        if( (h<precis) || (t>maxd) ) break;
	    h = map( ro+rd*t );
        t += h;
    }

    if( t>maxd ) t=-1.0;
	return t;
}

vec3 calcNormal( in vec3 pos )
{
    vec3 eps = vec3(0.005,0.0,0.0);
	return normalize( vec3(
           map(pos+eps.xyy) - map(pos-eps.xyy),
           map(pos+eps.yxy) - map(pos-eps.yxy),
           map(pos+eps.yyx) - map(pos-eps.yyx) ) );
}

float calcAO( in vec3 pos, in vec3 nor, in vec2 pix )
{
	float ao = 0.0;
    for( int i=0; i<64; i++ )
    {
        vec3 ap = forwardSF( float(i), 64.0 );
		ap *= sign( dot(ap,nor) ) * hash1(float(i));
        ao += clamp( map( pos + nor*0.05 + ap*1.0 )*32.0, 0.0, 1.0 );
    }
	ao /= 64.0;
	
    return clamp( ao*ao, 0.0, 1.0 );
}

float calcAO2( in vec3 pos, in vec3 nor, in vec2 pix )
{
	float ao = 0.0;
    for( int i=0; i<32; i++ )
    {
        vec3 ap = forwardSF( float(i), 32.0 );
		ap *= sign( dot(ap,nor) ) * hash1(float(i));
        ao += clamp( map( pos + nor*0.05 + ap*0.2 )*100.0, 0.0, 1.0 );
    }
	ao /= 32.0;
	
    return clamp( ao, 0.0, 1.0 );
}

vec4 texCube( sampler2D sam, in vec3 p, in vec3 n, in float k )
{
	vec4 x = texture( sam, p.yz );
	vec4 y = texture( sam, p.zx );
	vec4 z = texture( sam, p.xy );
    vec3 w = pow( abs(n), vec3(k) );
	return (x*w.x + y*w.y + z*w.z) / (w.x+w.y+w.z);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	#define ZERO (min(iFrame,0))
    
    vec3 tot = vec3(0.0);
    #if AA>1
    for( int m=ZERO; m<AA; m++ )
    for( int n=ZERO; n<AA; n++ )
    {
        // pixel coordinates
        vec2 o = vec2(float(m),float(n)) / float(AA) - 0.5;
        vec2 p = (2.0*(fragCoord+o)-iResolution.xy)/iResolution.y;
		vec2 q = (fragCoord+o)/iResolution.xy;
#else    
        vec2 p = (2.0*fragCoord-iResolution.xy)/iResolution.y;
		vec2 q = fragCoord/iResolution.xy;
#endif

    
        grow = smoothstep( 0.0, 1.0, (iTime-vec4(0.0,1.0,2.0,3.0))/3.0 );


        //-----------------------------------------------------
        // camera
        //-----------------------------------------------------

        float an = 1.1 + 0.05*(iTime-10.0);

        vec3 ro = vec3(4.5*sin(an),1.0,4.5*cos(an));
        vec3 ta = vec3(0.0,0.2,0.0);
        // camera matrix
        vec3 ww = normalize( ta - ro );
        vec3 uu = normalize( cross(ww,vec3(0.0,1.0,0.0) ) );
        vec3 vv = normalize( cross(uu,ww));
        // create view ray
        vec3 rd = normalize( p.x*uu + p.y*vv + 1.5*ww );


        //-----------------------------------------------------
        // render
        //-----------------------------------------------------

        vec3 col = vec3(0.07)*clamp(1.0-length(q-0.5),0.0,1.0);

        // raymarch
        float t = intersect(ro,rd);

        if( t>0.0 )
        {
            vec3 pos = ro + t*rd;
            vec3 nor = calcNormal(pos);
            vec3 ref = reflect( rd, nor );
            vec3 sor = nor;

            vec3 q = mapP( pos );
            float occ = calcAO( pos, nor, fragCoord ); occ = occ*occ;

            // materials
            col = vec3(0.04);
            float ar = clamp(1.0-0.7*length(q-pos),0.0,1.0);
            col = mix( col, vec3(2.1,2.0,1.2), ar);
            col  *= 0.3;          
            col *= mix(vec3(1.0,0.4,0.3), vec3(0.8,1.0,1.3), occ);
            float occ2 = calcAO2( pos, nor, fragCoord );


            col *= 1.0*mix( vec3(2.0,0.4,0.2), vec3(1.0), occ2*occ2*occ2 );
            float ks = texCube( iChannel0, pos*1.5, nor, 4.0 ).x;
            ks = 0.5 + 1.0*ks;
            ks *= (1.0-ar);

            // lighting
            float sky = 0.5 + 0.5*nor.y;
            float fre = clamp( 1.0 + dot(nor,rd), 0.0, 1.0 );
            float spe = pow(max( dot(-rd,nor),0.0),8.0);
            // lights
            vec3 lin  = vec3(0.0);
                 lin += 3.0*vec3(0.7,0.80,1.00)*sky*occ;
                 lin += 1.0*fre*vec3(1.2,0.70,0.60)*(0.1+0.9*occ);
            col += 0.3*ks*4.0*vec3(0.7,0.8,1.00)*smoothstep(0.0,0.2,ref.y)*(0.05+0.95*pow(fre,5.0))*(0.5+0.5*nor.y)*occ;
            col += 4.0*ks*1.5*spe*occ*col.x;
            col += 2.0*ks*1.0*pow(spe,8.0)*occ*col.x;
            col = col * lin;

            // dust
            col = mix( col, 0.2*fre*fre*fre+0.6*vec3(0.6,0.55,0.5)*sky*(0.8+0.4*texCube( iChannel0, pos*8.0, nor, 4.0 ).xyz), 0.6*smoothstep(0.3,0.7,nor.y)*sqrt(occ) );

            col *= 2.6*exp(-0.2*t);
        }

        col = pow(col,vec3(0.4545));
        
        tot += col;
#if AA>1
    }
    tot /= float(AA*AA);
#endif

    tot = pow( tot, vec3(1.0,1.0,1.4) ) + vec3(0.0,0.02,0.14);
    
    tot += (1.0/255.0)*hash1( fragCoord );
    
    fragColor = vec4( tot, 1.0 );
}
