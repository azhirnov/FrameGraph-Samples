// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Default math types and functions
*/

#define float2		vec2
#define float3		vec3
#define float4		vec4
#define float2x2	mat2x2
#define float3x3	mat3x3
#define float4x4	mat4x4

#define double2		dvec2
#define double3		dvec3
#define double4		dvec4
#define double2x2	dmat2x2
#define double3x3	dmat3x3
#define double4x4	dmat4x4

#define int2		ivec2
#define int3		ivec3
#define int4		ivec4

#define uint2		uvec2
#define uint3		uvec3
#define uint4		uvec4

#define bool2		bvec2
#define bool3		bvec3
#define bool4		bvec4

#define and			&&
#define or			||

#define Any				any
#define All				all
#define Abs				abs
#define ACos			acos
#define ASin			asin
#define ASinH			asinh
#define ACosH			acosh
#define ATan			atan			// result in range [-Pi...+Pi]
#define BitScanReverse	findMSB
#define BitScanForward	findLSB
#define ATanH			atanh
#define Clamp			clamp
#define Ceil			ceil
#define Cos				cos
#define CosH			cosh
#define Cross			cross
#define Distance		distance
#define Dot				dot
#define Exp				exp
#define Exp2			exp2
#define Fract			fract
#define Floor			floor
#define FusedMulAdd		fma		// (a * b) + c
#define IsNaN			isnan
#define IsInfinity		isinf
#define IsFinite( x )	(! IsInfinity( x ) && ! IsNaN( x ))
#define InvSqrt			inversesqrt
#define IntLog2			BitScanReverse
#define Length			length
#define Lerp			mix
#define Ln				log
#define Log2			log2
#define Log( x, base )	(Ln(x) / Ln(base))
#define Log10( x )		(Ln(x) * 0.4342944819032518)
#define Min				min
#define Max				max
#define Mod				mod
#define MatInverse		inverse
#define MatTranspose	transpose
#define Normalize		normalize
#define Pow				pow
#define Round			round
#define Reflect			reflect
#define Refract			refract
#define Step			step
#define SmoothStep		smoothstep
#define Saturate( x )	clamp( x, 0.0, 1.0 )
#define Sqrt			sqrt
#define Sin				sin
#define SinH			sinh
#define SignOrZero		sign
#define Tan				tan
#define TanH			tanh
#define Trunc			trunc
#define ToUNorm( x )	((x) * 0.5 + 0.5)
#define ToSNorm( x )	((x) * 2.0 - 1.0)


// to mark 'out' and 'inout' argument in function call
// in function argument list use defined by GLSL qualificators: in, out, inout
#define OUT
#define INOUT


//-----------------------------------------------------------------------------
// Comparison

#define Less			lessThan			// <
#define Greater			greaterThan			// >
#define LessEqual		lessThanEqual		// <=
#define GreaterEqual	greaterThanEqual	// >=
#define Not				not
#define not				!

bool  Equals (const float  lhs, const float  rhs)  { return lhs == rhs; }
bool2 Equals (const float2 lhs, const float2 rhs)  { return equal( lhs, rhs ); }
bool3 Equals (const float3 lhs, const float3 rhs)  { return equal( lhs, rhs ); }
bool4 Equals (const float4 lhs, const float4 rhs)  { return equal( lhs, rhs ); }

bool  Equals (const int  lhs, const int  rhs)  		{ return lhs == rhs; }
bool2 Equals (const int2 lhs, const int2 rhs)  		{ return equal( lhs, rhs ); }
bool3 Equals (const int3 lhs, const int3 rhs)  		{ return equal( lhs, rhs ); }
bool4 Equals (const int4 lhs, const int4 rhs)  		{ return equal( lhs, rhs ); }

bool  Equals (const uint  lhs, const uint  rhs)  	{ return lhs == rhs; }
bool2 Equals (const uint2 lhs, const uint2 rhs)  	{ return equal( lhs, rhs ); }
bool3 Equals (const uint3 lhs, const uint3 rhs)  	{ return equal( lhs, rhs ); }
bool4 Equals (const uint4 lhs, const uint4 rhs)  	{ return equal( lhs, rhs ); }

bool  Equals (const double  lhs, const double  rhs)  { return lhs == rhs; }
bool2 Equals (const double2 lhs, const double2 rhs)  { return equal( lhs, rhs ); }
bool3 Equals (const double3 lhs, const double3 rhs)  { return equal( lhs, rhs ); }
bool4 Equals (const double4 lhs, const double4 rhs)  { return equal( lhs, rhs ); }

#define AllLess( a, b )			All( Less( (a), (b) ))
#define AllLessEqual( a, b )	All( LessEqual( (a), (b) ))

#define AllGreater( a, b )		All( Greater( (a), (b) ))
#define AllGreaterEqual( a, b )	All( GreaterEqual( (a), (b) ))

#define AnyLess( a, b )			Any( Less( (a), (b) ))
#define AnyLessEqual( a, b )	Any( LessEqual( (a), (b) ))

#define AnyGreater( a, b )		Any( Greater( (a), (b) ))
#define AnyGreaterEqual( a, b )	Any( GreaterEqual( (a), (b) ))

#define AllEqual( a, b )		All( Equals( (a), (b) ))
#define AnyEqual( a, b )		Any( Equals( (a), (b) ))

#define AllNotEqual( a, b )		All( Not( Equals( (a), (b) )))
#define AnyNotEqual( a, b )		Any( Not( Equals( (a), (b) )))

#define NotAllEqual( a, b )		!All( Equals( (a), (b) ))
#define NotAnyEqual( a, b )		!Any( Equals( (a), (b) ))

#define All2( a, b )			All(bool2( (a), (b) ))
#define All3( a, b, c )			All(bool3( (a), (b), (c) ))
#define All4( a, b, c, d )		All(bool4( (a), (b), (c), (d) ))

#define Any2( a, b )			Any(bool2( (a), (b) ))
#define Any3( a, b, c )			Any(bool3( (a), (b), (c) ))
#define Any4( a, b, c, d )		Any(bool4( (a), (b), (c), (d) ))

#define Min3( a, b, c )			Min( Min( (a), (b) ), (c) )
#define Min4( a, b, c, d )		Min( Min( (a), (b) ), Min( (c), (d) ))
#define Max3( a, b, c )			Max( Max( (a), (b) ), (c) )
#define Max4( a, b, c, d )		Max( Max( (a), (b) ), Max( (c), (d) ))


//-----------------------------------------------------------------------------
// Constants

float  Epsilon ()		{ return 0.00001f; }
float  Pi ()			{ return 3.14159265358979323846f; }
float  Pi2 ()			{ return Pi() * 2.0; }
float  ReciporalPi ()	{ return 0.31830988618379067153f; }
float  SqrtOf2 ()		{ return 1.41421356237309504880f; }


//-----------------------------------------------------------------------------

float  Sign (const float x)  { return  x < 0.0 ? -1.0 : 1.0; }
int    Sign (const int x)    { return  x < 0 ? -1 : 1; }


float2  SinCos (const float x)  { return float2(sin(x), cos(x)); }


//-----------------------------------------------------------------------------
// Square

float  Square (const float x)	{ return x * x; }
float2 Square (const float2 x)	{ return x * x; }
float3 Square (const float3 x)	{ return x * x; }
float4 Square (const float4 x)	{ return x * x; }

int    Square (const int x)		{ return x * x; }
int2   Square (const int2 x)	{ return x * x; }
int3   Square (const int3 x)	{ return x * x; }
int4   Square (const int4 x)	{ return x * x; }

uint   Square (const uint x)	{ return x * x; }
uint2  Square (const uint2 x)	{ return x * x; }
uint3  Square (const uint3 x)	{ return x * x; }
uint4  Square (const uint4 x)	{ return x * x; }


//-----------------------------------------------------------------------------
// square length and distance

float Length2 (const float2 x)  { return Dot( x, x ); }
float Length2 (const float3 x)  { return Dot( x, x ); }

float Distance2 (const float2 x, const float2 y)  { float2 r = x - y;  return Dot( r, r ); }
float Distance2 (const float3 x, const float3 y)  { float3 r = x - y;  return Dot( r, r ); }


//-----------------------------------------------------------------------------
// clamp / wrap

float ClampOut (const float x, const float minVal, const float maxVal)
{
	float mid = (minVal + maxVal) * 0.5;
	return x < mid ? (x < minVal ? x : minVal) : (x > maxVal ? x : maxVal);
}

int ClampOut (const int x, const int minVal, const int maxVal)
{
	int mid = (minVal+1)/2 + (maxVal+1)/2;
	return x < mid ? (x < minVal ? x : minVal) : (x > maxVal ? x : maxVal);
}

float2 ClampOut (const float2 v, const float minVal, const float maxVal) {
	return float2( ClampOut( v.x, minVal, maxVal ),
				   ClampOut( v.y, minVal, maxVal ));
}

float3 ClampOut (const float3 v, const float minVal, const float maxVal) {
	return float3( ClampOut( v.x, minVal, maxVal ),
				   ClampOut( v.y, minVal, maxVal ),
				   ClampOut( v.z, minVal, maxVal ));
}

float4 ClampOut (const float4 v, const float minVal, const float maxVal) {
	return float4( ClampOut( v.x, minVal, maxVal ),
				   ClampOut( v.y, minVal, maxVal ),
				   ClampOut( v.z, minVal, maxVal ),
				   ClampOut( v.w, minVal, maxVal ));
}

int2 ClampOut (const int2 v, const float minVal, const float maxVal) {
	return int2( ClampOut( v.x, minVal, maxVal ),
				 ClampOut( v.y, minVal, maxVal ));
}

int3 ClampOut (const int3 v, const float minVal, const float maxVal) {
	return int3( ClampOut( v.x, minVal, maxVal ),
				 ClampOut( v.y, minVal, maxVal ),
				 ClampOut( v.z, minVal, maxVal ));
}

int4 ClampOut (const int4 v, const float minVal, const float maxVal) {
	return int4( ClampOut( v.x, minVal, maxVal ),
				 ClampOut( v.y, minVal, maxVal ),
				 ClampOut( v.z, minVal, maxVal ),
				 ClampOut( v.w, minVal, maxVal ));
}


float Wrap (const float x, const float minVal, const float maxVal)
{
	if ( maxVal < minVal ) return minVal;
	float size = maxVal - minVal;
	float res = minVal + Mod( x - minVal, size );
	if ( res < minVal ) return res + size;
	return res;
}

int Wrap (const int x, const int minVal, const int maxVal)
{
	if ( maxVal < minVal ) return minVal;
	int size = maxVal+1 - minVal;
	int res = minVal + ((x - minVal) % size);
	if ( res < minVal ) return res + size;
	return res;
}

float2 Wrap (const float2 v, const float minVal, const float maxVal) {
	return float2( Wrap( v.x, minVal, maxVal ),
				   Wrap( v.y, minVal, maxVal ));
}

float3 Wrap (const float3 v, const float minVal, const float maxVal) {
	return float3( Wrap( v.x, minVal, maxVal ),
				   Wrap( v.y, minVal, maxVal ),
				   Wrap( v.z, minVal, maxVal ));
}

float4 Wrap (const float4 v, const float minVal, const float maxVal) {
	return float4( Wrap( v.x, minVal, maxVal ),
				   Wrap( v.y, minVal, maxVal ),
				   Wrap( v.z, minVal, maxVal ),
				   Wrap( v.w, minVal, maxVal ));
}

float2 Wrap (const float2 v, const float2 minVal, const float2 maxVal) {
	return float2( Wrap( v.x, minVal.x, maxVal.y ),
				   Wrap( v.y, minVal.x, maxVal.y ));
}

float3 Wrap (const float3 v, const float3 minVal, const float3 maxVal) {
	return float3( Wrap( v.x, minVal.x, maxVal.x ),
				   Wrap( v.y, minVal.y, maxVal.y ),
				   Wrap( v.z, minVal.z, maxVal.z ));
}

float4 Wrap (const float4 v, const float4 minVal, const float4 maxVal) {
	return float4( Wrap( v.x, minVal.x, maxVal.x ),
				   Wrap( v.y, minVal.y, maxVal.y ),
				   Wrap( v.z, minVal.z, maxVal.z ),
				   Wrap( v.w, minVal.w, maxVal.w ));
}

int2 Wrap (const int2 v, const float minVal, const float maxVal) {
	return int2( Wrap( v.x, minVal, maxVal ),
				 Wrap( v.y, minVal, maxVal ));
}

int3 Wrap (const int3 v, const float minVal, const float maxVal) {
	return int3( Wrap( v.x, minVal, maxVal ),
				 Wrap( v.y, minVal, maxVal ),
				 Wrap( v.z, minVal, maxVal ));
}

int4 Wrap (const int4 v, const float minVal, const float maxVal) {
	return int4( Wrap( v.x, minVal, maxVal ),
				 Wrap( v.y, minVal, maxVal ),
				 Wrap( v.z, minVal, maxVal ),
				 Wrap( v.w, minVal, maxVal ));
}


//-----------------------------------------------------------------------------
// bit operations

int BitRotateLeft  (const int x, uint shift)
{
	const uint mask = 31u;
	shift = shift & mask;
	return (x << shift) | (x >> ( ~(shift-1u) & mask ));
}

uint BitRotateLeft  (const uint x, uint shift)
{
	const uint mask = 31u;
	shift = shift & mask;
	return (x << shift) | (x >> ( ~(shift-1u) & mask ));
}


int BitRotateRight (const int x, uint shift)
{
	const uint mask = 31u;
	shift = shift & mask;
	return (x >> shift) | (x << ( ~(shift-1u) & mask ));
}

uint BitRotateRight (const uint x, uint shift)
{
	const uint mask = 31u;
	shift = shift & mask;
	return (x >> shift) | (x << ( ~(shift-1u) & mask ));
}


//-----------------------------------------------------------------------------
// interpolation

float   BaryLerp (const float  a, const float  b, const float  c, const float3 barycentrics)  { return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z; }
float2  BaryLerp (const float2 a, const float2 b, const float2 c, const float3 barycentrics)  { return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z; }
float3  BaryLerp (const float3 a, const float3 b, const float3 c, const float3 barycentrics)  { return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z; }
float4  BaryLerp (const float4 a, const float4 b, const float4 c, const float3 barycentrics)  { return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z; }


float   BiLerp (const float  x1y1, const float  x2y1, const float  x1y2, const float  x2y2, const float2 factor)  { return Lerp( Lerp( x1y1, x2y1, factor.x ), Lerp( x1y2, x2y2, factor.x ), factor.y ); }
float2  BiLerp (const float2 x1y1, const float2 x2y1, const float2 x1y2, const float2 x2y2, const float2 factor)  { return Lerp( Lerp( x1y1, x2y1, factor.x ), Lerp( x1y2, x2y2, factor.x ), factor.y ); }
float3  BiLerp (const float3 x1y1, const float3 x2y1, const float3 x1y2, const float3 x2y2, const float2 factor)  { return Lerp( Lerp( x1y1, x2y1, factor.x ), Lerp( x1y2, x2y2, factor.x ), factor.y ); }
float4  BiLerp (const float4 x1y1, const float4 x2y1, const float4 x1y2, const float4 x2y2, const float2 factor)  { return Lerp( Lerp( x1y1, x2y1, factor.x ), Lerp( x1y2, x2y2, factor.x ), factor.y ); }


// map 'x' in 'src' interval to 'dst' interval
float   Remap (const float2 src, const float2 dst, const float  x)  { return (x - src.x) / (src.y - src.x) * (dst.y - dst.x) + dst.x; }
float2  Remap (const float2 src, const float2 dst, const float2 x)  { return (x - src.x) / (src.y - src.x) * (dst.y - dst.x) + dst.x; }
float3  Remap (const float2 src, const float2 dst, const float3 x)  { return (x - src.x) / (src.y - src.x) * (dst.y - dst.x) + dst.x; }
float4  Remap (const float2 src, const float2 dst, const float4 x)  { return (x - src.x) / (src.y - src.x) * (dst.y - dst.x) + dst.x; }

// map 'x' in 'src' interval to 'dst' interval and clamp
float   RemapClamped (const float2 src, const float2 dst, const float  x)  { return Clamp( Remap( src, dst, x ), dst.x, dst.y ); }
float2  RemapClamped (const float2 src, const float2 dst, const float2 x)  { return Clamp( Remap( src, dst, x ), dst.x, dst.y ); }
float3  RemapClamped (const float2 src, const float2 dst, const float3 x)  { return Clamp( Remap( src, dst, x ), dst.x, dst.y ); }
float4  RemapClamped (const float2 src, const float2 dst, const float4 x)  { return Clamp( Remap( src, dst, x ), dst.x, dst.y ); }
