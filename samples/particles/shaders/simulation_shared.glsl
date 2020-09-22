// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Math.glsl"
#include "Hash.glsl"
#include "AABB.glsl"
#include "GlobalIndex.glsl"
#include "Color.glsl"


//-----------------------------------------------------------------------------
// Types

struct Particle
{
	float3		position;
	float		size;
	float3		velocity;
	uint		color;
	float4		param;
};

struct GravityObject
{
	float3		position;
	float		gravity;
	float		radius;
};

struct MagneticObject
{
	float3		north;
	float3		south;
	float		induction;
};

struct Emitter
{
	float3		position;
	float		gravity;
	float		radius;
	float		induction;
};

void  InitParticle (out Particle particle, const float globalTime);
void  UpdateParticle (inout Particle outParticle, const float stepTime, const uint steps, const float globalTime);


//-----------------------------------------------------------------------------
// Utils

float3  GravityAccel (const float3 position, const float3 center, const float gravity)
{
	const float3 v = center - position;
	return Normalize( v ) * gravity / Dot( v, v );
}


float3  LinearMagneticFieldAccel (const float3 velocity, const float3 magneticInduction)
{
	// lorentz force
	return Cross( velocity, magneticInduction );
}


float3  SphericalMagneticFieldAccel (const float3 velocity, const float3 postion,
								     const float3 northPos, const float3 southPos,
								     const float induction)
{
	const float3	nv = postion - northPos;
	const float3	n  = Normalize( nv ) * induction / Dot( nv, nv );
	const float3	sv = southPos - postion;
	const float3	s  = Normalize( sv ) * induction / Dot( sv, sv );
	return LinearMagneticFieldAccel( velocity, n + s );
}


void  UniformlyAcceleratedMotion (inout float3 pos, inout float3 vel, const float3 accel, const float dt)
{
	pos += vel * dt * 0.5;
	vel += accel * dt;
	pos += vel * dt * 0.5;
}


uint  ParticleColor_FromNormalizedVelocity (const float3 velocity)
{
	return packUnorm4x8( float4( ToUNorm( Normalize( velocity )), 1.0 ));
}


uint  ParticleColor_FromVelocity (const float3 velocity)
{
	return packUnorm4x8( float4( ToUNorm( Clamp( velocity, -1.0, 1.0 )), 1.0 ));
}


uint  ParticleColor_FromVelocityLength (const float3 velocity)
{
	const float vel = 1.0 - Clamp( Length( velocity ), 0.0, 1.0 );
	return packUnorm4x8( float4( HSVtoRGB( float3( vel, 1.0, 1.0 )), 1.0 ));
}


float3  ParticleEmitter_Plane (const float pointIndex, const float pointsCount)
{
	const float side = Sqrt( pointsCount );
	return float3( ToSNorm( float2(Mod( pointIndex, side ), Floor( pointIndex / side )) / side ), 0.0 );
}


float3  ParticleEmitter_Plane (const float pointIndex, const float pointsCount, const float ratio)
{
	const float	side_x	 = Sqrt( pointsCount * ratio );
	const float	side_y	 = pointsCount / side_x;
	const float	max_side = Max( side_x, side_y );

	return float3( (float2(Mod( pointIndex, side_x ), Floor( pointIndex / side_x )) * 2.0 - float2(side_x, side_y)) / max_side, 0.0 );
}


float3  ParticleEmitter_Circle (const float pointIndex, const float pointsCount)
{
	return float3( SinCos( Pi() * 2.0 * pointIndex / (pointsCount - 1.0) ), 0.0 );
}


float3  ParticleEmitter_FillCircle (const float pointIndex, const float pointsCount)
{
	const float2	p = ParticleEmitter_Plane( pointIndex, pointsCount ).xy;
	return float3( SinCos( Pi() * 2.0 * p.x ) * p.y, 0.0 );
}


float3  ParticleEmitter_Sphere (const float pointIndex, const float pointsCount)
{
	const float2	angle	= ParticleEmitter_Plane( pointIndex, pointsCount, 0.5 ).yx * Pi();
	const float2	theta	= SinCos( angle.x );
	const float2	phi		= SinCos( angle.y );

	return float3( theta.x * phi.y, theta.x * phi.x, theta.y );
}


float3  ParticleEmitter_ConeVector (const float pointIndex, const float pointsCount, const float zLength)
{
	const float2	p = ParticleEmitter_Plane( pointIndex, pointsCount ).xy;
	const float2	c = SinCos( Pi() * 2.0 * p.x ) * p.y;
	return Normalize( float3( c, zLength ));
}
//-----------------------------------------------------------------------------


#if MODE == 1

	const GravityObject		g_GravityObjects[1] = {
		{ float3( 0.0 ), 0.1, 0.1 }
	};

	const MagneticObject	g_MagneticObjects[1] = {
		{ float3( 0.0, 0.0, 0.1 ), float3( 0.0, 0.0, -0.1 ), 0.5 }
	};

	const AABB				g_BoundingBox	= { float3(-10.0), float3(10.0) };


	void  RestartParticle (out Particle particle, const float globalTime)
	{
		float	index	= float(GetGlobalIndex());
		float	size	= float(GetGlobalIndexSize());
		float	vel		= 0.5;

		particle.position	= AABB_GetPointInBox( g_BoundingBox, float3( 0.1, 0.0, 0.0 ));
		particle.size		= 8.0;
		particle.color		= 0xFFFFFFFF;
		particle.velocity	= ParticleEmitter_ConeVector( index, size, 1.0 ).zxy * -vel;
		particle.param.x	= Sign(ToSNorm( DHash12(float2( globalTime, index / size )) ));	// sign
	}


	void  InitParticle (out Particle particle, const float globalTime)
	{
		RestartParticle( particle, DHash11( globalTime + GetGlobalIndexUNorm() * 1.6543324 ));
	}


	void  UpdateParticle (inout Particle outParticle, const float stepTime, const uint steps, const float globalTime)
	{
		for (uint t = 0; t < steps; ++t)
		{
			float3	accel		= float3(0.0);
			int		destroyed	= 0;
			float	sign		= outParticle.param.x;
		
			for (int i = 0; i < g_GravityObjects.length(); ++i)
			{
				accel		+= GravityAccel( outParticle.position, g_GravityObjects[i].position, g_GravityObjects[i].gravity );
				destroyed	+= int( Distance( outParticle.position, g_GravityObjects[i].position ) < g_GravityObjects[i].radius );
			}
		
			for (int i = 0; i < g_MagneticObjects.length(); ++i)
			{
				accel += SphericalMagneticFieldAccel( outParticle.velocity, outParticle.position,
													  g_MagneticObjects[i].north, g_MagneticObjects[i].south,
													  g_MagneticObjects[i].induction ) * sign;
			}

			UniformlyAcceleratedMotion( INOUT outParticle.position, INOUT outParticle.velocity, accel, stepTime );

			if ( not AABB_IsInside( g_BoundingBox, outParticle.position ) or destroyed > 0 )
			{
				RestartParticle( outParticle, globalTime );
			}
		}

		outParticle.color = ParticleColor_FromVelocityLength( outParticle.velocity );
	}

#endif
//-----------------------------------------------------------------------------

	
#if MODE == 3
	
	const GravityObject		g_GravityObjects[1] = {
		{ float3( 0.0 ), 0.1, 0.05 }
	};
	const float3			g_MagneticField	= float3( 1.0, 0.0, 0.0 );
	const AABB				g_BoundingBox	= { float3(-10.0), float3(10.0) };


	void  RestartParticle (out Particle particle, const float globalTime)
	{
		float	index	= float(GetGlobalIndex());
		float	size	= float(GetGlobalIndexSize());
		float	vel		= 0.5;

		particle.position	= AABB_GetPointInBox( g_BoundingBox, float3( 0.1, 0.0, 0.0 ));
		particle.size		= 8.0;
		particle.color		= 0xFFFFFFFF;
		particle.velocity	= ParticleEmitter_ConeVector( index, size, 1.0 ) * vel;
		particle.param.x	= Sign(ToSNorm( DHash12(float2( globalTime, index / size )) ));	// sign
		particle.param.y	= 0.0;
		particle.param.z	= 0.0;
	}

	
	void  InitParticle (out Particle particle, const float globalTime)
	{
		RestartParticle( particle, DHash11( globalTime + GetGlobalIndexUNorm() * 1.6543324 ));
	}

	
	void  UpdateParticle (inout Particle outParticle, const float stepTime, const uint steps, const float globalTime)
	{
		for (uint t = 0; t < steps; ++t)
		{
			float3	accel		= float3(0.0);
			int		destroyed	= 0;
			float	sign		= outParticle.param.x;
		
			for (int i = 0; i < g_GravityObjects.length(); ++i)
			{
				accel		+= GravityAccel( outParticle.position, g_GravityObjects[i].position, g_GravityObjects[i].gravity );
				destroyed	+= int( Distance( outParticle.position, g_GravityObjects[i].position ) < g_GravityObjects[i].radius );
			}

			accel += LinearMagneticFieldAccel( outParticle.velocity, g_MagneticField ) * sign;
			
			UniformlyAcceleratedMotion( INOUT outParticle.position, INOUT outParticle.velocity, accel, stepTime );

			outParticle.param.z	 += stepTime;
			outParticle.param.y	  = globalTime;

			if ( not AABB_IsInside( g_BoundingBox, outParticle.position ) or destroyed > 0 )
			{
				RestartParticle( outParticle, globalTime );
			}
		}

		outParticle.color = ParticleColor_FromVelocityLength( outParticle.velocity );
	}

#endif
//-----------------------------------------------------------------------------

	
#if MODE == 4
	
	const GravityObject		g_GravityObjects[1] = {
		{ float3( 0.0 ), 0.1, 0.1 }
	};

	const MagneticObject	g_MagneticObjects[1] = {
		{ float3( 0.0, 0.0, 0.1 ), float3( 0.0, 0.0, -0.1 ), 0.5 }
	};

	const AABB				g_BoundingBox	= { float3(-10.0), float3(10.0) };


	void  RestartParticle (out Particle particle, const float globalTime)
	{
		float	index	= float(GetGlobalIndex());
		float	size	= float(GetGlobalIndexSize());
		float	vel		= 0.5;

		particle.position	= AABB_GetPointInBox( g_BoundingBox, float3( 0.1, 0.0, 0.0 ));
		particle.size		= 8.0;
		particle.color		= 0xFFFFFFFF;
		particle.velocity	= ParticleEmitter_ConeVector( index, size, 1.0 ).zxy * -vel;
		particle.param.x	= Sign(ToSNorm( DHash12(float2( globalTime, index / size )) ));	// sign
	}

	
	void  InitParticle (out Particle particle, const float globalTime)
	{
		RestartParticle( particle, DHash11( globalTime + GetGlobalIndexUNorm() * 1.6543324 ));
	}

	
	void  UpdateParticle (inout Particle outParticle, const float stepTime, const uint steps, const float globalTime)
	{
		for (uint t = 0; t < steps; ++t)
		{
			float3	accel		= float3( 0.0 );
			int		destroyed	= 0;
			float	sign		= outParticle.param.x;
		
			for (int i = 0; i < g_GravityObjects.length(); ++i)
			{
				accel		+= GravityAccel( outParticle.position, g_GravityObjects[i].position, g_GravityObjects[i].gravity );
				destroyed	+= int( Distance( outParticle.position, g_GravityObjects[i].position ) < g_GravityObjects[i].radius );
			}
		
			for (int i = 0; i < g_MagneticObjects.length(); ++i)
			{
				accel += SphericalMagneticFieldAccel( outParticle.velocity, outParticle.position,
													  g_MagneticObjects[i].north, g_MagneticObjects[i].south,
													  g_MagneticObjects[i].induction ) * sign;
			}
			
			UniformlyAcceleratedMotion( INOUT outParticle.position, INOUT outParticle.velocity, accel, stepTime );

			if ( not AABB_IsInside( g_BoundingBox, outParticle.position ) or destroyed > 0 )
			{
				RestartParticle( outParticle, globalTime );
			}
		}

		outParticle.color = ParticleColor_FromNormalizedVelocity( outParticle.velocity );
	}

#endif
//-----------------------------------------------------------------------------

	
#if MODE == 5
	
	const MagneticObject	g_MagneticObjects[2] = {
		{ float3( 0.0, 0.0, 0.9 ), float3( 0.0, 0.0,-0.9 ), 0.1 },
		{ float3( 0.0, 0.9, 0.0 ), float3( 0.0,-0.9, 0.0 ), 0.3 }
	};

	const AABB				g_BoundingBox	= { float3(-10.0), float3(10.0) };


	void  RestartParticle (out Particle particle, const float globalTime)
	{
		float	index	= float(GetGlobalIndex());
		float	size	= float(GetGlobalIndexSize());
		float	vel		= 0.05;
		float	rnd		= DHash12(float2( index / size, globalTime + 2.28374 )) * size;

		particle.position	= ParticleEmitter_Sphere( rnd, size ) * 1.0;
		particle.size		= 8.0;
		particle.color		= 0xFFFFFFFF;
		particle.velocity	= Normalize( particle.position ) * vel;
		particle.param.x	= Sign(ToSNorm( DHash12(float2( globalTime, index / size )) ));	// sign
	}

	
	void  InitParticle (out Particle particle, const float globalTime)
	{
		RestartParticle( particle, DHash11( globalTime + GetGlobalIndexUNorm() * 1.6543324 ));
	}

	
	void  UpdateParticle (inout Particle outParticle, const float stepTime, const uint steps, const float globalTime)
	{
		for (uint t = 0; t < steps; ++t)
		{
			float3	accel	= float3( 0.0 );
			float	sign	= outParticle.param.x;
		
			for (int i = 0; i < g_MagneticObjects.length(); ++i)
			{
				accel += SphericalMagneticFieldAccel( outParticle.velocity, outParticle.position,
													  g_MagneticObjects[i].north, g_MagneticObjects[i].south,
													  g_MagneticObjects[i].induction ) * sign;
			}
			
			UniformlyAcceleratedMotion( INOUT outParticle.position, INOUT outParticle.velocity, accel, stepTime );

			if ( not AABB_IsInside( g_BoundingBox, outParticle.position ))
			{
				RestartParticle( outParticle, globalTime );
			}
		}

		outParticle.color = ParticleColor_FromNormalizedVelocity( outParticle.velocity );
	}

#endif
//-----------------------------------------------------------------------------
