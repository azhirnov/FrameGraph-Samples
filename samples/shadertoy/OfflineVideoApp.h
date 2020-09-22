// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "BaseSample.h"
#include "ShaderView.h"
#include "video/IVideoRecorder.h"

namespace FG
{

	//
	// Shadertoy Offline Video Renderer
	//

	class OfflineVideoApp final : public BaseSample
	{
	// types
	public:
		using EViewMode		= ShaderView::EViewMode;
		using ShaderDescr	= ShaderView::ShaderDescr;
		using Shader_t		= Function< void (Ptr<ShaderView> sv) >;

		struct Config
		{
			uint2				imageSize		= uint2{1920, 1080};
			EViewMode			viewMode		= EViewMode::Mono;
			EPixelFormat		imageFormat		= EPixelFormat::RGBA8_UNorm;
			uint				imageSamples	= 1;
			uint				fps				= 30;
			uint64_t			bitrate			= 10ull << 20;
		};


	// variables
	private:
		UniquePtr<ShaderView>	_view;
		
		const Config			_config;
		
		uint					_frameCounter	= 0;
		uint					_maxFrames		= 0;
		bool					_pause			= false;
		bool					_mirror			= true;

		String						_videoName;
		UniquePtr<IVideoRecorder>	_videoRecorder;

		static inline const Rad		_cameraFov	= 60_deg;


	// methods
	public:
		explicit OfflineVideoApp (const Config &cfg);
		~OfflineVideoApp ();
		
		bool  Initialize (Shader_t shader, uint maxFrames, StringView videoName);


	// BaseSceneApp
	public:
		bool  DrawScene () override;
		void  OnUpdateFrameStat (OUT String &) const;


	// IWindowEventListener
	private:
		void  OnKey (StringView, EKeyAction) override;


	private:
		void _StopRecording ();
	};


}	// FG
