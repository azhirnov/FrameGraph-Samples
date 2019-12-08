// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Shaders.h"
#include "stl/Algorithms/StringUtils.h"

namespace FG
{

	void Shaders::Shadertoy::Glowballs (Ptr<ShaderView> sv)
	{
		ShaderDescr sh_main;
		sh_main.Pipeline( "st_shaders/Glowballs.glsl" );
		sh_main.InChannel( "main", 0 );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::Shadertoy::SirenianDawn (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_bufA;
		sh_bufA.Pipeline( "st_shaders/SirenianDawn_1.glsl" );
		sh_bufA.InChannel( "st_data/GreyNoiseMedium.png", 0, sv->LinearRepeatSampler() );
		sh_bufA.InChannel( "bufA", 1 );
		sv->AddShader( "bufA", std::move(sh_bufA) );

		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/SirenianDawn_2.glsl" );
		sh_main.InChannel( "bufA", 0 );
		sv->AddShader( "main", std::move(sh_main) );
	}
//-----------------------------------------------------------------------------


	void Shaders::ShadertoyVR::Catacombs (Ptr<ShaderView> sv)
	{
		ShaderDescr sh_main;
		sh_main.Pipeline( "st_shaders/Catacombs.glsl" );
		sh_main.InChannel( "st_data/Stars.jpg", 0, sv->LinearRepeatSampler() );
		sh_main.InChannel( "st_data/RockTiles.jpg", 1, sv->LinearRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}
	
	void Shaders::ShadertoyVR::DesertCanyon (Ptr<ShaderView> sv)
	{
		ShaderDescr sh_main;
		sh_main.Pipeline( "st_shaders/DesertCanyon.glsl" );
		sh_main.InChannel( "st_data/Pebbles.png", 0, sv->LinearRepeatSampler() );
		sh_main.InChannel( "st_data/Organic_1.jpg", 1, sv->LinearRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::ShadertoyVR::FrozenWasteland (Ptr<ShaderView> sv)
	{
		ShaderDescr sh_main;
		sh_main.Pipeline( "st_shaders/FrozenWasteland.glsl" );
		sh_main.InChannel( "st_data/RGBANoiseMedium.png", 0, sv->LinearRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}
//-----------------------------------------------------------------------------


	void Shaders::My::OptimizedSDF (Ptr<ShaderView> sv)
	{
		const uint2	map_size{ 128 };
		const uint	radius	= 32;

		ShaderDescr sh_bufA;
		sh_bufA.Pipeline( "my_shaders/OptSDF_1.glsl" );
		sh_bufA.SetFormat( EPixelFormat::R8_UNorm );
		sh_bufA.SetDimension( map_size );
		sv->AddShader( "bufA", std::move(sh_bufA) );

		ShaderDescr sh_bufB;
		sh_bufB.Pipeline( "my_shaders/OptSDF_2.glsl", "#define RADIUS "s << ToString(radius) );
		sh_bufB.InChannel( "bufA", 0 );
		sh_bufB.SetFormat( EPixelFormat::RG32F );
		sh_bufB.SetDimension( map_size );
		sv->AddShader( "bufB", std::move(sh_bufB) );
			
		ShaderDescr sh_main;
		sh_main.Pipeline( "my_shaders/OptSDF_3.glsl", "#define RADIUS "s << ToString(radius) );
		sh_main.InChannel( "bufA", 0 );
		sh_main.InChannel( "bufB", 1 );
		sv->AddShader( "main", std::move(sh_main) );
	}

}	// FG
