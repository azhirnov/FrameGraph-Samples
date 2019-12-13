// from https://www.shadertoy.com/view/ltfBzM
/*

    Nebulous Tunnel
    ---------------

    Volumetrically raymarching an organic distance field to produce a tunnel winding
    through a nebulous substrate... I'm not entirely sure how to describe it, but I
    guess it resembles a microbial dust flythough. :) Either way, there's nothing new 
    here - I just thought it'd be fun to make.	

    There are two common ways to apply volumetrics to a distance field: One is to
    step evenly through the field accumulating layer colors at each point. Layers 
    are assigned a weight according to the distance from the viewer. 

    The other method is to sphere-trace to the surface, then accumulate color once
    the ray crosses a certain surface distance threshold. The color weighing is 
    calculated according to distance from the surface.  

    The first method gives an overall gaseous kind of effect. The latter method 
    displays more of the underlying surface and is the one I'm using here. You can 
    also use hybrids of the two. There's no right or wrong way to do it. It all 
    depends on what you're trying to achieve.

    Volumetric calculations are generally more expensive, due to the number of steps
    involved, and the need to calculate lighting at each one. In a perfect world, 
    you'd calculate normals (numerically, or analytically, if possible), etc, to give 
    better results. Unfortunately, that's expensive, so it's common to use a cheap 
    (directional derivative) lighting trick to get the job done. If you're interested, 
    IQ explains it here:

    Directional Derivative - www.iquilezles.org/www/articles/derivative/derivative.htm

    I've tried to keep the GPU workload down do a dull roar, but that was at the
    expense of quality via various detail sacrifices. However, hopefully, it will run 
    at a reasonal pace on moderate systems. By the way, if you prefer a slighly more 
    conventional look, uncomment the "WHITE_FLUFFY_CLOUDS" and the "BETWEEN_LAYERS" 
    define.
    
    
    Based on:
    
    Cloudy Spikeball - Duke
    https://www.shadertoy.com/view/MljXDw
    // Port from a demo by Las - Worth watching.
    // http://www.pouet.net/topic.php?which=7920&page=29&x=14&y=9

*/

#include "RayTracing.glsl"

#define FAR 50.

// More conventional look. Probably more pleasing to the eye, but a vacuum cleaner 
// dust flythrough was the look I was going for. :D
//#define WHITE_FLUFFY_CLOUDS

// A between cloud layers look. Works better with the white clouds. Needs work, but
// it's there to give you a different perspective.
//#define BETWEEN_LAYERS

// Fabrice's concise, 2D rotation formula.
//mat2 r2(float th){ vec2 a = sin(vec2(1.5707963, 0) + th); return mat2(a, -a.y, a.x); }
// Standard 2D rotation formula - See Nimitz's comment.
mat2 r2(in float a){ float c = cos(a), s = sin(a); return mat2(c, s, -s, c); }

// Hash function. This particular one probably doesn't disperse things quite 
// as nicely as some of the others around, but it's compact, and seems to work.
//
vec3 hash33(vec3 p){ 
    float n = sin(dot(p, vec3(7, 157, 113)));    
    return fract(vec3(2097152, 262144, 32768)*n); 
}


// IQ's texture lookup noise... in obfuscated form. There's less writing, so
// that makes it faster. That's how optimization works, right? :) Seriously,
// though, refer to IQ's original for the proper function.
// 
// By the way, you could replace this with the non-textured version, and the
// shader should run at almost the same efficiency.
float n3D( in vec3 p ){
    
    //return texture(iChannel 1, p/24., 0.25).x;
    
    vec3 i = floor(p); p -= i; p *= p*(3. - 2.*p);
    p.xy = texture(iChannel0, (p.xy + i.xy + vec2(37, 17)*i.z + .5)/256., -100.).yx;
    return mix(p.x, p.y, p.z);
}

/*
// Textureless 3D Value Noise:
//
// This is a rewrite of IQ's original. It's self contained, which makes it much
// easier to copy and paste. I've also tried my best to minimize the amount of 
// operations to lessen the work the GPU has to do, but I think there's room for
// improvement. I have no idea whether it's faster or not. It could be slower,
// for all I know, but it doesn't really matter, because in its current state, 
// it's still no match for IQ's texture-based, smooth 3D value noise.
//
// By the way, a few people have managed to reduce the original down to this state, 
// but I haven't come across any who have taken it further. If you know of any, I'd
// love to hear about it.
//
// I've tried to come up with some clever way to improve the randomization line
// (h = mix(fract...), but so far, nothing's come to mind.
float n3D(vec3 p){
    
    // Just some random figures, analogous to stride. You can change this, if you want.
    const vec3 s = vec3(7, 157, 113);
    
    vec3 ip = floor(p); // Unique unit cell ID.
    
    // Setting up the stride vector for randomization and interpolation, kind of. 
    // All kinds of shortcuts are taken here. Refer to IQ's original formula.
    vec4 h = vec4(0., s.yz, s.y + s.z) + dot(ip, s);
    
    p -= ip; // Cell's fractional component.
    
    // A bit of cubic smoothing, to give the noise that rounded look.
    p = p*p*(3. - 2.*p);
    
    // Smoother version of the above. Weirdly, the extra calculations can sometimes
    // create a surface that's easier to hone in on, and can actually speed things up.
    // Having said that, I'm sticking with the simpler version above.
    //p = p*p*p*(p*(p * 6. - 15.) + 10.);
    
    // Even smoother, but this would have to be slower, surely?
    //vec3 p3 = p*p*p; p = ( 7. + ( p3 - 7. ) * p ) * p3;	
    
    // Cosinusoidal smoothing. OK, but I prefer other methods.
    //p = .5 - .5*cos(p*3.14159);
    
    // Standard 3D noise stuff. Retrieving 8 random scalar values for each cube corner,
    // then interpolating along X. There are countless ways to randomize, but this is
    // the way most are familar with: fract(sin(x)*largeNumber).
    h = mix(fract(sin(h)*43758.5453), fract(sin(h + s.x)*43758.5453), p.x);
    
    // Interpolating along Y.
    h.xy = mix(h.xz, h.yw, p.y);
    
    // Interpolating along Z, and returning the 3D noise value.
    return mix(h.x, h.y, p.z); // Range: [0, 1].
    
}
*/


// Basic low quality noise consisting of three layers of rotated, mutated 
// trigonometric functions. Needs work, but sufficient for this example.
float trigNoise3D(in vec3 p){

    
    float res = 0., sum = 0.;
    
    // IQ's cheap, texture-lookup noise function. Very efficient, but still 
    // a little too processor intensive for multiple layer usage in a largish 
    // "for loop" setup. Therefore, just a couple of layers are used here.
    //float n = n3D(p*8. + iTime*.2); // Not great.
    float n = n3D(p*6. + iTime*.2)*.67 +  n3D(p*12. + iTime*.4)*.33; // Compromise.
    // Nicer, but I figured too many layers was pushing it. :)
    //float n = n3D(p*6. + iTime*.2)*.57 +  n3D(p*12. + iTime*.4)*.28 +  n3D(p*24. + iTime*.8)*.15;


    // Two sinusoidal layers. I'm pretty sure you could get rid of one of 
    // the swizzles (I have a feeling the GPU doesn't like them as much), 
    // which I'll try to do later.
    
    vec3 t = sin(p.yzx*3.14159265 + cos(p.zxy*3.14159265 + 3.14159265/4.))*.5 + .5;
    p = p*1.5 + (t - 1.5); //  + iTime*0.1
    res += (dot(t, vec3(0.333)));

    t = sin(p.yzx*3.14159265 + cos(p.zxy*3.14159265 + 3.14159265/4.))*.5 + .5;
    res += (dot(t, vec3(0.333)))*0.7071; 
    
     
    return ((res/1.7071))*.85 + n*.15;
}



// Smooth maximum, based on IQ's smooth minimum.
float smax(float a, float b, float s){
    
    float h = clamp(.5 + .5*(a - b)/s, 0., 1.);
    return mix(b, a, h) + h*(1. - h)*s;
}

// The path is a 2D sinusoid that varies over time, depending upon the frequencies, and amplitudes.
vec2 path(in float z){ 

    //return vec2(0); // Straight path.
    return vec2(sin(z*.075)*8., cos(z*.1)*.75); // Windy path.
    
}

// Distance function.
float map(vec3 p) {
    
    // Cheap and nasty fBm emulation. Two noise layers and a couple of sinusoidal layers.
    // Sadly, you get what you pay for. :) Having said that, it works fine here.
    float tn = trigNoise3D(p*.5);
    
    // Mapping the tunnel around the path.
    p.xy -= path(p.z);
    
    #ifndef BETWEEN_LAYERS
    // Smoothly carve out the windy tunnel path from the nebulous substrate.
    return smax(tn - .025, -length(p.xy) + .25, 2.);
    #else
    // Between cloud layers... I guess. The "trigNoise3D" function above would have
    // to be reworked to look more like clouds, but it gives you a rough idea.
    return smax(tn - .05, -abs(p.y - sign(p.y)*(tn - .5)) + .125, 2.);
    #endif


}

/*
// Tetrahedral normal, to save a couple of "map" calls. Courtesy of IQ.
vec3 fNorm(in vec3 p){

    // Note the large sampling distance.
    vec2 e = vec2(0.005, -0.005); 
    return normalize(e.xyy * map(p + e.xyy) + e.yyx * map(p + e.yyx) + e.yxy * map(p + e.yxy) + e.xxx * map(p + e.xxx));
}
*/

// Less accurate 4 tap (3 extra taps, in this case) normal calculation. Good enough for this example.
vec3 fNorm(in vec3 p, float d){
    
    // Note the large sampling distance.
    vec2 e = vec2(.01, 0); 

    // Return the normal.
    return normalize(vec3(d - map(p - e.xyy), d - map(p - e.yxy), d - map(p - e.yyx)));
}

vec4 RayTrace (const Ray ray, const vec2 fragCoord)
{
    // Screen coordinates.
    vec2 uv = (fragCoord - iResolution.xy*.5)/iResolution.y;

    // Ray origin. Moving along the Z-axis.
    vec3 ro = vec3(0, 0, iTime*4.);
    vec3 lk = ro + vec3(0, 0, .25);  // "Look At" position.
    
        // Using the Z-value to perturb the XY-plane.
    // Sending the camera, "look at," and light vector down the path. The "path" function is 
    // synchronized with the distance function.
    ro.xy += path(ro.z);
    lk.xy += path(lk.z);

    
    // Using the above to produce the unit ray-direction vector.
    float FOV = 3.14159/2.75; // FOV - Field of view.
    vec3 forward = normalize(lk-ro);
    vec3 right = normalize(vec3(forward.z, 0., -forward.x )); 
    vec3 up = cross(forward, right);

    // rd - Ray direction.
    vec3 rd = normalize(forward + FOV*uv.x*right + FOV*uv.y*up);
    //rd = normalize(vec3(rd.xy, rd.z - length(rd.xy)*.15));

#if 1   // manual camera
    ro += ray.origin;
    rd = ray.dir;
#endif
    
    // Camera swivel - based on path position.
    vec2 sw = path(lk.z);
    rd.xy *= r2(-sw.x/24.);
    rd.yz *= r2(-sw.y/16.);
    

    // The ray is effectively marching through discontinuous slices of noise, so at certain
    // angles, you can see the separation. A bit of randomization can mask that, to a degree.
    // At the end of the day, it's not a perfect process. Anyway, the hash below is used to
    // at jitter to the jump off point (ray origin).
    //    
    // It's also used for some color based jittering inside the loop.
    vec3 rnd = hash33(rd.yzx + fract(iTime));

    // Local density, total density, and weighting factor.
    float lDen = 0., td = 0., w = 0.;

    // Closest surface distance, a second sample distance variable, and total ray distance 
    // travelled. Note the comparitively large jitter offset. Unfortunately, due to cost 
    // cutting (64 largish steps, it was  necessary to get rid of banding.
    float d = 1., d2 = 0., t = dot(rnd, vec3(.333));

    // Distance threshold. Higher numbers give thicker clouds, but fill up the screen too much.    
    const float h = .5;


    // Initializing the scene color to black, and declaring the surface position vector.
    vec3 col = vec3(0), sp;
    
    // Directional light. Don't quote me on it, but I think directional derivative lighting
    // only works with unidirectional light... Thankfully, the light source is the cun which 
    // tends to be unidirectional anyway.
    vec3 ld = normalize(vec3(-.2, .3, .4));
    
    // Using the light position to produce a blueish sky and sun. Pretty standard.
    vec3 sky = vec3(.6, .8, 1.)*min((1.5+rd.y*.5)/2., 1.); 	
    sky = mix(vec3(1, 1, .9), vec3(.31, .42, .53), rd.y*0.5 + 0.5);
    //sky = mix(vec3(1, .8, .7), vec3(.31, .52, .73), rd.y*0.5 + 0.5);
    
    
    // Sun position in the sky - Note that the sun has been cheated down a little lower for 
    // aesthetic purposes. All this is fake anyway.
    float sun = clamp(dot(normalize(vec3(-.2, .3, .4*4.)), rd), 0.0, 1.0);
    
    
    // Combining the clouds, sky and sun to produce the final color.
    sky += vec3(1, .3, .05)*pow(sun, 5.)*.25; 
    sky += vec3(1, .4, .05)*pow(sun, 16.)*.35; 


    // Raymarching loop.
    for (int i=0; i<64; i++) {


        sp = ro + rd*t; // Current ray position.
        d = map(sp); // Closest distance to the surface... particle.
        
        // Loop break conditions - If the ray hits the surface, the accumulated density maxes out,
        // or if the total ray distance goes beyong the maximum, break.
        if(d<.001*(1. + t*.125) || td>1. || t>FAR) break;


        // If we get within a certain distance, "h," of the surface, accumulate some surface values.
        //
        // Values further away have less influence on the total. When you accumulate layers, you'll
        // usually need some kind of weighting algorithm based on some identifying factor - in this
        // case, it's distance. This is one of many ways to do it. In fact, you'll see variations on 
        // the following lines all over the place.
        //
        // On a side note, you could wrap the next few lines in an "if" statement to save a
        // few extra "map" calls, etc. However, some cards hate branching, nesting, etc, so it
        // could be a case of diminishing returns... Not sure what the right call is, so I'll 
        // leave it to the experts. :)
        w = d<h? (1. - td)*(h - d) : 0.;   

        // Use the weighting factor to accumulate density. How you do this is up to you. 
        //td += w*w*8. + 1./60.; // More transparent looking... kind of.
        td += w + 1./64.; // Looks cleaner, but a little washed out.

        
       
        // Lighting calculations.
        // Standard diffuse calculation using a cheap 4 tap normal. "d" is passed in, so that means 
        // only 3 extra taps. It's more expensive than 2 tap (1 extra tap) directional derivative
        // lighting. However, it will work with point light, and enables better lighting.
        //float diff = max(dot(ld, fNorm(sp, d)), 0.);
        
        // Directional derivative-based diffuse calculation. Uses only two function taps,
        // but only works with unidirectional light. By the way, the "1 + t*.125" is a fake
        // tweak to hightlight further down the tunnel, but it doesn't belong there. :)
        d2 = map(sp + ld*.02*(1. + t*.125));
        // Possibly quicker than the line above, but I feel it overcomplicates things... Maybe. 
        //d2 = d<h? map(sp + ld*.02*(1. + t*.125)) : d;
        float diff = max(d2 - d, 0.)*50.*(1. + t*.125);
        //float diff = max(d2*d2 - d*d, 0.)*100.; // Slightly softer diffuse effect.
        


        // Accumulating the color. You can do this any way you like.
        //
        #ifdef WHITE_FLUFFY_CLOUDS
        // I wanted dust, but here's something fluffier - The effects you can create are endless.
        col += w*d*(diff*vec3(1, .85, .7) + 2.5)*1.25;
        // Other variations - Tweak them to suit your needs.
        //col += w*d*(sqrt(diff)*vec3(1, .85, .7)*2. + 3.);
        //col += w*d*((1.-exp(-diff*8.))*vec3(1, .85, .7)*1.5 + 2.5);
        
        
        #else
        col += w*d*(.5 + diff*vec3(1, .8, .6)*4.);
        #endif
        
        // Optional extra: Color-based jittering. Roughens up the clouds that hit the camera lens.
        col *= .98 + fract(rnd*289. + t*41.13)*.04;

        // Enforce minimum stepsize. This is probably the most important part of the procedure.
        // It reminds me a little of of the soft shadows routine.
        t += max(d*.5, .05); //
        //t += 0.25; // t += d*.5;// These also work - It depends what you're trying to achieve.

    }
    
    col = max(col, 0.);
    
    
    #ifndef WHITE_FLUFFY_CLOUDS
    // Adding a bit of a firey tinge to the volumetric substance.
    col = mix(pow(vec3(1.3, 1, 1)*col, vec3(1, 2, 10)), col, dot(cos(rd*6. +sin(rd.yzx*6.)), vec3(.333))*.2 + .8);
    #endif
 
    
    
    // Fogging out the volumetric substance. The fog blend is heavier than usual. It was a style
    // choice - Not sure if was the right one though. :)
    col = mix(col, sky, smoothstep(0., .55, t/FAR));
    col += vec3(1, .4, .05)*pow(sun, 16.)*.25; 	
    
    // Tweaking the contrast.
    col = pow(col, vec3(1.5));

    
    // Subtle vignette.
    //uv = fragCoord/iResolution.xy;
    //col *= pow(16.*uv.x*uv.y*(1. - uv.x)*(1. - uv.y) , .25)*.35 + .65;
    // Colored varation.
    //col = mix(pow(min(vec3(1.5, 1, 1).zyx*col, 1.), vec3(1, 3, 16)), col, 
              //pow(16.*uv.x*uv.y*(1. - uv.x)*(1. - uv.y) , .125)*.5 + .5);
 
    // Done.
    return vec4(sqrt(min(col, 1.)), 1.0);
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
