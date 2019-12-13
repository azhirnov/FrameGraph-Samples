// from https://www.shadertoy.com/view/MlG3zh

/*

	Canyon Pass
	-----------

	Combining some cheap distance field functions with some functional and texture-based bump 
	mapping to carve out a rocky canyon-like passageway.

	There's nothing overly exciting about this example. I was trying to create a reasonably
    convincing looking rocky setting using cheap methods.

	I added in some light frosting, mainly to break the monotony of the single colored rock.
	There's a mossy option below, for anyone interested. Visually speaking, I find the moss more
	interesting, but I thought the frost showed the rock formations a little better. Besides,
	I'd like to put together a more dedicated greenery example later.

*/

#define PI 3.14159265
#define FAR 60.

// Extra settings. Use one or the other. The MOSS setting overrides the HOT setting.
// Mossy setting. Better, if you want more color to liven things up.
#define MOSS 
// Hot setting. It represents 2 minutes of post processing work, so it's definitely nothing to excited about. :)
//#define HOT

// Coyote's snippet to provide a virtual reality element. Really freaky. It gives the scene 
// physical depth, but you have to do that magic-picture focus-adjusting thing with your eyes.
//#define THREE_D 


// Rotation matrix.
const mat2 rM = mat2(.7071, .7071, -.7071, .7071); 

// 2x2 matrix rotation. Note the absence of "cos." It's there, but in disguise, and comes courtesy
// of Fabrice Neyret's "ouside the box" thinking. :)
mat2 rot2( float a ){ vec2 v = sin(vec2(1.570796, 0) + a);	return mat2(v, -v.y, v.x); }


// Tri-Planar blending function. Based on an old Nvidia writeup:
// GPU Gems 3 - Ryan Geiss: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch01.html
vec3 tex3D(sampler2D channel, vec3 p, vec3 n){
    
    //vec3 col = mix(vec3(.7, 1, 1.3), vec3(1), n.y*.5 + .5);
    
    n = max(abs(n) - .2, 0.001);
    n /= dot(n, vec3(1));
	vec3 tx = texture(channel, p.zy).xyz;
    vec3 ty = texture(channel, p.xz).xyz;
    vec3 tz = texture(channel, p.xy).xyz;
    
    // Textures are stored in sRGB (I think), so you have to convert them to linear space 
    // (squaring is a rough approximation) prior to working with them... or something like that. :)
    // Once the final color value is gamma corrected, you should see correct looking colors.
    return (tx*tx*n.x + ty*ty*n.y + tz*tz*n.z);//*col;
}


// Cellular tile setup. Draw four overlapping objects (spheres, in this case) 
// at various positions throughout the tile.
 
float drawObject(in vec3 p){
  
    p = fract(p)-.5;
    return dot(p, p);
    
}


// 3D cellular tile function.
float cellTile(in vec3 p){
   
    vec4 d; 
    
    // Plot four objects.
    d.x = drawObject(p - vec3(.81, .62, .53));
    p.xy *= rM;
    d.y = drawObject(p - vec3(.6, .82, .64));
    p.yz *= rM;
    d.z = drawObject(p - vec3(.51, .06, .70));
    p.zx *= rM;
    d.w = drawObject(p - vec3(.12, .62, .64));

	// Obtaining the minimum distance.
    d.xy = min(d.xz, d.yw);
    
    // Normalize... roughly. Trying to avoid another min call (min(d.x*A, 1.)).
    return  min(d.x, d.y)*2.5;
    
}


// The triangle function that Shadertoy user Nimitz has used in various triangle noise demonstrations.
// See Xyptonjtroz - Very cool. Anyway, it's not really being used to its full potential here.
// https://www.shadertoy.com/view/4ts3z2
vec3 tri(in vec3 x){return abs(fract(x)-.5);} // Triangle function.



// The path is a 2D sinusoid that varies over time, depending upon the frequencies, and amplitudes.
vec2 path(in float z){
   
    //return vec2(0); // Straight.
    float a = sin(z * 0.11);
    float b = cos(z * 0.14);
    return vec2(a*4. -b*1.5, b*1.7 + a*1.5); 
    //return vec2(a*4. -b*1.5, 0.); // Just X.
    //return vec2(0, b*1.7 + a*1.5); // Just Y.
}



// A fake noise looking sinusoial field - flanked by a ground plane and some walls with
// some triangular-based perturbation mixed in. Cheap, but reasonably effective.
float map(vec3 p){
    
 
    p.xy -= path(p.z); // Wrap the passage around
    
    vec3 w = p; // Saving the position prior to mutation.
    
    vec3 op = tri(p*.4*3. + tri(p.zxy*.4*3.)); // Triangle perturbation.
   
    
    float ground = p.y + 3.5 + dot(op, vec3(.222))*.3; // Ground plane, slightly perturbed.
 
    p += (op - .25)*.3; // Adding some triangular perturbation.
   
	p = cos(p*.315*1.41 + sin(p.zxy*.875*1.27)); // Applying the sinusoidal field (the rocky bit).
    
    float canyon = (length(p) - 1.05)*.95 - (w.x*w.x)*.01; // Spherize and add the canyon walls.
    
    return min(ground, canyon);

    
}



// Surface bump function. I'm reusing the "cellTile" function, but absoulte sinusoidals
// would do a decent job too.
float bumpSurf3D( in vec3 p, in vec3 n){
    
    //return (cellTile(p/1.5))*.66 + (cellTile(p*2./1.5))*.34;
    
    return cellTile(p/1.5);
    
}

// Standard function-based bump mapping function.
vec3 doBumpMap(in vec3 p, in vec3 nor, float bumpfactor){
    
    const vec2 e = vec2(0.001, 0);
    float ref = bumpSurf3D(p, nor);                 
    vec3 grad = (vec3(bumpSurf3D(p - e.xyy, nor),
                      bumpSurf3D(p - e.yxy, nor),
                      bumpSurf3D(p - e.yyx, nor) )-ref)/e.x;                     
          
    grad -= nor*dot(nor, grad);          
                      
    return normalize( nor + grad*bumpfactor );
	
}

// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total. I tried to 
// make it as concise as possible. Whether that translates to speed, or not, I couldn't say.
vec3 doBumpMap( sampler2D tx, in vec3 p, in vec3 n, float bf){
   
    const vec2 e = vec2(0.001, 0);
    
    // Three gradient vectors rolled into a matrix, constructed with offset greyscale texture values.    
    mat3 m = mat3( tex3D(tx, p - e.xyy, n), tex3D(tx, p - e.yxy, n), tex3D(tx, p - e.yyx, n));
    
    vec3 g = vec3(0.299, 0.587, 0.114)*m; // Converting to greyscale.
    g = (g - dot(tex3D(tx,  p , n), vec3(0.299, 0.587, 0.114)) )/e.x; g -= n*dot(n, g);
                      
    return normalize( n + g*bf ); // Bumped normal. "bf" - bump factor.
    
}


float accum;



// Basic raymarcher.
float trace(in vec3 ro, in vec3 rd){
    
    accum = 0.;

    float t = 0.0, h;
    for(int i = 0; i < 160; i++){
    
        h = map(ro+rd*t);
        // Note the "t*b + a" addition. Basically, we're putting less emphasis on accuracy, as
        // "t" increases. It's a cheap trick that works in most situations... Not all, though.
        if(abs(h)<0.001*(t*.25 + 1.) || t>FAR) break; // Alternative: 0.001*max(t*.25, 1.)
        t += h;//*.7;
        
        if(abs(h)<0.25) accum += (.25-abs(h))/24.;///(1.+t);//.0005/abs(h);
        //if(abs(h)<0.25)accum += (.25-abs(h))*vec3(3, 2, 1)/4.*n3D((ro+rd*t)*16. - vec3(0, 0, 1)*iTime*1.);
        
    }

    return min(t, FAR);
    
}

/*
// Ambient occlusion, for that self shadowed look. Based on the original by XT95. I love this 
// function, and in many cases, it gives really, really nice results. For a better version, and 
// usage, refer to XT95's examples below:
//
// Hemispherical SDF AO - https://www.shadertoy.com/view/4sdGWN
// Alien Cocoons - https://www.shadertoy.com/view/MsdGz2
float calculateAO2( in vec3 p, in vec3 n )
{
	float ao = 0.0, l;
    const float maxDist = 2.;
	const float nbIte = 6.0;
	//const float falloff = 0.9;
    for( float i=1.; i< nbIte+.5; i++ ){
    
        l = (i*.75 + fract(cos(i)*45758.5453)*.25)/nbIte*maxDist;
        
        ao += (l - map( p + n*l ))/(1.+ l);// / pow(1.+l, falloff);
    }
	
    return clamp(1.- ao/nbIte, 0., 1.);
}
*/

// I keep a collection of occlusion routines... OK, that sounded really nerdy. :)
// Anyway, I like this one. I'm assuming it's based on IQ's original.
float calculateAO(in vec3 p, in vec3 n){
	
    float sca = 1., occ = 0.;
    for(float i=0.; i<5.; i++){
    
        float hr = .01 + i*.5/4.;        
        float dd = map(n * hr + p);
        occ += (hr - dd)*sca;
        sca *= 0.7;
    }
    return clamp(1.0 - occ, 0., 1.);    
}


// Tetrahedral normal, to save a couple of "map" calls. Courtesy of IQ. In instances where there's no descernible 
// aesthetic difference between it and the six tap version, it's worth using.
vec3 calcNormal(in vec3 p){

    // Note the slightly increased sampling distance, to alleviate artifacts due to hit point inaccuracies.
    vec2 e = vec2(0.001, -0.001); 
    return normalize(e.xyy*map(p + e.xyy) + e.yyx*map(p + e.yyx) + e.yxy*map(p + e.yxy) + e.xxx*map(p + e.xxx));
}

/*
// Standard normal function. 6 taps.
vec3 calcNormal(in vec3 p) {
	const vec2 e = vec2(0.002, 0);
	return normalize(vec3(map(p + e.xyy) - map(p - e.xyy), map(p + e.yxy) - map(p - e.yxy),	map(p + e.yyx) - map(p - e.yyx)));
}
*/

// Shadows.
float shadows(in vec3 ro, in vec3 rd, in float start, in float end, in float k){

    float shade = 1.0;
    const int shadIter = 24; 

    float dist = start;
    //float stepDist = end/float(shadIter);

    for (int i=0; i<shadIter; i++){
        float h = map(ro + rd*dist);
        shade = min(shade, k*h/dist);
        //shade = min(shade, smoothstep(0.0, 1.0, k*h/dist)); // Subtle difference. Thanks to IQ for this tidbit.

        dist += clamp(h, 0.02, 0.2);
        
        // There's some accuracy loss involved, but early exits from accumulative distance function can help.
        if ((h)<0.001 || dist > end) break; 
    }
    
    return min(max(shade, 0.) + 0.0, 1.0); 
}

//////
// Very basic pseudo environment mapping... and by that, I mean it's fake. :) However, it 
// does give the impression that the surface is reflecting the surrounds in some way.
//
// Anyway, the idea is very simple. Obtain the reflected (or refracted) ray at the surface 
// hit point, then index into a repeat texture in some way. It can be pretty convincing 
// (in an abstract way) and facilitates environment mapping without the need for a cube map, 
// or a reflective pass.
//
// More sophisticated environment mapping:
// UI easy to integrate - XT95    
// https://www.shadertoy.com/view/ldKSDm

vec3 envMap(vec3 rd, vec3 n){
    
    return tex3D(iChannel0, rd, n);
}



void mainImage( out vec4 fragColor, in vec2 fragCoord ){
	
	// Screen coordinates.
	vec2 uv = (fragCoord - iResolution.xy*0.5)/iResolution.y;
    
    #ifdef THREE_D
    float sg = sign(fragCoord.x - .5*iResolution.x);
    uv.x -= sg*.25*iResolution.x/iResolution.y;
    #endif
	
	// Camera Setup.
    vec3 camPos = vec3(0.0, 0.0, iTime*4.); // Camera position, doubling as the ray origin.

	vec3 lookAt = camPos + vec3(0, 0, 0.25);  // "Look At" position.

 
    // Light positioning. The positioning is fake. Obviously, the light source would be much 
    // further away, so illumination would be relatively constant and the shadows more static.
    // That's what direct lights are for, but sometimes it's nice to get a bit of a point light 
    // effect... but don't move it too close, or your mind will start getting suspicious. :)
 	vec3 lightPos = camPos + vec3(-10, 20, -20);


	// Using the Z-value to perturb the XY-plane.
	// Sending the camera, "look at," and two light vectors down the tunnel. The "path" function is 
	// synchronized with the distance function. Change to "path2" to traverse the other tunnel.
	lookAt.xy += path(lookAt.z);
	camPos.xy += path(camPos.z);
	//lightPos.xy += path(lightPos.z);
    
    
    #ifdef THREE_D
    camPos.x -= sg*.15; lookAt.x -= sg*.15; lightPos.x -= sg*.15;
    #endif
    
    

    // Using the above to produce the unit ray-direction vector.
    float FOV = 1.333;//PI/3.; // FOV - Field of view.
    vec3 forward = normalize(lookAt-camPos);
    vec3 right = normalize(vec3(forward.z, 0., -forward.x )); 
    vec3 up = cross(forward, right);

    // rd - Ray direction.
    vec3 rd = normalize(forward + FOV*uv.x*right + FOV*uv.y*up);
    
    // Lens distortion.
    //vec3 rd = (forward + FOV*uv.x*right + FOV*uv.y*up);
    //rd = normalize(vec3(rd.xy, rd.z - length(rd.xy)*.25));    
    
    // Swiveling the camera about the XY-plane (from left to right) when turning corners.
    // Naturally, it's synchronized with the path in some kind of way.
	rd.xy = rot2( path(lookAt.z).x/16. )*rd.xy;

    /*    
    // Mouse controls. I use them as a debugging device, but they can be used to look around. 
	vec2 ms = vec2(0);
    if (iMouse.z > 1.0) ms = (2.*iMouse.xy - iResolution.xy)/iResolution.xy;
    vec2 a = sin(vec2(1.5707963, 0) - ms.x); 
    mat2 rM = mat2(a, -a.y, a.x);
    rd.xz = rd.xz*rM; 
    a = sin(vec2(1.5707963, 0) - ms.y); 
    rM = mat2(a, -a.y, a.x);
    rd.yz = rd.yz*rM;
	*/
    
    // Standard ray marching routine. I find that some system setups don't like anything other than
    // a "break" statement (by itself) to exit. 
	float t = trace(camPos, rd);   
    
	
    // Initialize the scene color.
    vec3 sceneCol = vec3(0);
	
	// The ray has effectively hit the surface, so light it up.
	if(t<FAR){
	
   	
    	// Surface position and surface normal.
	    vec3 sp = camPos + rd*t;
        
        // Voxel normal.
        //vec3 sn = -(mask * sign( rd ));
        vec3 sn = calcNormal(sp);
        
        // Sometimes, it's necessary to save a copy of the unbumped normal.
        vec3 snNoBump = sn;
        
        // I try to avoid it, but it's possible to do a texture bump and a function-based
        // bump in succession. It's also possible to roll them into one, but I wanted
        // the separation... Can't remember why, but it's more readable anyway.
        //
        // Texture scale factor.
        const float tSize0 = 1./2.;
        
        
        // Function based bump mapping. Comment it out to see the under layer. It's pretty
        // comparable to regular beveled Voronoi... Close enough, anyway.
        sn = doBumpMap(sp, sn, .5);
        
        // Texture-based bump mapping.
	    sn = doBumpMap(iChannel0, sp*tSize0, sn, .1);//(-sign(sn.y)*.15+.85)*

        
    	// Light direction vectors.
	    vec3 ld = lightPos - sp;

        // Distance from respective lights to the surface point.
	    float lDist = max(length(ld), 0.001);
    	
    	// Normalize the light direction vectors.
	    ld /= lDist;
        
        // Shadows.
        float shading = shadows(sp + sn*.005, ld, .05, lDist, 8.);
        
	    // Ambient occlusion.
	    float ao = calculateAO(sp, sn);//*.75 + .25;

	    
        
	    // Light attenuation, based on the distances above.
	    float atten = 1./(1. + lDist*.007);
    	

    	
    	// Diffuse lighting.
	    float diff = max( dot(sn, ld), 0.0);
   	
    	// Specular lighting.
	    float spec = pow(max( dot( reflect(-ld, sn), -rd ), 0.0 ), 32.);

	    
	    // Fresnel term. Good for giving a surface a bit of a reflective glow.
        float fre = pow( clamp(dot(sn, rd) + 1., .0, 1.), 1.);
        
        // Ambient light, due to light bouncing around the the canyon.
	    float ambience = 0.35*ao + fre*fre*.25;

        // Object texturing, coloring and shading.
	    vec3 texCol = tex3D(iChannel0, sp*tSize0, sn);

        // Tones down the pinkish limestone\granite color.
        //texCol *= mix(vec3(.7, 1, 1.3), vec3(1), snNoBump.y*.5 + .5);
        
        #ifdef MOSS
        // Some quickly improvised moss.
        texCol = texCol*mix(vec3(1), vec3(.5, 1.5, 1.5), abs(snNoBump));
        texCol = texCol*mix(vec3(1), vec3(.6, 1, .5), pow(abs(sn.y), 4.));
        #else
        // Adding in the white frost. A bit on the cheap side, but it's a subtle effect.
        // As you can see, it's improvised, but from a physical perspective, you want the frost to accumulate
        // on the flatter surfaces, hence the "sn.y" factor. There's some Fresnel thrown in as well to give
        // it a tiny bit of sparkle.
        texCol = mix(texCol, vec3(.35, .55, 1)*(texCol*.5+.5)*vec3(2), ((snNoBump.y*.5 + sn.y*.5)*.5+.5)*pow(abs(sn.y), 4.)*texCol.r*fre*4.);
        #endif      

        
        // Final color. Pretty simple.
        sceneCol = texCol*(diff + spec + ambience);// + vec3(.2, .5, 1)*spec;
        
        // A bit of accumulated glow.
        sceneCol += texCol*((sn.y)*.5+.5)*min(vec3(1, 1.15, 1.5)*accum, 1.);  
     
        
        // Adding a touch of Fresnel for a bit of glow.
        sceneCol += texCol*vec3(.8, .95, 1)*pow(fre, 4.)*.5;
        
        
        // Faux environmental mapping. Adds a bit more ambience.        
        vec3 sn2 = snNoBump*.5 + sn*.5;
        vec3 ref = reflect(rd, sn2);//
        vec3 em = envMap(ref/2., sn2);
        ref = refract(rd, sn2, 1./1.31);
        vec3 em2 = envMap(ref/8., sn2);
        //sceneCol += ((sn.y)*.25+.75)*sceneCol*(em + em2);
        sceneCol += sceneCol*2.*(sn.y*.25+.75)*mix(em2, em, pow(fre, 4.));


	    // Shading. Adding some ambient occlusion to the shadow for some fake global lighting.
        sceneCol *= atten*min(shading + ao*.35, 1.)*ao;
	   
	
	}
    
       
    // Blend in a bit of light fog for atmospheric effect. I really wanted to put a colorful, 
    // gradient blend here, but my mind wasn't buying it, so dull, blueish grey it is. :)
    vec3 fog = vec3(.6, .8, 1.2)*(rd.y*.5 + .5);
    #ifdef MOSS
    fog *= vec3(1, 1.25, 1.5);
    #else
    #ifdef HOT
    fog *= 4.;
    #endif
    #endif
    sceneCol = mix(sceneCol, fog, smoothstep(0., .95, t/FAR)); // exp(-.002*t*t), etc. fog.zxy
    
    
    //sceneCol *= vec3(.5, .75, 1.5); // Nighttime vibe.
    #ifndef MOSS
    #ifdef HOT
    float gr = dot(sceneCol, vec3(.299, .587, .114)); // Grayscale.
    // A tiny portion of the original color blended with a very basic fire palette.
    sceneCol = sceneCol*.1 + pow(min(vec3(1.5, 1, 1)*gr*1.2, 1.), vec3(1, 3, 16));
    // Alternative artsy look. Comment out the line above first.
    //sceneCol = mix(sceneCol, pow(min(vec3(1.5, 1, 1)*gr*1.2, 1.), vec3(1, 3, 16)), -uv.y + .5);
    #endif
    #endif
    
    // Subtle, bluish vignette.
    uv = fragCoord/iResolution.xy;
    sceneCol = mix(vec3(0, .1, 1), sceneCol, pow( 16.0*uv.x*uv.y*(1.0-uv.x)*(1.0-uv.y) , .125)*.15 + .85);
    

    // Clamp and present the badly gamma corrected pixel to the screen.
	fragColor = vec4(sqrt(clamp(sceneCol, 0., 1.)), 1.0);
	
}