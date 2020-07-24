// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "SphericalCube/SphericalCube.h"
#include "scene/BaseSceneApp.h"

namespace FG
{

	//
	// Generated Planet Application
	//

	class GenPlanetApp final : public BaseSceneApp
	{
	// types
	private:
		using Mat3x3_t	= Matrix< float, 3, 3, EMatrixOrder::ColumnMajor, sizeof(float4) >;

		struct PlanetMaterial
		{
		};

		struct PlanetData
		{
			mat4x4			viewProj;
			vec4			position;
			vec2			clipPlanes;
			float			tessLevel;
			float			radius;

			vec3			lightDirection;
			float			_padding[1];

			//PlanetMaterial	materials [256];
		};


	// variables
	private:
		ImageID					_colorBuffer;
		ImageID					_depthBuffer;

		struct {
			SphericalCube			cube;
			ImageID					heightMap;
			ImageID					normalMap;
			ImageID					albedoMap;		// albedo, material id
			ImageID					emissionMap;	// temperature, emission
			BufferID				ubuffer;
			GPipelineID				pipeline;
			PipelineResources		resources;
		}						_planet;

		struct {
		}						_atmosphere;

		RawSamplerID			_linearSampler;
		RawSamplerID			_linearClampSampler;

		bool					_recreatePlanet	= true;
		bool					_showTimemap	= false;
		Optional<vec2>			_debugPixel;

		const float				_surfaceScale = 0.5f;

		
	// methods
	public:
		GenPlanetApp () {}
		~GenPlanetApp ();

		bool  Initialize ();


	// BaseSceneApp
	private:
		bool  DrawScene () override;
		

	// IWindowEventListener
	private:
		void  OnKey (StringView, EKeyAction) override;


	private:
		bool  _CreatePlanet (const CommandBuffer &);
		bool  _GenerateHeightMap (const CommandBuffer &);
		bool  _GenerateColorMap (const CommandBuffer &);
		
		ND_ static String  _LoadShader (StringView filename);
	};

}	// FG
