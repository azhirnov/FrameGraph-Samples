/*
    Fog functions.
*/

#include "Math.glsl"


float  FogFactorLinear (const float dist, const float start, const float end)
{
    // from https://github.com/hughsk/glsl-fog
    return 1.0 - Clamp( (end - dist) / (end - start), 0.0, 1.0 );
}

float  FogFactorExp (const float dist, const float density)
{
    // from https://github.com/hughsk/glsl-fog
    return 1.0 - Clamp( Exp( -density * dist ), 0.0, 1.0 );
}

float  FogFactorExp2 (const float dist, const float density)
{
    // from https://github.com/hughsk/glsl-fog
    const float LOG2 = -1.442695;
    float   d = density * dist;
    return  1.0 - Clamp( Exp2( d * d * LOG2 ), 0.0, 1.0 );
}
