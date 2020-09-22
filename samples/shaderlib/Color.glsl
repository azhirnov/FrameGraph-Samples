// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Color functions
*/

#include "Math.glsl"


float3  ToNonLinear (const float3 color)
{
	return Pow( color.rgb, float3(2.2f) ); 
}

float4  ToNonLinear (const float4 color)
{
	return float4(ToNonLinear( color.rgb ), color.a ); 
}

float3  ToLinear (const float3 color)
{
	return Pow( color.rgb, float3(0.454545f) ); 
}

float4  ToLinear (const float4 color)
{
	return float4(ToLinear( color.rgb ), color.a ); 
}


float3  HSVtoRGB (const float3 hsv)
{
	// from http://chilliant.blogspot.ru/2014/04/rgbhsv-in-hlsl-5.html
	float3 col = float3( Abs( hsv.x * 6.0 - 3.0 ) - 1.0,
						 2.0 - Abs( hsv.x * 6.0 - 2.0 ),
						 2.0 - Abs( hsv.x * 6.0 - 4.0 ));
	return (( Clamp( col, float3(0.0), float3(1.0) ) - 1.0 ) * hsv.y + 1.0 ) * hsv.z;
}

float3  Rainbow (float factor)
{
	return HSVtoRGB( float3(RemapClamped( float2(0.0, 1.45), float2(0.0, 1.0), 1.0 - factor ), 1.0, 1.0) );
}
