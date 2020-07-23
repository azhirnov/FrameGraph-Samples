// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_control_flow_attributes : require
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (local_size_x_id = 0, local_size_y_id = 1, local_size_z = 1) in;

#include "simulation_shared.glsl"

layout(set=0, binding=0, std430) buffer ParticleSSB
{
	Particle	particles[];
};

layout(set=0, binding=1, std140) uniform ParticleUB
{
	uint		steps;
	float		timeDelta;
	float		globalTime;
} ub;


void main ()
{
	UpdateParticle( INOUT particles[GetGlobalIndex()], ub.timeDelta, ub.steps, ub.globalTime );
}
