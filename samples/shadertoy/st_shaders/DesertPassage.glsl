// from https://www.shadertoy.com/view/XtyGzc

/*
	Desert Passage
	--------------

	This is a rendering of an ancient sandstone passageway, carved out over time on a planet... that 
	slightly resembles a hastily-constructed, man-made set on the original Star Trek series, and where 
	the occasional rock hangs in mid air. Thankfully, I made up for it with some three-layer dust to 
	give it a bit more authenticity. :D

	I tend to favor abstract scenes, simply for the reason that so-called natural looking ones are harder 
	to produce at decent frame rates... for me, anyway. With abstract scenes, I can use fake physics - or
	incorrect physics - then claim that I meant for it to be that way. :)

	Aiekick's "Weird Canyon" was the inspiration for this. I liked his idea to carve out a solid object 
	with Voronoi to create an "Antelope Canyon" like setting. The rendering style was influenced by 
	Dave Hoskins's "Skin Peeler," which is based off of Nimitz's "Xyptonjtroz" example. Originally, I'd
	hoped to emulate the look of IQ's "Fractal Cave" with cool streaming light shafts, but I thought I'd 
	save that for another time.

	The scene is created by constructing a sinusoidal cave like mass, then carving out the surface with a
	custom cellular algorithm that emulates Voronoi. It's pretty self explanatory and is contained in the
	distance function. By the way, if you tried rendering the same scene using regular 3D Voronoi, your 
	circuits would fry.

	In fact, this particular example makes usage of layering in order of aesthetic importance. Basically, 
	the larger, undulating base layers are raymarched -- preferably with cheap algorithms -- and the 
    finer details -- which tend to be more expensive -- are bump mapped.
	

	Related examples:

	// Gorgeous rendering.
	Fractal Cave - IQ
	https://www.shadertoy.com/view/Xtt3Wn

	// A more abstract version.
	Weird Canyon - Aiekick
	https://www.shadertoy.com/view/XtjSRm

	// One of my favorite simple coloring jobs.
    Skin Peeler - Dave Hoskins
    https://www.shadertoy.com/view/XtfSWX
    Based on one of my all time favorites:
    Xyptonjtroz - Nimitz
	https://www.shadertoy.com/view/4ts3z2

*/

// The far plane. I'd like this to be larger, but the extra iterations required to render the 
// additional scenery starts to slow things down on my slower machine.
#define FAR 80.


// 2x2 matrix rotation. Angle vector, courtesy of Fabrice.
mat2 rot2( float th ){ vec2 a = sin(vec2(1.5707963, 0) + th); return mat2(a, -a.y, a.x); }

// 1x1 and 3x1 hash functions.
float hash( float n ){ return fract(cos(n)*45758.5453); }
float hash( vec3 p ){ return fract(sin(dot(p, vec3(7, 157, 113)))*45758.5453); }


// Draw the object on the repeat tile. In this case, a sphere. The result it squared, but that can
// be taken into account after obtaining the minimum. See below.
float drawObject(in vec3 p){ p = fract(p)-.5; return dot(p, p); }


// Repeat cellular tile routine. The operation count is extremely low when compared to conventional
// methods. No loops, no flooring, no hash calls, etc. Conceptually speaking, this is the fastest way 
// to produce a reasonable 3D cellular pattern... Although, there's one with three objects and no 
// rotation, but quality really suffers at that point. 
float cellTile(in vec3 p){
    
    // Draw four overlapping objects (spheres, in this case) at various positions throughout the tile.
    vec4 d; 
    d.x = drawObject(p - vec3(.81, .62, .53));
    p.xy = vec2(p.y-p.x, p.y + p.x)*.7071;
    d.y = drawObject(p - vec3(.39, .2, .11));
    p.yz = vec2(p.z-p.y, p.z + p.y)*.7071;
    d.z = drawObject(p - vec3(.62, .24, .06));
    p.xz = vec2(p.z-p.x, p.z + p.x)*.7071;
    d.w = drawObject(p - vec3(.2, .82, .64));

    // Obtain the minimum, and you're done.
    d.xy = min(d.xz, d.yw);
        
    return min(d.x, d.y)*2.66; // Scale between zero and one... roughly.
}



// The path is a 2D sinusoid that varies over time, which depends upon the frequencies and amplitudes.
vec2 path(in float z){ return vec2(20.*sin(z * .04), 4.*cos(z * .09) + 3.*(sin(z*.025)  - 1.)); }


// The triangle function that Shadertoy user Nimitz has used in various triangle noise demonstrations.
// See Xyptonjtroz - Very cool.
//vec3 tri(in vec3 x){return abs(fract(x)-.5);} // Triangle function.

// The function used to perturb the walls of the passage structure: I came up with the tiled cellular
// routine in order to raymarch something that resembled Voronoi. Regular 3D Voronoi is so intensive
// that it's hard enough to bump map, let alone raymarch. Conceptually speaking, this algorithm is as
// fast as you're going to get, yet it's still only good for one raymarching layer. The other cellular
// layers (two more) have been bump mapped.
float surfFunc(in vec3 p){
    
    float c = cellTile(p/6.); // Resembles a standard 3D Voronoi layer.
    return mix(c, cos(c*6.283*2.)*.5 + .5, .125); // Mixing in a touch of sinusoidal variation.
    
    // Cheaper wall layering (although, not much), for comparison. 
    //p /= 2.;
    //float c = dot(tri(p*.5 + tri(p*0.25).yzx), vec3(0.666));
    //return mix(c, cos(c*6.283*1.5)*.5 + .5, .25);
    
    //p /= 5.;
    //return dot(tri(p + tri(p.zxy)), vec3(0.666));

}


// IQ's smooth minium function. 
float smin(float a, float b , float s){
    
    float h = clamp( 0.5 + 0.5*(b-a)/s, 0. , 1.);
    return mix(b, a, h) - h*(1.0-h)*s;
}

// Smooth maximum, based on IQ's smooth minimum.
float smax(float a, float b, float s){
    
    float h = clamp( 0.5 + 0.5*(a-b)/s, 0., 1.);
    return mix(b, a, h) + h*(1.0-h)*s;
}

// The desert passage scene. Use a gyroid object as the passage-system base layer, carve it out
// with the cellular function, put in a floor, then cap the whole thing off at roof height.
float map(vec3 p){
    
	// Surface function to perturb the walls.
    float sf = surfFunc(p);

    // A gyroid object to form the main passage base layer.
    float cav = dot(cos(p*3.14159265/8.), sin(p.yzx*3.14159265/8.)) + 2.;
    
    // Mold everything around the path.
    p.xy -= path(p.z);
    
    // The oval tunnel. Basically, a circle stretched along Y.
    float tun = 1.5 - length(p.xy*vec2(1, .4));
   
    // Smoothly combining the tunnel with the passage base layer,
    // then perturbing the walls.
    tun = smax(tun, 1.-cav, 2.) + .75 + (.5-sf);
    
    float gr = p.y + 7. - cav*.5 + (.5-sf)*.5; // The ground.
    float rf = p.y - 15.; // The roof cutoff point.
    
    // Smoothly combining the passage with the ground, and capping
    // it off at roof height.
    return smax(smin(tun, gr, .1), rf, 1.);
 
 
}



// Basic raymarcher. I haven't tweaked this yet. I think it needs it.
float trace(in vec3 ro, in vec3 rd){

    float t = 0., h;
    
    for(int i=0; i<128; i++){
    
        h = map(ro+rd*t);
        // Note the "t*b + a" addition. Basically, we're putting less emphasis on accuracy, as
        // "t" increases. It's a cheap trick that works in most situations... Not all, though.
        if(abs(h)<0.002*(t*.25 + 1.) || t>FAR) break; // Alternative: 0.001*max(t*.25, 1.)
        
        t += h*.8;
    }

    return min(t, FAR);
}


// Tetrahedral normal - courtesy of IQ. I'm in saving mode, so the two "map" calls saved make
// a difference. Also because of the random nature of the scene, the tetrahedral normal has the 
// same aesthetic effect as the regular - but more expensive - one, so it's an easy decision.
vec3 normal(in vec3 p)
{  
    vec2 e = vec2(-1., 1.)*0.001;   
	return normalize(e.yxx*map(p + e.yxx) + e.xxy*map(p + e.xxy) + 
					 e.xyx*map(p + e.xyx) + e.yyy*map(p + e.yyy) );   
}

/*
// Standard normal function. It's not as fast as the tetrahedral calculation, but more symmetrical.
vec3 normal(in vec3 p) {
	const vec2 e = vec2(0.002, 0);
	return normalize(vec3(map(p + e.xyy) - map(p - e.xyy), map(p + e.yxy) - map(p - e.yxy),	map(p + e.yyx) - map(p - e.yyx)));
}
*/

// Tri-Planar blending function. Based on an old Nvidia writeup:
// GPU Gems 3 - Ryan Geiss: https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch01.html
vec3 tex3D( sampler2D t, in vec3 p, in vec3 n ){
   
    n = max(abs(n) - .2, .001); // The original is multiplied by "7," but it feels slightly redundant.
    n /= (n.x + n.y + n.z );  // Normalize.
    // Three planes, weighted by their normals. Hence, tri-planar, I guess. :)
	p = (texture(t, p.yz)*n.x + texture(t, p.zx)*n.y + texture(t, p.xy)*n.z).xyz;
    return p*p; // Rough sRGB to linear.
}


// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total.
vec3 doBumpMap( sampler2D tx, in vec3 p, in vec3 n, float bf){
   
    const vec2 e = vec2(0.001, 0);
    
    // Three gradient vectors rolled into a matrix, constructed with offset greyscale texture values.    
    mat3 m = mat3( tex3D(tx, p - e.xyy, n), tex3D(tx, p - e.yxy, n), tex3D(tx, p - e.yyx, n));
    
    vec3 g = vec3(0.299, 0.587, 0.114)*m; // Converting to greyscale.
    g = (g - dot(tex3D(tx,  p , n), vec3(0.299, 0.587, 0.114)) )/e.x; g -= n*dot(n, g);
                      
    return normalize( n + g*bf ); // Bumped normal. "bf" - bump factor.
	
}

// Compact, self-contained version of IQ's 3D value noise function. I have a transparent noise
// example that explains it, if you require it.
float n3D(in vec3 p){
    
	const vec3 s = vec3(7, 157, 113);
	vec3 ip = floor(p); p -= ip; 
    vec4 h = vec4(0., s.yz, s.y + s.z) + dot(ip, s);
    p = p*p*(3. - 2.*p); //p *= p*p*(p*(p * 6. - 15.) + 10.);
    h = mix(fract(sin(h)*43758.5453), fract(sin(h + s.x)*43758.5453), p.x);
    h.xy = mix(h.xz, h.yw, p.y);
    return mix(h.x, h.y, p.z); // Range: [0, 1].
}

// Surface bump function: Cheap, but with decent visual impact. Of couse, "cheap" is a relative
// term. This contains two 3D cellular functions and a 3D noise function. Thankfully, they're all
// custom written and pretty quick.
float bumpSurf3D( in vec3 p){

    float bmp = cellTile(p/3.)*.8 + cellTile(p)*.2;
    float ns = n3D(p*6. - bmp*6.);
    
    return mix(bmp, 1. - abs(ns-.333)/.667, .05);

}

// Standard function-based bump mapping function.
vec3 doBumpMap(in vec3 p, in vec3 nor, float bumpfactor){
    
    const vec2 e = vec2(0.001, 0);
    float ref = bumpSurf3D(p);                 
    vec3 grad = (vec3(bumpSurf3D(p - e.xyy),
                      bumpSurf3D(p - e.yxy),
                      bumpSurf3D(p - e.yyx) )-ref)/e.x;                     
          
    grad -= nor*dot(nor, grad);          
                      
    return normalize( nor + grad*bumpfactor );
	
}

// The iterations should be higher for proper accuracy, but in this case, I wanted less accuracy, just to leave
// behind some subtle trails of light in the caves. They're fake, but they look a little like light streaming 
// through some openings... kind of.
float softShadow(in vec3 ro, in vec3 rd, in float start, in float end, in float k){

    float shade = 1.0;
    // Increase this and the shadows will be more accurate, but the wispy light trails in the caves will disappear.
    // Plus more iterations slow things down, so it works out, in this case.
    const int maxIterationsShad = 10; 

    // The "start" value, or minimum, should be set to something more than the stop-threshold, so as to avoid a collision with 
    // the surface the ray is setting out from. It doesn't matter how many times I write shadow code, I always seem to forget this.
    // If adding shadows seems to make everything look dark, that tends to be the problem.
    float dist = start;
    float stepDist = end/float(maxIterationsShad);

    // Max shadow iterations - More iterations make nicer shadows, but slow things down. Obviously, the lowest 
    // number to give a decent shadow is the best one to choose. 
    for (int i=0; i<maxIterationsShad; i++){
        // End, or maximum, should be set to the distance from the light to surface point. If you go beyond that
        // you may hit a surface not between the surface and the light.
        float h = map(ro + rd*dist);
        //shade = min(shade, k*h/dist);
        shade = min(shade, smoothstep(0., 1., k*h/dist));
        
        // What h combination you add to the distance depends on speed, accuracy, etc. To be honest, I find it impossible to find 
        // the perfect balance. Faster GPUs give you more options, because more shadow iterations always produce better results.
        // Anyway, here's some posibilities. Which one you use, depends on the situation:
        // +=max(h, 0.001), +=clamp( h, 0.01, 0.25 ), +=min( h, 0.1 ), +=stepDist, +=min(h, stepDist*2.), etc.
        
        // In this particular instance the light source is a long way away. However, we're only taking a few small steps
        // toward the light and checking whether anything "locally" gets in the way. If a part of the scene a long distance away
        // is between our hit point and the light source, it won't be accounted for. Technically that's not correct, but the local
        // shadows give that illusion... kind of.
        dist += clamp(h, .2, stepDist); // For this example only. Not to be trusted. :)
        
        // There's some accuracy loss involved, but early exits from accumulative distance function can help.
        if (abs(h)<0.001 || dist > end) break; 
    }

    // I usually add a bit to the final shade value, which lightens the shadow a bit. It's a preference thing. Really dark shadows 
    // look too brutal to me.
    return min(max(shade, 0.) + .1, 1.); 
}





// Ambient occlusion, for that self shadowed look. Based on the original by XT95. I love this 
// function. For a better version, and usage, refer to XT95's examples below:
//
// Hemispherical SDF AO - https://www.shadertoy.com/view/4sdGWN
// Alien Cocoons - https://www.shadertoy.com/view/MsdGz2
float calculateAO( in vec3 p, in vec3 n)
{
	float ao = 0.0, l;
	const float nbIte = 6.0;
    const float maxDist = 3.;
	//const float falloff = 0.9;
    for(float i=1.; i< nbIte+.5; i++ ){
    
        l = (i*.66 + hash(i)*.34)/nbIte*maxDist;
        
        ao += (l - map( p + n*l ))/(1.+ l);// / pow(1.+l, falloff);
    }
	
    return clamp( 1.-ao/nbIte, 0., 1.);
}


// Just a single color. I debated over whether to include the sun, but the dust is there and I'm saving cycles.
vec3 getSky(){ return vec3(2., 1.4, .7); }


/////
// Code block to produce three layers of fine dust. Not sophisticated at all.
// If you'd like to see a much more sophisticated version, refer to Nitmitz's
// Xyptonjtroz example. Incidently, I wrote this off the top of my head, but
// I did have that example in mind when writing this.
float trig3(in vec3 p){
    p = cos(p*2. + (cos(p.yzx) + 1.)*1.57);// + iTime*1.
    return dot(p, vec3(0.1666)) + 0.5;
}

// Basic low quality noise consisting of three layers of rotated, mutated 
// trigonometric functions. Needs work, but it's OK for this example.
float trigNoise3D(in vec3 p){

    // 3D transformation matrix.
    const mat3 m3RotTheta = mat3(0.25, -0.866, 0.433, 0.9665, 0.25, -0.2455127, -0.058, 0.433, 0.899519 )*1.5;
  
	float res = 0.;

    float t = trig3(p*3.14159265);
	p += (t);
    p = m3RotTheta*p;
    //p = (p+0.7071)*1.5;
    res += t;
    
    t = trig3(p*3.14159265); 
	p += (t)*0.7071;
    p = m3RotTheta*p;
     //p = (p+0.7071)*1.5;
    res += t*0.7071;

    t = trig3(p*3.14159265);
	res += t*0.5;
	 
	return res/2.2071;
}

// Hash to return a scalar value from a 3D vector.
float hash31(vec3 p){ return fract(sin(dot(p, vec3(127.1, 311.7, 74.7)))*43758.5453); }

// Very few layers of cheap trigonometric noise to produce some subtle mist.
// Start at the ray origin, then take four samples of noise between it
// and the surface point. Apply some very simplistic lighting along the 
// way. It's not particularly well thought out, but it doesn't have to be.
float getMist(in vec3 ro, in vec3 rd, in vec3 lp, in float t){

    float mist = 0.;
    ro += rd*t/3.; // Edge the ray a little forward to begin.

    
    for (int i = 0; i<3; i++){
        // Lighting. Technically, a lot of these points would be
        // shadowed, but we're ignoring that.
        float sDi = length(lp-ro)/FAR; 
	    float sAtt = 1./(1. + sDi*0.1 + sDi*sDi*0.01);
	    // Noise layer.
        mist += trigNoise3D(ro/2.)*sAtt;//trigNoise3D
        // Advance the starting point towards the hit point.
        ro += rd*t/3.;
    }
    
    // Add a little noise, then clamp, and we're done.
    return clamp(mist/1.5 + hash31(ro)*0.1-0.05, 0., 1.);

}

void mainImage( out vec4 fragColor, in vec2 fragCoord ){	


	
	// Screen coordinates.
	vec2 u = (fragCoord - iResolution.xy*.5)/iResolution.y;
	
	// Camera Setup.
	vec3 ro = vec3(0, 0, iTime*8.); // Camera position, doubling as the ray origin.
	vec3 lookAt = ro + vec3(0, 0, .5);  // "Look At" position.
 
	// Using the Z-value to perturb the XY-plane.
	// Sending the camera and "look at" vectors down the tunnel. The "path" function is 
	// synchronized with the distance function.
	ro.xy += path(ro.z);
	lookAt.xy += path(lookAt.z);

    // Using the above to produce the unit ray-direction vector.
    float FOV = 3.14159265/2.5; // FOV - Field of view.
    vec3 forward = normalize(lookAt - ro);
    vec3 right = normalize(vec3(forward.z, 0, -forward.x )); 
    vec3 up = cross(forward, right);

    // rd - Ray direction.
    vec3 rd = normalize(forward + FOV*u.x*right + FOV*u.y*up);
    
    // Swiveling the camera about the XY-plane (from left to right) when turning corners.
    // Naturally, it's synchronized with the path in some kind of way.
	rd.xy = rot2( path(lookAt.z).x/64. )*rd.xy;
    
	
    // Usually, you'd just make this a unit directional light, and be done with it, but I
    // like some of the angular subtleties of point lights, so this is a point light a
    // long distance away. Fake, and probably not advisable, but no one will notice.
    vec3 lp = vec3(FAR*.5, FAR, FAR) + vec3(0, 0, ro.z);
 

	// Raymarching.
    float t = trace(ro, rd);
    
   
    // Sky. Only retrieving a single color this time.
    vec3 sky = getSky();
    
    // The passage color. Can't remember why I set it to sky. I'm sure I had my reasons.
    vec3 col = sky;
    
    // Surface point. "t" is clamped to the maximum distance, and I'm reusing it to render
    // the mist, so that's why it's declared in an untidy postion outside the block below...
    // It seemed like a good idea at the time. :)
    vec3 sp = ro+t*rd; 
    
    float pathHeight = sp.y-path(sp.z).y; // Path height line, of sorts.
    
    // If we've hit the ground, color it up.
    if (t < FAR){
    
        
        vec3 sn = normal( sp ); // Surface normal.
        
        // Light direction vector. From the sun to the surface point. We're not performing
        // light distance attenuation, since it'll probably have minimal effect.
        vec3 ld = lp-sp;
        ld /= max(length(ld), 0.001); // Normalize the light direct vector.

        
        // Texture scale factor.        
        const float tSize = 1./4.;
        
        // Function based bump mapping.
        sn = doBumpMap(sp, sn, .75/(1. + t/FAR*.25));
        
        // Bump mapping with the pink sandstone texture to provide a bit of gritty detailing.
        float bf = (pathHeight + 5. < 0.)?  .05: .025;
        sn = doBumpMap(iChannel0, sp*tSize, sn, bf/(1. + t/FAR));
        
        
        float shd = softShadow(sp, ld, 0.05, FAR, 8.); // Shadows.
        float ao = calculateAO(sp, sn); // Ambient occlusion.
        
        float dif = max( dot( ld, sn ), 0.0); // Diffuse term.
        float spe = pow(max( dot( reflect(-ld, sn), -rd ), 0.0 ), 5.); // Specular term.
        float fre = clamp(1.0 + dot(rd, sn), 0.0, 1.0); // Fresnel reflection term.
 
        // Schlick approximation. I use it to tone down the specular term. It's pretty subtle,
        // so could almost be aproximated by a constant, but I prefer it. Here, it's being
        // used to give a sandstone consistency... It "kind of" works.
		float Schlick = pow( 1. - max(dot(rd, normalize(rd + ld)), 0.), 5.0);
		float fre2 = mix(.2, 1., Schlick);  //F0 = .2 - Hard clay... or close enough.
       
        // Overal global ambience. It's made up, but I figured a little occlusion (less ambient light
        // in the corners, etc) and reflectance would be in amongst it... Sounds good, anyway. :)
        float amb = ao*.125 + fre*fre2*.2;
        
        // Coloring the soil - based on depth. Based on a line from Dave Hoskins's "Skin Peeler."
        col = clamp(mix(vec3(1.152, 0.4275,.153), vec3(.225, 0.05985, 0.0153), -sn.y*.5 + pathHeight*.5 + 1.75), vec3(.9, 0.534375, 0.239), vec3(.9, .855, .765));
        
              
        // Give the soil a bit of a sandstone texture.
        col = smoothstep(-.5, 1., tex3D(iChannel0, sp*tSize, sn)*2.)*(col + vec3(.225, .21375, .19125));
        
        // Making the ground reflect just a little more to give the appearance of fine dust or sand...
        // It's a work in progress. :)
        col += smoothstep(0., 1., -pathHeight - 5.5)*fre*.25;
 
        // A bit of sky reflection. Not really accurate, but I've been using fake physics since the 90s. :)
        col += getSky()*fre*fre2; 
        
        
        
        // Combining all the terms from above. Some diffuse, some specular - both of which are
        // shadowed and occluded - plus some global ambience. Not entirely correct, but it's
        // good enough for the purposes of this demonstation.        
        col = (col*(dif + amb) + vec3(1)*fre2*spe)*shd*ao + amb*pow(col, vec3(2.));

        
    }
    
   
    // Combine the scene with the sky using some cheap volumetric substance.
	float dust = getMist(ro, rd, lp, t)*(1.-clamp((pathHeight - 5.)*.125, 0., 1.));//(-rd.y + 1.);
    sky = getSky()*mix(1., .75, dust);
    col = mix(col, sky, min(t*t*1.5/FAR/FAR, 1.)); // Quadratic fade off. More subtle.
    //col = mix(col, sky, min(t*.75/FAR, 1.)); // Linear fade. Much dustier. I kind of like it.

    
    // Standard way to do a square vignette. Note that the maxium value value occurs at "pow(0.5, 4.) = 1./16," 
    // so you multiply by 16 to give it a zero to one range. This one has been toned down with a power
    // term to give it more subtlety.
    u = fragCoord/iResolution.xy;
    col = min(col, 1.)*pow( 16.0*u.x*u.y*(1.0-u.x)*(1.0-u.y) , .125);
 
    // Done.
	fragColor = vec4(sqrt(clamp(col, 0., 1.)), 1);
}
