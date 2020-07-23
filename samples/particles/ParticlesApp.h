// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/BaseSceneApp.h"
#include "ui/ImguiRenderer.h"

namespace FG
{

	//
	// Particle Render Application
	//

	class ParticlesApp final : public BaseSceneApp
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

		using KeyStates_t	= StaticArray< EKeyAction, 3 >;


	// variables
	private:
		ImageID					_colorBuffer1;
		ImageID					_colorBuffer2;
		ImageID					_depthBuffer;

		RawSamplerID			_linearSampler;
		RawSamplerID			_linearClampSampler;

		BufferID				_cameraUB;
		BufferID				_particlesUB;
		BufferID				_particlesBuf;
		
		CPipelineID				_updateParticlesPpln;
		PipelineResources		_updateParticlesRes;

		GPipelineID				_dotsParticlesPpln;
		GPipelineID				_raysParticlesPpln;
		PipelineResources		_drawParticlesRes;

		Optional<vec2>			_debugPixel;

		EBlendMode				_blendMode		= Default;
		EParticleDrawMode		_particleMode	= Default;
		uint					_numParticles;
		uint					_numSteps;

		uint					_curMode		= 1;
		uint					_newMode		= 1;

		bool					_initialized;
		bool					_reloadShaders;
		bool					_halfSurfaceSize	= true;
		bool					_settingsWndOpen	= true;

		float					_timeScale		= 0.0f;
		Nanoseconds				_startTime;

		// config
		const uint				_maxSteps		= 512;
		const uint				_maxParticles 	= 1u << 22;
		const uint				_localSize		= 64;
		
		#ifdef FG_ENABLE_IMGUI
		ImguiRenderer			_uiRenderer;
		KeyStates_t				_mouseJustPressed;
		#endif

		
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


	private:
		void  _ReloadShaders (const CommandBuffer &cmdbuf);
		void  _DrawParticles (const CommandBuffer &cmdbuf, RawImageID colorBuffer);
		void  _ResetPosition ();
		void  _ResetOrientation ();
		
		bool  _UpdateUI ();
		bool  _UpdateInput ();

		float _GetTimeStep () const;

		ND_ static String  _LoadShader (StringView filename);
	};

}	// FG
