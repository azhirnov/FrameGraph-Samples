// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/BaseSceneApp.h"

namespace FG
{

	//
	// Shader View
	//

	class ShaderView final
	{
	// types
	public:
		enum class EViewMode
		{
			Mono		= 0,
			HMD_VR		= 1,
			VR180_Video	= 180,
			VR360_Video	= 360,
		};

		static constexpr uint	MaxChannels	= 4;

		struct ShaderDescr
		{
		// types
			struct Channel {
				String			name;
				uint			index	= UMax;
				RawSamplerID	samp;
			};
			using Channels_t = FixedArray< Channel, MaxChannels >;
			
		// variables
			String					_pplnFilename;
			String					_pplnDefines;
			Channels_t				_channels;
			Optional<float>			_surfaceScale;
			Optional<uint2>			_surfaceSize;
			Optional<EPixelFormat>	_format;
			RawSamplerID			_sampler;

		// methods
			ShaderDescr () {}
			ShaderDescr&  Pipeline (String &&file, String &&def = "")               	{ _pplnFilename = std::move(file);  _pplnDefines = std::move(def);  return *this; }
			ShaderDescr&  InChannel (const String &name, uint index, RawSamplerID samp)	{ _channels.push_back({ name, index, samp });  return *this; }
			ShaderDescr&  InChannel (const String &name, uint index)					{ _channels.push_back({ name, index, {} });  return *this; }
			ShaderDescr&  SetScale (float value)										{ _surfaceScale = value;  return *this; }
			ShaderDescr&  SetDimension (uint2 value)									{ _surfaceSize = value;  return *this; }
			ShaderDescr&  SetFormat (EPixelFormat value)								{ _format = value;  return *this; }
		};


	private:
		
		struct ShadertoyUB
		{
			vec3		iResolution;			// offset: 0, align: 16		// viewport resolution (in pixels)
			float		iTime;					// offset: 12, align: 4		// shader playback time (in seconds)
			float		iTimeDelta;				// offset: 16, align: 4		// render time (in seconds)
			int			iFrame;					// offset: 20, align: 4		// shader playback frame
			vec2		_padding0;
			vec4		iChannelTime [MaxChannels];		// offset: 32, align: 16
			vec4		iChannelResolution [MaxChannels];// offset: 96, align: 16
			vec4		iMouse;					// offset: 160, align: 16	// mouse pixel coords. xy: current (if MLB down), zw: click
			vec4		iDate;					// offset: 176, align: 16	// (year, month, day, time in seconds)
			float		iSampleRate;			// offset: 192, align: 4	// sound sample rate (i.e., 44100)
			float		_padding1;
			float		_padding2;
			float		_padding3;
			vec3		iCameraFrustumRayLB;	// offset: 208, align: 16	// left bottom - frustum rays
			float		_padding4;
			vec3		iCameraFrustumRayRB;	// offset: 224, align: 16	// right bottom
			float		_padding5;
			vec3		iCameraFrustumRayLT;	// offset: 240, align: 16	// left top
			float		_padding6;
			vec3		iCameraFrustumRayRT;	// offset: 256, align: 16	// right top
			float		_padding7;
			vec3		iCameraPos;				// offset: 272, align: 16	// camera position in world space
			int			iEyeIndex;
		};


		struct Shader
		{
		// types
			using ChannelImages_t	= FixedArray< ImageID, ShaderDescr::Channels_t::capacity() >;

			struct PerPass {
				PipelineResources	resources;
				ImageID				renderTargetMS;
				ImageID				renderTarget;
				uint2				viewport;
				ChannelImages_t		images;
			};
			using PerPass_t		= StaticArray< PerPass, 4 >;

			struct PerEye {
				PerPass_t		passes;
				BufferID		ubuffer;
			};

			using PerEye_t		= FixedArray< PerEye, 2 >;
			using Channels_t	= ShaderDescr::Channels_t;

		// variables
			struct {
				GPipelineID				mono;
				GPipelineID				hmdVR;
				GPipelineID				vr180;
				GPipelineID				vr360;
			}						_pipeline;
			const String			_name;
			String					_pplnFilename;
			String					_pplnDefines;
			Channels_t				_channels;
			PerEye_t				_perEye;
			Optional<float>			_surfaceScale;
			Optional<uint2>			_surfaceSize;
			Optional<EPixelFormat>	_format;

		// methods
			explicit Shader (StringView name, ShaderDescr &&desc);

			ND_ StringView  Name ()	const	{ return _name; }
		};

		using ShaderPtr		= SharedPtr< Shader >;
		using ShadersMap_t	= HashMap< String, ShaderPtr >;

		using ImageCache_t	= HashMap< String, ImageID >;
		
		using SecondsF		= std::chrono::duration< float >;

		using DrawResult_t	= Tuple< Task, RawImageID, RawImageID >;	// last task, left eye, right eye (can be null)


	// variables
	private:
		FrameGraph				_frameGraph;

		uint2					_viewSize;
		EViewMode				_viewMode			= EViewMode::Mono;
		EPixelFormat			_imageFormat		= EPixelFormat::RGBA8_UNorm;
		uint					_imageSamples		= 1;
		uint					_passIdx : 1;
		bool					_recreateShaders	= false;

		ShadersMap_t			_shaders;
		Array< ShaderPtr >		_ordered;

		ShadertoyUB				_ubData;
		Task					_currTask;
		
		SamplerID				_nearestClampSampler;
		SamplerID				_linearClampSampler;
		SamplerID				_nearestRepeatSampler;
		SamplerID				_linearRepeatSampler;
		
		// camera
		FPSCamera				_camera;
		Rad						_cameraFov			= 60_deg;
		VRCamera				_vrCamera;

		Optional<vec2>			_debugPixel;

		ImageCache_t			_imageCache;
		
		vec2					_lastMousePos;		// in unorm coords
		bool					_mousePressed		= false;


	// methods
	public:
		explicit ShaderView (const FrameGraph &fg);
		~ShaderView ();
		
		void  AddShader (const String &name, ShaderDescr &&desc);
		void  AddShader (String &&fname);

		bool  Recompile ();
		void  ResetShaders ();

		void  SetMode (const uint2 &viewSize, EViewMode mode);
		void  SetMouse (const vec2 &pos, bool pressed);
		void  SetCamera (const FPSCamera &value);
		void  SetCamera (const VRCamera &value);
		void  SetFov (Rad value);
		void  SetImageFormat (EPixelFormat value, uint msaa = 0);
		void  DebugPixel (const vec2 &coord);

		ND_ DrawResult_t  Draw (const CommandBuffer &cmd, uint frameId, SecondsF time, SecondsF dt);


	private:
		void _CreateSamplers ();

		bool _RecreateShaders (const CommandBuffer &cmd);
		bool _CreateShader (const CommandBuffer &cmd, const ShaderPtr &shader);
		void _DestroyShader (const ShaderPtr &shader, bool destroyPipeline);
		bool _DrawWithShader (const CommandBuffer &cmd, const ShaderPtr &shader, uint eye, uint passIndex, bool isLast);
		void _UpdateShaderData (uint frameId, SecondsF time, SecondsF dt);

		bool _LoadImage (const CommandBuffer &cmd, const String &filename, OUT ImageID &id);
		bool _HasImage (StringView filename) const;

		GPipelineID  _Compile (StringView name, StringView defs) const;
	};


}	// FG
