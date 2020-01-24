// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/BaseSceneApp.h"

namespace FG
{
	static constexpr float	DefaultIPD	= 64.0e-3f;


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
			Mono360		= 3601,
			VR180_Video	= 1802,
			VR360_Video	= 3602,
		};

		static constexpr uint	MaxChannels	= 4;

		struct ShaderDescr
		{
		// types
			struct Channel {
				String			name;
				uint			index	= UMax;
				RawSamplerID	samp;
				bool			flipY	= false;
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
			float					_ipd		= DefaultIPD;	// (m) Interpupillary distance, the distance between the eyes.

		// methods
			ShaderDescr () {}
			ShaderDescr&  Pipeline (String &&file, String &&def = "")	{ _pplnFilename = std::move(file);  _pplnDefines = std::move(def);  return *this; }
			ShaderDescr&  SetScale (float value)						{ _surfaceScale = value;  return *this; }
			ShaderDescr&  SetDimension (uint2 value)					{ _surfaceSize = value;  return *this; }
			ShaderDescr&  SetFormat (EPixelFormat value)				{ _format = value;  return *this; }
			
			ShaderDescr&  InChannel (const String &name, uint index, RawSamplerID samp = {}, bool flipY = false) {
				_channels.push_back({ name, index, samp, flipY });  return *this;
			}
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
			float		iCameraIPD;				// offset: 196, align: 4	// (m) Interpupillary distance, the distance between the eyes.
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
				RawImageID			renderTargetMS;
				ImageID				renderTarget;
				uint2				viewport;
				ChannelImages_t		images;
			};
			using PerPass_t		= StaticArray< PerPass, 2 >;

			struct PerEye {
				PerPass_t		passes;
				BufferID		ubuffer;
				ImageID			renderTargetMS;
			};

			using PerEye_t		= FixedArray< PerEye, 2 >;
			using Channels_t	= ShaderDescr::Channels_t;

		// variables
			struct {
				GPipelineID				mono;
				GPipelineID				mono360;
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
			float					_ipd;

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
		SamplerID				_mipmapClampSampler;
		SamplerID				_mipmapRepeatSampler;
		
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

		ND_ RawSamplerID  NearestClampSampler ()	const	{ return _nearestClampSampler; }
		ND_ RawSamplerID  LinearClampSampler ()		const	{ return _linearClampSampler; }
		ND_ RawSamplerID  NearestRepeatSampler ()	const	{ return _nearestRepeatSampler; }
		ND_ RawSamplerID  LinearRepeatSampler ()	const	{ return _linearRepeatSampler; }
		ND_ RawSamplerID  MipmapClampSampler ()		const	{ return _mipmapClampSampler; }
		ND_ RawSamplerID  MipmapRepeatSampler ()	const	{ return _mipmapRepeatSampler; }


	private:
		void _CreateSamplers ();

		bool _RecreateShaders (const CommandBuffer &cmd);
		bool _CreateShader (const CommandBuffer &cmd, const ShaderPtr &shader);
		void _DestroyShader (const ShaderPtr &shader, bool destroyPipeline);
		bool _DrawWithShader (const CommandBuffer &cmd, const ShaderPtr &shader, uint eye, uint passIndex, bool isLast);

		bool _LoadImage (const CommandBuffer &cmd, const String &filename, bool flipY, OUT ImageID &id);
		bool _LoadImage2D (const CommandBuffer &cmd, const String &filename, bool flipY, OUT ImageID &id);
		bool _LoadImage3D (const CommandBuffer &cmd, const String &filename, OUT ImageID &id);
		bool _HasImage (StringView filename) const;
		EImage _GetImageType (StringView filename) const;

		GPipelineID  _Compile (StringView name, StringView defs, StringView samplers) const;
	};


}	// FG
