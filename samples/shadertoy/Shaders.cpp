// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Shaders.h"
#include "stl/Algorithms/StringUtils.h"

namespace FG
{

	void Shaders::Shadertoy::Glowballs (Ptr<ShaderView> sv)
	{
		ShaderDescr sh_main;
		sh_main.Pipeline( "st_shaders/Glowballs.glsl" );
		sh_main.InChannel( "main", 0, sv->LinearClampSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}
	
	void Shaders::Shadertoy::PlasmaGlobe (Ptr<ShaderView> sv)
	{
		ShaderDescr sh_main;
		sh_main.Pipeline( "st_shaders/PlasmaGlobe.glsl" );
		sh_main.InChannel( "st_data/RGBANoiseMedium.png", 0, sv->MipmapRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}
	
	void Shaders::Shadertoy::SculptureIII (Ptr<ShaderView> sv)
	{
		ShaderDescr sh_main;
		sh_main.Pipeline( "st_shaders/Sculpture-III.glsl" );
		sh_main.InChannel( "st_data/Stars.jpg", 0, sv->MipmapRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::Shadertoy::Volcanic (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/Volcanic.glsl" );
		sh_main.InChannel( "st_data/RGBANoiseMedium.png", 0, sv->MipmapRepeatSampler() );
		sh_main.InChannel( "st_data/Lichen.jpg", 1, sv->MipmapRepeatSampler() );
		sh_main.InChannel( "st_data/Organic_2.jpg", 2, sv->MipmapRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::Shadertoy::CloudyTerrain (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/CloudyTerrain.glsl" );
		sh_main.InChannel( "st_data/GreyNoiseMedium.png", 0, sv->MipmapRepeatSampler() );
		sh_main.InChannel( "st_data/Abstract_1.jpg", 1, sv->MipmapRepeatSampler() );
		//sh_main.InChannel( "main", 2, sv->LinearClampSampler() );
		sh_main.InChannel( "st_data/RGBANoiseSmall.png", 3, sv->MipmapRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::Shadertoy::Canyon (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/Canyon.glsl" );
		sh_main.InChannel( "st_data/Organic_2.jpg", 0, sv->MipmapRepeatSampler() );
		sh_main.InChannel( "st_data/Abstract_1.jpg", 1, sv->MipmapRepeatSampler() );
		sh_main.InChannel( "st_data/RGBANoiseMedium.png", 2, sv->MipmapRepeatSampler() );
		sh_main.InChannel( "st_data/Lichen.jpg", 3, sv->MipmapRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::Shadertoy::StructuredVolSampling (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/StructuredVolSampling.glsl" );
		sh_main.InChannel( "st_data/RGBANoiseMedium.png", 0, sv->MipmapRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::Shadertoy::DesertPassage (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/DesertPassage.glsl" );
		sh_main.InChannel( "st_data/Organic_4.jpg", 0, sv->MipmapRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::Shadertoy::CanyonPass (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/CanyonPass.glsl" );
		sh_main.InChannel( "st_data/Organic_4.jpg", 0, sv->MipmapRepeatSampler(), true );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::Shadertoy::CloudFlight (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/CloudFlight.glsl" );
		sh_main.InChannel( "st_data/RGBANoiseMedium.png", 0, sv->MipmapRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::Shadertoy::DesertSand (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/DesertSand.glsl" );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::Shadertoy::Mesas (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/Mesas.glsl" );
		sh_main.InChannel( "st_data/RGBANoiseMedium.png", 0, sv->MipmapRepeatSampler() );
		sh_main.InChannel( "st_data/Organic_1.jpg", 1, sv->MipmapRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::Shadertoy::Organix (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_bufA;
		sh_bufA.Pipeline( "st_shaders/Organix_0.glsl" );
		sv->AddShader( "bufA", std::move(sh_bufA) );
		
		ShaderDescr	sh_bufB;
		sh_bufB.Pipeline( "st_shaders/Organix_1.glsl" );
		sh_bufB.InChannel( "bufA", 0, sv->LinearClampSampler() );
		sv->AddShader( "bufB", std::move(sh_bufB) );

		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/Organix_2.glsl" );
		sh_main.InChannel( "bufB", 0, sv->LinearClampSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::Shadertoy::Insect (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/Insect.glsl" );
		sh_main.InChannel( "st_data/Organic_2.jpg", 0, sv->MipmapRepeatSampler() );
		sh_main.InChannel( "st_data/GreyNoiseSmall.png", 1, sv->MipmapRepeatSampler() );
		sh_main.InChannel( "st_data/RGBANoiseSmall.png", 2, sv->MipmapRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::Shadertoy::AlienBeacon (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/AlienBeacon.glsl" );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::Shadertoy::ServerRoom (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/ServerRoom.glsl" );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::Shadertoy::Luminescence (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/Luminescence.glsl" );
		sv->AddShader( "main", std::move(sh_main) );
	}
//-----------------------------------------------------------------------------

	
	void Shaders::ShadertoyVR::Skyline (Ptr<ShaderView> sv)
	{
		ShaderDescr sh_main;
		sh_main.Pipeline( "st_shaders/Skyline.glsl" );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::ShadertoyVR::Catacombs (Ptr<ShaderView> sv)
	{
		ShaderDescr sh_main;
		sh_main.Pipeline( "st_shaders/Catacombs.glsl" );
		sh_main.InChannel( "st_data/Stars.jpg", 0, sv->MipmapRepeatSampler() );
		sh_main.InChannel( "st_data/RockTiles.jpg", 1, sv->MipmapRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}
	
	void Shaders::ShadertoyVR::DesertCanyon (Ptr<ShaderView> sv)
	{
		ShaderDescr sh_main;
		sh_main.Pipeline( "st_shaders/DesertCanyon.glsl" );
		sh_main.InChannel( "st_data/Pebbles.png", 0, sv->MipmapRepeatSampler() );
		sh_main.InChannel( "st_data/Organic_1.jpg", 1, sv->MipmapRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::ShadertoyVR::FrozenWasteland (Ptr<ShaderView> sv)
	{
		ShaderDescr sh_main;
		sh_main.Pipeline( "st_shaders/FrozenWasteland.glsl" );
		sh_main.InChannel( "st_data/RGBANoiseMedium.png", 0, sv->MipmapRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::ShadertoyVR::SirenianDawn (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/SirenianDawn.glsl" );
		sh_main.InChannel( "st_data/GreyNoiseMedium.png", 0, sv->MipmapRepeatSampler() );
		sh_main.InChannel( "main", 1, sv->LinearClampSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::ShadertoyVR::AtTheMountains (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/AtTheMountains.glsl" );
		sh_main.InChannel( "st_data/GreyNoiseMedium.png", 0, sv->LinearRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::ShadertoyVR::IveSeen (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/IveSeen.glsl" );
		sh_main.InChannel( "st_data/RustyMetal.jpg", 0, sv->LinearRepeatSampler() );
		sh_main.InChannel( "st_data/Abstract_2.jpg", 2, sv->LinearRepeatSampler() );
		sh_main.InChannel( "st_data/Organic_2.jpg", 3, sv->LinearRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::ShadertoyVR::NebulousTunnel (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/NebulousTunnel.glsl" );
		sh_main.InChannel( "st_data/RGBANoiseMedium.png", 0, sv->LinearRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::ShadertoyVR::CavePillars (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/CavePillars.glsl" );
		sh_main.InChannel( "st_data/Pebbles.png", 0, sv->LinearRepeatSampler() );
		sh_main.InChannel( "st_data/Abstract_3.jpg", 1, sv->MipmapRepeatSampler() );
		sh_main.InChannel( "st_data/Organic_2.jpg", 2, sv->MipmapRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}
	
	void Shaders::ShadertoyVR::ProteanClouds (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/ProteanClouds.glsl" );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::ShadertoyVR::PeacefulPostApocalyptic (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/PeacefulPostApocalyptic_0.glsl" );
		sh_main.InChannel( "st_data/Pebbles.png", 0, sv->MipmapRepeatSampler() );
		sh_main.InChannel( "st_data/RustyMetal.jpg", 1, sv->MipmapRepeatSampler() );
		sh_main.InChannel( "st_data/GreyNoiseSmall.png", 2, sv->MipmapRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::ShadertoyVR::OpticalDeconstruction (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/OpticalDeconstruction.glsl" );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::ShadertoyVR::FractalExplorer (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_bufB;
		sh_bufB.Pipeline( "st_shaders/FractalExplorer_1.glsl" );
		sh_bufB.InChannel( "st_data/Abstract_1.jpg", 0, sv->MipmapRepeatSampler() );
		sh_bufB.InChannel( "st_data/Organic_4.jpg", 1, sv->MipmapRepeatSampler() );
		sv->AddShader( "bufB", std::move(sh_bufB) );
		
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/FractalExplorer_2.glsl" );
		sh_main.InChannel( "bufB", 0, sv->LinearClampSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::ShadertoyVR::AncientMars (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( "st_shaders/AncientMars.glsl" );
		sh_main.InChannel( "st_data/RGBANoiseMedium.png", 0, sv->MipmapRepeatSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}

	void Shaders::ShadertoyVR::ThenAndBefore (Ptr<ShaderView> sv)
	{
		ShaderDescr sh_main;
		sh_main.Pipeline( "st_shaders/ThenAndBefore.glsl" );
		sh_main.InChannel( "main", 0, sv->LinearClampSampler() );
		sh_main.SetIPD( DefaultIPD * 1.5f );
		sv->AddShader( "main", std::move(sh_main) );
	}
//-----------------------------------------------------------------------------
	
	
	void Shaders::My::PrecalculatedRays (Ptr<ShaderView> sv)
	{
		const uint2	map_size	{ 64 };
		const uint	max_angles	= 128;
		const uint	radius		= 32;

		ShaderDescr sh_bufA;
		sh_bufA.Pipeline( "my_shaders/PrecalcRays_1.glsl" );
		sh_bufA.SetFormat( EPixelFormat::R8_UNorm );
		sh_bufA.SetDimension( map_size );
		sv->AddShader( "bufA", std::move(sh_bufA) );
			
		ShaderDescr sh_bufB;
		sh_bufB.Pipeline( "my_shaders/PrecalcRays_2.glsl", "#define RADIUS "s << ToString(radius) );
		sh_bufB.InChannel( "bufA", 0 );
		sh_bufB.SetFormat( EPixelFormat::R16F );
		sh_bufB.SetDimension( map_size );
		sv->AddShader( "bufB", std::move(sh_bufB) );

		ShaderDescr sh_bufC;
		sh_bufC.Pipeline( "my_shaders/PrecalcRays_3.glsl", "#define MAX_FRAMES "s << ToString(max_angles) << "\n#define RADIUS "s << ToString(radius) );
		sh_bufC.SetFormat( EPixelFormat::RGBA16F );
		sh_bufC.SetDimension( map_size * uint2(max_angles, 1) );
		sh_bufC.InChannel( "bufC", 0 );
		sh_bufC.InChannel( "bufB", 1 );
		sv->AddShader( "bufC", std::move(sh_bufC) );

		ShaderDescr sh_main;
		sh_main.Pipeline( "my_shaders/PrecalcRays_4.glsl", "#define ANGLE_COUNT "s << ToString(max_angles) );
		sh_main.InChannel( "bufA", 0 );
		sh_main.InChannel( "bufC", 1 );
		sv->AddShader( "main", std::move(sh_main) );
	}

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

	void Shaders::My::ThousandsOfStars (Ptr<ShaderView> sv)
	{
		ShaderDescr	sh_bufA;
		sh_bufA.Pipeline( "my_shaders/ThousandsOfStars_0.glsl" );
		sv->AddShader( "bufA", std::move(sh_bufA) );

		ShaderDescr	sh_main;
		sh_main.Pipeline( "my_shaders/ThousandsOfStars_1.glsl" );
		sh_main.InChannel( "bufA", 0, sv->LinearClampSampler() );
		sv->AddShader( "main", std::move(sh_main) );
	}
//-----------------------------------------------------------------------------


}	// FG
