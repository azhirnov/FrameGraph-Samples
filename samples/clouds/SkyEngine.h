/*
	Based on code from https://github.com/mccannd/Project-Marshmallow
	For more information see 'LICENSE'
*/

#pragma once

#include "BaseSample.h"
#include "Texture.h"
#include "Geometry.h"
#include "Shader.h"

namespace FG
{

	//
	// Sky Engine
	//

	class SkyEngine final : public BaseSample
	{
	// variables
	private:
		SkyManager				_skySystem;

		// 
		UniquePtr<Texture>		_meshTexture;
		UniquePtr<Texture>		_meshPBRInfo;
		UniquePtr<Texture>		_meshNormals;
		UniquePtr<Texture>		_backgroundTexture;
		UniquePtr<Texture>		_backgroundTexturePrev;
		UniquePtr<Texture>		_depthTexture;
		UniquePtr<Texture>		_cloudPlacementTexture;
		UniquePtr<Texture>		_nightSkyTexture;
		UniquePtr<Texture>		_cloudCurlNoise;
		UniquePtr<Texture3D>	_lowResCloudShapeTexture3D;
		UniquePtr<Texture3D>	_hiResCloudShapeTexture3D;

		// 
		UniquePtr<Geometry>		_sceneGeometry;
		UniquePtr<Geometry>		_backgroundGeometry;
		
		// 
		UniquePtr<MeshShader>			_meshShader;
		UniquePtr<BackgroundShader>		_backgroundShader;
		UniquePtr<ComputeShader>		_computeShader;
		UniquePtr<ReprojectShader>		_reprojectShader;
		UniquePtr<PostProcessShader>	_toneMapShader;
		UniquePtr<PostProcessShader>	_godRayShader;
		UniquePtr<PostProcessShader>	_radialBlurShader;

		//
		struct {
			UniquePtr<Texture>	colorBuffer[3];
			UniquePtr<Texture>	depthBuffer[3];
			RawSamplerID		sampler;
		}					_offscreenPass;

		/// --- Window interaction / functionality
		Nanoseconds		_startTime;

		mat4x4			_prevProjection;
		mat4x4			_prevView;
		vec3			_prevPos;
		float			_aspect;

		uint2			_renderTargetSize;
		Optional<vec2>	_debugPixel;

		bool			_reprojection	= false;
		
		
		const int WIDTH = 1920;// 1280;
		const int HEIGHT = 1080;// 720;

	// methods
	public:
		SkyEngine () {}
		~SkyEngine ();

		bool  Initialize ();


	// BaseSceneApp
	private:
		bool  DrawScene () override;
		

	// IWindowEventListener
	private:
		void  OnKey (StringView, EKeyAction) override;
		void  OnResize (const uint2 &size) override;


	private:
		bool  _InitializeTextures ();
		void  _CleanupTextures ();
		
		bool  _InitializeRenderTargets ();
		void  _CleanupRenderTargets ();

		bool  _InitializeShaders ();
		void  _CleanupShaders ();
		
		bool  _InitializeGeometry ();
		void  _CleanupGeometry ();

		bool  _SetupOffscreenPass ();
		void  _CleanupOffscreenPass ();

		void  _UpdateUniformBuffer (const CommandBuffer &cmdbuf);

		bool  _LoadImage (const CommandBuffer &cmdbuf, StringView filename, OUT ImageID &id);
		
		ND_ static String  _LoadShader (NtStringView filename)	{ return BaseSample::_LoadShader( String{FG_DATA_PATH} + filename.c_str()); }
	};

}	// FG
