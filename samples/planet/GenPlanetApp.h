// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "SphericalCube/SphericalCube.h"
#include "BaseSample.h"

namespace FG
{

	//
	// Generated Planet Application
	//

	class GenPlanetApp final : public BaseSample
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

		bool					_recreatePlanet	= true;
		bool					_showTimemap	= false;
		Optional<vec2>			_debugPixel;
		
		int						_sufaceScaleIdx		= 0;

		
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
		

	// BaseSample
	private:
		void  OnUpdateUI () override;


	private:
		bool  _CreatePlanet (const CommandBuffer &);
		bool  _GenerateHeightMap (const CommandBuffer &);
		bool  _GenerateColorMap (const CommandBuffer &);
		
		ND_ static String  _LoadShader (NtStringView filename)	{ return BaseSample::_LoadShader( String{FG_DATA_PATH} + filename.c_str()); }
	};

}	// FG
