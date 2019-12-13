// from https://www.shadertoy.com/view/MdjGRw

// Hazel Quantock 2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// If you get artefacts on the ground, try this:
//#define INTERPOLATION_FIX

//compiler won't let me init const with mix()
#define lerp(a,b,c) ((a)*(1.0-(c))+(b)*(c))

// control colours
const vec3 rock = vec3(.8,.33,.18);
const vec3 darkRock = lerp(rock, vec3(.1,0,0),.8);
const vec3 dust = vec3(1,.53,.35);
const vec3 scrub = vec3(.13,.3,.1);

const vec3 sunColour = vec3(1.8,1.7,1.5);
const vec3 ambientColour = vec3(.09,.12,.2);
	
const vec3 atmosphere = vec3(.1,.25,1.0)/8.0; // this is the rate of attenuation per channel



const float tau = 6.28318530717958647692;

// Gamma correction
#define GAMMA (2.2)

vec3 ToLinear( in vec3 col )
{
	// simulate a monitor, converting colour values into light values
	return pow( col, vec3(GAMMA) );
}

vec3 ToGamma( in vec3 col )
{
	// convert back into colour values, so the correct light will come out of the monitor
	return pow( col, vec3(1.0/GAMMA) );
}

vec3 localRay;

void Cam( out vec3 pos, out vec3 ray, in vec3 origin, in vec3 look, in float zoom, in vec2 fragCoord )
{
	vec3 dir = look-origin;
	
	vec2 yrot = normalize(dir.xz);
	vec2 xrot = normalize(vec2(length(dir.xz),dir.y));
	
	// get rotation coefficients
	vec2 c = vec2(xrot.x,yrot.y);
	vec4 s;
	s.xy = vec2(-xrot.y,yrot.x); // worth testing if this is faster as sin or sqrt(1.0-cos);
	s.zw = -s.xy;

	// ray in view space
	ray.xy = fragCoord.xy - iResolution.xy*.5;
	ray.z = iResolution.y*zoom;
	ray = normalize(ray);
	
	localRay = ray;
	
	// rotate ray
	ray.yz = ray.yz*c.xx + ray.zy*s.zx;
	ray.xz = ray.xz*c.yy + ray.zx*s.yw;
	
	// position camera
	pos = origin;
}


// Noise functions, distinguished by variable types

vec2 Noise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 f = fract(x);
	f = f*f*(3.0-2.0*f);
//	vec3 f2 = f*f; f = f*f2*(10.0-15.0*f+6.0*f2);

	vec2 uv = (p.xy+vec2(37.0,17.0)*p.z) + f.xy;

#ifndef INTERPOLATION_FIX
	vec4 rg = textureLod( iChannel0, (uv+0.5)/256.0, 0.0 );
#else
	// on some hardware interpolation lacks precision
	vec4 rg = mix( mix(
				textureLod( iChannel0, (floor(uv)+0.5)/256.0, 0.0 ),
				textureLod( iChannel0, (floor(uv)+vec2(1,0)+0.5)/256.0,  0.0 ),
				fract(uv.x) ),
				  mix(
				textureLod( iChannel0, (floor(uv)+vec2(0,1)+0.5)/256.0, 0.0 ),
				textureLod( iChannel0, (floor(uv)+1.5)/256.0, 0.0 ),
				fract(uv.x) ),
				fract(uv.y) );
#endif			  

	return mix( rg.yw, rg.xz, f.z );
}

vec4 Noise( in vec2 x )
{
    vec2 p = floor(x.xy);
    vec2 f = fract(x.xy);
	f = f*f*(3.0-2.0*f);
//	vec2 f2 = f*f; f = f*f2*(10.0-15.0*f+6.0*f2);

	vec2 uv = p.xy + f.xy;
#ifndef INTERPOLATION_FIX
	return textureLod( iChannel0, (uv+0.5)/256.0, 0.0 );
#else
	// on some hardware interpolation lacks precision
	return mix( mix(
				textureLod( iChannel0, (floor(uv)+0.5)/256.0, 0.0 ),
				textureLod( iChannel0, (floor(uv)+vec2(1,0)+0.5)/256.0, 0.0 ),
				fract(uv.x) ),
				  mix(
				textureLod( iChannel0, (floor(uv)+vec2(0,1)+0.5)/256.0, 0.0 ),
				textureLod( iChannel0, (floor(uv)+1.5)/256.0, 0.0 ),
				fract(uv.x) ),
				fract(uv.y) );
#endif			  
}

vec4 Noise( in ivec2 x )
{
	return textureLod( iChannel0, (vec2(x)+0.5)/256.0, 0.0 );
}

vec2 Noise( in ivec3 x )
{
	vec2 uv = vec2(x.xy)+vec2(37.0,17.0)*float(x.z);
	return textureLod( iChannel0, (uv+0.5)/256.0, 0.0 ).xz;
}

vec4 Rand( in int x )
{
	vec2 uv;
	uv.x = (float(x)+0.5)/256.0;
	uv.y = (floor(uv.x)+0.5)/256.0;
	return textureLod( iChannel0, uv, 0.0 );
}

float DistanceField( vec3 pos, float t );
float DistanceField2( vec3 pos );

vec3 Normal( vec3 pos )
{
	const vec2 delta = vec2(0,.001);
	vec3 grad;
	grad.x = DistanceField2( pos+delta.yxx )-DistanceField2( pos-delta.yxx );
	grad.y = DistanceField2( pos+delta.xyx )-DistanceField2( pos-delta.xyx );
	grad.z = DistanceField2( pos+delta.xxy )-DistanceField2( pos-delta.xxy );
	return normalize(grad);
}

float Trace( vec3 pos, vec3 ray, vec2 interval )
{
	//const vec2 interval = vec2(0.0,10.0); // could do ray traced bounding shape to get tighter region
	
	float h = 1.0;
	float t = interval.x;
	for ( int i=0; i < 100; i++ )
	{
		if ( t > interval.y || h < .001 )
			break;
		h = DistanceField( pos+ray*t, t );
		t += h;
	}
	
	if ( t > interval.y || t < interval.x || h > .1 )
		return 0.0;
	
	return t;
}

float Trace2( vec3 pos, vec3 ray, vec2 interval )
{
	//const vec2 interval = vec2(0.0,10.0); // could do ray traced bounding shape to get tighter region
	
	float h = 1.0;
	float t = interval.x;
	for ( int i=0; i < 30; i++ )
	{
		if ( t > interval.y || h < .01 )
			break;
		h = DistanceField( pos+ray*t, 1.0 );
		t += h;
	}
	
	if ( t > interval.y || t < interval.x || h > .1 )
		return 0.0;
	
	return t;
}


// ----------------------

float Mesa( vec2 pos )
{
	return
		Noise( pos*exp2(1.0) ).x/exp2(1.0) +
		Noise( pos*exp2(2.0) ).x/exp2(2.0) +
		Noise( pos*exp2(3.0) ).x/exp2(3.0) +
		Noise( pos*exp2(4.0) ).x/exp2(4.0) +
		Noise( pos*exp2(5.0) ).x/exp2(5.0) +
		Noise( pos*exp2(6.0) ).x/exp2(6.0) +
		Noise( pos*exp2(7.0) ).x/exp2(7.0) +
		Noise( pos*exp2(8.0) ).x/exp2(8.0);
}

float Mesa2( vec2 pos )
{
	return
		Noise( pos*exp2(1.0) ).x/exp2(1.0) +
		Noise( pos*exp2(2.0) ).x/exp2(2.0) +
		Noise( pos*exp2(3.0) ).x/exp2(3.0) +
		Noise( pos*exp2(4.0) ).x/exp2(4.0) +
		Noise( pos*exp2(5.0) ).x/exp2(5.0);
}

float Mesa3( vec2 pos )
{
	return
		Noise( pos*exp2(1.0) ).x/exp2(1.0) +
		Noise( pos*exp2(2.0) ).x/exp2(2.0) +
		Noise( pos*exp2(3.0) ).x/exp2(3.0) +
		Noise( pos*exp2(4.0) ).x/exp2(4.0) +
		Noise( pos*exp2(5.0) ).x/exp2(5.0) +
		Noise( pos*exp2(6.0) ).x/exp2(6.0) +
		Noise( pos*exp2(7.0) ).x/exp2(7.0) +
		Noise( pos*exp2(8.0) ).x/exp2(8.0) +
		Noise( pos*exp2(9.0) ).x/exp2(9.0);
}

float Mesa0( vec2 pos )
{
	return
		Noise( pos*exp2(1.0) ).x/exp2(1.0) +
		Noise( pos*exp2(2.0) ).x/exp2(2.0);
}

float DistanceField( vec3 pos, float t )
{
	return
		min(
			//pos.y+.1-.5*max(.0,.5-Mesa(pos.xz)),
			(pos.y+.1-2.4*pow(max(.0,.6-Mesa2(pos.xz)),3.0))*1.0,
			(Mesa(pos.xz)+pos.y*.01-.3)*.5
			+.1*.03*(pos.y/.03-cos(pos.y/.03+Noise(pos.xy).g*2.0))
		)
		*mix(2.0,.9,1.0/max(1.0,t*3.0-4.0)); // lower values give more errors in distance but better up close
}
float DistanceField2( vec3 pos )
{
	return
		min(
			//pos.y+.1-.5*max(.0,.5-Mesa(pos.xz)),
			(pos.y+.1-2.4*pow(max(.0,.6-Mesa2(pos.xz)),3.0))*1.0,
			(Mesa3(pos.xz)+pos.y*.01-.3)*.5
			+.003*(pos.y/.1-cos(pos.y/.1+Noise(pos.xy).g*2.0))
		);
}


vec3 Shade( vec3 pos, vec3 norm, float dist )
{
	vec3 sunDir = normalize(vec3(1,2,3));
	float ndotl = dot(norm, sunDir);

	// shadow
	vec2 interval = vec2(.03,(.2-pos.y)/sunDir.y);
	float shadow = 1.0;
	if ( ndotl > 0.0 )
	{
		float s = Trace2( pos, sunDir, interval );
		
		if ( pos.y < .199 && (s > .0 || DistanceField( pos+interval.x*sunDir, dist ) < .0) )
			shadow = 0.0;
	}
	
	// albedo
	vec3 p = pos*vec3(5,100,5);
	float sedimentary = pow(mix(Noise(p).x,Noise(p*8.0).y,.2),2.0);
	vec3 albedoRock = mix( rock, darkRock, sedimentary );
	
	p = pos*100.0;
	p.xy = p.xy*cos(.75)+p.yx*vec2(-1,1)*sin(.75);
	p.yz = p.yz*cos(.5)+p.zy*vec2(-1,1)*sin(.5);
	float scrubPattern = pow((Noise(p).x+Noise(p*2.0).x*.5+Noise(p*16.0).x*.25)/1.75,2.0);
	vec3 albedoDirt = mix( dust, scrub, scrubPattern );
	
	vec3 albedo = mix( albedoRock, albedoDirt, smoothstep(.65,1.0,norm.y) );

	vec3 ambient = (dot(norm,sunDir*vec3(-1,1,-1))*.5+.5)*ambientColour;
	
	vec3 lighting = shadow*max(ndotl,.0)*sunColour + ambient;
	
	vec3 col = albedo*lighting;

	// fog
	// absorb blue faster, by mixing each channel independently
	col = mix( vec3(.5), col, exp2(-dist*atmosphere*2.0));
	
	return col;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	float T = iTime*.2+iMouse.x*.1;
	
	vec3 camPos, ray;
//	CamPolar(camPos, ray, vec3(T,-.2,0), vec2(-cos(T*.5)*.15+.26,T/8.0), 4.0, 2.0);

	// better camera path
	vec3 look = vec3(T*.1-4.0,-.0,0);
	float a = -1.0-T/8.0;
	camPos = look + 2.0*vec3(sin(a),0,cos(a));
	camPos.y = /*min( camPos.y,*/ .35-.6*Mesa0(camPos.xz) ;//);
	Cam( camPos, ray, camPos, look, 2.0, fragCoord );
	
	vec3 pos = camPos;
	
	// ray trace against bounding plane on top
	// this improves precision and lets us render waaaay into the distance
	vec2 interval = vec2(0,10.0);
	bool sky = false;
	if ( pos.y > .2 )
	{
		if ( ray.y < .0 )
			interval.x = (pos.y-.2)/(-ray.y);
		else
			sky = true;
	}
	else
	{
		if ( ray.y > .0 )
			interval.y = min( interval.y, (pos.y-.2)/(-ray.y) );
	}
	
	vec3 norm;

	if ( DistanceField(pos+interval.x*ray, 1.0) <= .0 )
	{
		if ( interval.x > interval.y )
			sky = true;
		
		pos = pos + interval.x*ray;
		norm = vec3(0,1,0); // if we're inside the pattern, normal is up
	}
	else
	{
		float t = Trace( pos, ray, interval );
		
		if ( t <= .0 )
			sky = true;
		
		pos = pos+t*ray;
		norm = Normal(pos);
	}

	vec3 col;
	if ( sky )
	{
		col = mix( vec3(1), vec3(0), exp2(-(10.0/max(ray.y,.01))*atmosphere/2.5));;
	}
	else
	{
		col = Shade( pos, norm, length(pos-camPos) );
	}
	
	// a little post-processing, to make it feel a bit photographic
	// vignette
	col *= smoothstep(.65,1.0,localRay.z*localRay.z);

	// grain
	vec3 grainPos = vec3(fragCoord.xy*.8,iTime*30.0);
	grainPos.xy = grainPos.xy*cos(.75)+grainPos.yx*vec2(-1,1)*sin(.75);
	grainPos.yz = grainPos.yz*cos(.5)+grainPos.zy*vec2(-1,1)*sin(.5);
	vec2 filmNoise = Noise(grainPos);
	col *= mix( vec3(1), mix(vec3(1,.5,0),vec3(0,.5,1),filmNoise.x), .4*pow(filmNoise.y,2.0) );

	// dust
	vec2 uv = fragCoord.xy/iResolution.y;
	T = floor( iTime * 60.0 );
	vec2 scratchSpace = mix( Noise(vec3(uv*8.0,T)).xy, uv.yx+T, .8 )*1.0;
	float scratches = texture( iChannel1, scratchSpace ).r;
	
	col *= vec3(1.0)-vec3(.3,.5,.7)*pow(1.0-smoothstep( .0, .05, scratches ),2.0);

	
	// brightness, contrast
	//col = smoothstep(vec3(.0),vec3(.8),col);
	col = col*1.3-.02;

	col = max(vec3(0),col);
	fragColor = vec4(ToGamma(col),1.0);
}