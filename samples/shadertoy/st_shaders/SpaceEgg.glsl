/* space egg, by mattz
   License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

   Found a couple of nice uses for spherical voronoi textures in 2D & 3D. 
   See https://www.shadertoy.com/view/MtBGRD for technical details.

   Note the code below is not as nice or pretty as the linked shader MtBGRD.
   Please consider that one the "reference implementation".

 */


/* Magic angle that equalizes projected area of squares on sphere. */
#define MAGIC_ANGLE 0.868734829276 // radians

const float warp_theta = MAGIC_ANGLE;
float tan_warp_theta = tan(warp_theta);

/* Bunch o' globals. */
const float farval = 1e5;
const vec3 tgt = vec3(0);
const vec3 cpos = vec3(0,0,4.0);
const int rayiter = 60;
const float dmax = 20.0;
vec3 L = normalize(vec3(-0.7, 1.0, 0.5));
mat3 Rview;
const float nbumps = 6.0;
const float bump_pow = 2.0;
const float bump_scale = 18.0;


/* Return a permutation matrix whose first two columns are u and v basis 
   vectors for a cube face, and whose third column indicates which axis 
   (x,y,z) is maximal. */
mat3 getPT(in vec3 p) {

    vec3 a = abs(p);
    float c = max(max(a.x, a.y), a.z);    
    vec3 s = c == a.x ? vec3(1.,0,0) : c == a.y ? vec3(0,1.,0) : vec3(0,0,1.);
    s *= sign(dot(p, s));
    vec3 q = s.yzx;
    return mat3(cross(q,s), q, s);
}

/* For any point in 3D, obtain the permutation matrix, as well as grid coordinates
   on a cube face. */
void posToGrid(in float N, in vec3 pos, out mat3 PT, out vec2 g) {
    
    // Get permutation matrix and cube face id
    PT = getPT(pos);
    
    // Project to cube face
    vec3 c = pos * PT;     
    vec2 p = c.xy / c.z;      
    
    // Unwarp through arctan function
    vec2 q = atan(p*tan_warp_theta)/warp_theta; 
    
    // Map [-1,1] interval to [0,N] interval
    g = (q*0.5 + 0.5)*N;
    
}


/* For any grid point on a cube face, along with projection matrix, 
   obtain the 3D point it represents. */
vec3 gridToPos(in float N, in mat3 PT, in vec2 g) {
    
    // Map [0,N] to [-1,1]
    vec2 q = g/N * 2.0 - 1.0;
    
    // Warp through tangent function
    vec2 p = tan(warp_theta*q)/tan_warp_theta;

    // Map back through permutation matrix to place in 3D.
    return PT * vec3(p, 1.0);
    
}


/* Return whether a neighbor can be identified for a particular grid cell.
   We do not allow moves that wrap more than one face. For example, the 
   bottom-left corner (0,0) on the +X face may get stepped by (-1,0) to 
   end up on the -Y face, or, stepped by (0,-1) to end up on the -Z face, 
   but we do not allow the motion (-1,-1) from that spot. If a neighbor is 
   found, the permutation/projection matrix and grid coordinates of the 
   neighbor are computed.
*/
bool gridNeighbor(in float N, in mat3 PT, in vec2 g, in vec2 delta, out mat3 PTn, out vec2 gn) {

    vec2 g_dst = g.xy + delta;
    vec2 g_dst_clamp = clamp(g_dst, 0.0, N);

    vec2 extra = abs(g_dst_clamp - g_dst);
    float esum = extra.x + extra.y;
 
    if (max(extra.x, extra.y) == 0.0) {
        PTn = PT;
        gn = g_dst;
        return true;
    } else if (min(extra.x, extra.y) == 0.0 && esum < N) {
        vec3 pos = PT * vec3(g_dst_clamp/N*2.0-1.0, 1.0 - 2.0*esum/N);
        PTn = getPT(pos);
        gn = ((pos * PTn).xy*0.5 + 0.5) * N;
        return true;	        
    } else {
        return false;
    }
    

}


/* Return squared great circle distance of two points projected onto sphere. */
float sphereDist2(vec3 a, vec3 b) {
	// Fast-ish approximation for acos(dot(normalize(a), normalize(b)))^2
    return 2.0-2.0*dot((a),(b));
}


/* From https://www.shadertoy.com/view/Xd23Dh */
vec3 hash3( vec2 p )
{
    vec3 q = vec3( dot(p,vec2(127.1,311.7)), 
                  dot(p,vec2(269.5,183.3)), 
                  dot(p,vec2(419.2,371.9)) );
    return fract(sin(q)*43758.5453);
}

/* Magic bits. */
void voronoi(in vec3 pos, in float N,
             out vec4 pd1, out vec4 pd2) {

    mat3 PT;
    vec2 g;

    // Get grid coords
    posToGrid(N, pos, PT, g);   
    
    pd1 = vec4(farval);
    pd2 = vec4(farval);

    // Move to center of grid cell for neighbor calculation below.
    g = floor(g) + 0.5;
    
    // For each potential neighbor
    for (float u=-1.0; u<=1.0; ++u) {
        for (float v=-1.0; v<=1.0; ++v) {
            
            vec2 gn;
            mat3 PTn;

            // If neighbor exists
            if (gridNeighbor(N, PT, g, vec2(u,v), PTn, gn)) {
                
                float face = dot(PTn[2], vec3(1.,2.,3.));
                
                // Perturb based on grid cell ID
                gn = floor(gn);
                vec3 rn = hash3(gn*0.123 + face);
                gn += 0.5 + (rn.xy * 2.0 - 1.0)*0.5;

                // Get the 3D position
                vec3 pos_n = normalize(gridToPos(N, PTn, gn));                
                
                vec4 pd = vec4(pos_n, sphereDist2(pos, pos_n));
                                                            
                // See if new closest point (or second closest)
                if (pd.w < pd1.w) {
                    pd2 = pd1;
                    pd1 = pd;
                } else if (pd.w < pd2.w) {
                    pd2 = pd;
                }
                
            }
        }
    }       

}


/* Rotate about x-axis */
mat3 rotX(in float t) {
    float cx = cos(t), sx = sin(t);
    return mat3(1., 0, 0, 
                0, cx, sx,
                0, -sx, cx);
}


/* Rotate about y-axis */
mat3 rotY(in float t) {
    float cy = cos(t), sy = sin(t);
    return mat3(cy, 0, -sy,
                0, 1., 0,
                sy, 0, cy);

}

/* Distance to egg */
float map(in vec3 pos) {	

    float d = length(pos)-1.0;
    
    vec4 pd1, pd2;    

    if (abs(d) < 0.5) {
        pos = normalize(pos);
        voronoi(pos, nbumps, pd1, pd2);        
        pd1.w = pow(pd1.w, bump_pow);  
        pd2.w = pow(pd2.w, bump_pow);
        d += bump_scale * (pd1.w - pd2.w);
    }
    
    return d;

}


/* IQ's normal calculation. */
vec3 calcNormal( in vec3 pos ) {
    vec3 eps = vec3( 0.001, 0.0, 0.0 );
    vec3 nor = vec3(
        map(pos+eps.xyy) - map(pos-eps.xyy),
        map(pos+eps.yxy) - map(pos-eps.yxy),
        map(pos+eps.yyx) - map(pos-eps.yyx) );
    return normalize(nor);
}


/* IQ's distance marcher, more or less. */
float castRay( in vec3 ro, in vec3 rd, in float maxd ) {

    const float precis = 0.002;   
    float h=2.0*precis;

    float t = 0.0;

    for( int i=0; i<rayiter; i++ )
    {
        if( abs(h)<precis||t>maxd ) continue;//break;
        t += h;
        h = map( ro+rd*t );
       
    }    
    
    return t > maxd ? -1.0 : t;

}


vec3 fakeBlackBody(float t) {
    
    return (t < 0.3 ?
            mix(vec3(1.0, 0.0, 0.0), vec3(1.0, 1.0, 0.0), t/0.3) :
            t < 0.7 ?
            mix(vec3(1.0, 1.0, 0.0), vec3(1.0), (t-0.3)/0.4) :
            mix(vec3(1.0), vec3(0.5, 0.5, 1.0), (t-0.7)/0.3));
    
}

vec3 star(vec3 pos, vec4 pd, float scl) {
    
    vec3 rn = hash3(pd.xy+pd.z);
    vec3 k = 0.3*fakeBlackBody(rn.z) + 0.7;
                
    float s = exp(-sqrt(scl*pd.w/0.000004));
                
    return k*s*2.0*pow(rn.x,5.0);

}

vec3 stars(in vec3 rd, in float scl) {
    vec4 pd1, pd2;
    rd = normalize(rd);
    voronoi(rd, 50.0, pd1, pd2);    
    return star(rd, pd1, scl) + star(rd, pd2, scl);
}
    
float bisectorDistance(vec3 p, vec3 a, vec3 b) {
    vec3 n1 = cross(a,b);
    vec3 n2 = normalize(cross(n1, 0.5*((a)+(b))));
    return abs(dot(p, n2));             
}

vec3 shade( in vec3 ro, in vec3 rd ){

    float t = castRay(ro, rd, dmax);        

    vec3 c;
    vec4 pd1, pd2;


    if (t < 0.0) {

        c = stars(rd, 1.5);

    } else {        

        vec3 pos = ro + t*rd;
        vec3 n = calcNormal(pos);
        
        pos = normalize(pos);
        
        vec4 pd1, pd2;
        voronoi(pos, nbumps, pd1, pd2);    
        float d = bisectorDistance(pos, pd1.xyz, pd2.xyz);
        
        vec3 color = vec3(0.25, 0.23, 0.28);        
       
        vec3 diffamb = (0.9*clamp(dot(n,L), 0.0, 1.0)+0.1) * color;
        vec3 R = 2.0*n*dot(n,L)-L;
        float spec = 0.5*pow(clamp(-dot(R, rd), 0.0, 1.0), 20.0);
        c = diffamb + spec*vec3(1.0, 0.8, 0.9);
        
		c += 0.15*stars(reflect(rd, n), 0.5);
        
        vec3 k1 = rotX(iTime*0.7) * (rotY(iTime*0.3)*vec3(0, 0, 1.0));
        vec3 k2 = rotX(iTime*1.3) * (rotY(iTime*0.9)*vec3(0, 0, 1.0));
        
        float p = abs(dot(pos, k1)) * (1.0 -abs(dot(pos, k2)));
        p = 0.5 * p + 0.5;
        p *= 0.8 + 0.2*sin(iTime);

        c += exp(-pow(d,0.5)/0.12)*vec3(0.8, 0.4, 0.45)*p;

    }

    return c;

    

}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {

    vec2 uv = (fragCoord.xy - .5*iResolution.xy) * 0.8 / (iResolution.y);

    vec3 rz = normalize(tgt - cpos),
        rx = normalize(cross(rz,vec3(0,1.,0))),
        ry = cross(rx,rz);

    float t = iTime;
         
    float thetay = -t * 0.15;
    float thetax = t * 0.05;        

    if (max(iMouse.x, iMouse.y) > 20.0) { 
        thetax = (iMouse.y - .5*iResolution.y) * 5.0/iResolution.y; 
        thetay = (iMouse.x - .5*iResolution.x) * -10.0/iResolution.x; 
    }

    Rview = mat3(rx,ry,rz)*rotX(thetax)*rotY(thetay);        
  
    vec3 rd = Rview*normalize(vec3(uv, 1.)),
        ro = tgt + Rview*vec3(0,0,-length(cpos-tgt));

    fragColor.xyz = shade(ro, rd);


}
