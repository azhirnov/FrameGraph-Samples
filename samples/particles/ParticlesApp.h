// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "BaseSample.h"

namespace FG
{

	//
	// Particle Render Application
	//

	class ParticlesApp final : public BaseSample
	{
	// types
	private:
		struct ParticlesUB
		{
			uint		steps;
			float		timeDelta;
			float		globalTime;
		};

		struct CameraUB
		{
			mat4x4		proj;
			mat4x4		modelView;
			mat4x4		modelViewProj;
			float2		viewport;
			float2		clipPlanes;
		};
		
		struct ParticleVertex
		{
			float3		position;
			float		size;
			float3		velocity;
			RGBA8u		color;
			float4		param;
		};

		enum class EParticleDrawMode : uint
		{
			Dots,
			Rays,
			Unknown		= ~0u,
		};

		enum class EBlendMode : uint
		{
			None,
			Additive,
			Unknown		= None,
		};


	// variables
	private:
		ImageID					_colorBuffer[2];
		ImageID					_depthBuffer;

		BufferID				_cameraUB[2];
		BufferID				_particlesUB;
		BufferID				_particlesBuf;
		
		CPipelineID				_updateParticlesPpln;
		PipelineResources		_updateParticlesRes;

		GPipelineID				_dotsParticlesPpln;
		GPipelineID				_raysParticlesPpln;
		PipelineResources		_drawParticlesRes;

		Optional<vec2>			_debugPixel;

		EBlendMode				_blendMode			= Default;
		EParticleDrawMode		_particleMode		= Default;
		uint					_numParticles;
		uint					_numSteps;

		uint					_curMode			= 1;
		uint					_newMode			= 1;

		bool					_initialized;
		bool					_reloadShaders;
		int						_sufaceScaleIdx		= 0;

		float					_timeScale			= 0.0f;
		Nanoseconds				_startTime;

		// config
		const uint				_maxSteps			= 512;
		const uint				_maxParticles 		= 1u << 22;
		const uint				_localSize			= 64;

		
	// methods
	public:
		ParticlesApp () {}
		~ParticlesApp ();

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
		void  _ReloadShaders (const CommandBuffer &cmdbuf);
		void  _DrawParticles (const CommandBuffer &cmdbuf, uint eye);
		void  _ResetPosition ();
		void  _ResetOrientation ();

		float _GetTimeStep () const;

		ND_ static String  _LoadShader (NtStringView filename)	{ return BaseSample::_LoadShader( String{FG_DATA_PATH} + filename.c_str()); }
	};

}	// FG
