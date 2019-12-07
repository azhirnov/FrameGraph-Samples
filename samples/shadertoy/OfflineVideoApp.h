// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ShaderView.h"
#include "video/IVideoRecorder.h"

namespace FG
{

	//
	// Shadertoy Offline Video Renderer
	//

	class OfflineVideoApp final : public BaseSceneApp
	{
	// types
	public:
		using EViewMode		= ShaderView::EViewMode;
		using ShaderDescr	= ShaderView::ShaderDescr;
		using Shader_t		= std::function< void (Ptr<ShaderView> sv) >;


	// variables
	private:
		UniquePtr<ShaderView>	_view;
		
		const uint2				_targetSize;
		const EViewMode			_viewMode;
		const EPixelFormat		_imageFormat;
		const uint				_fps;
		const uint				_bitrateKb;
		
		uint					_frameCounter	= 0;
		uint					_maxFrames		= 0;
		bool					_pause			= false;
		bool					_mirror			= true;

		String						_videoName;
		UniquePtr<IVideoRecorder>	_videoRecorder;
		
		static inline const Rad		_cameraFov		= 60_deg;
		static inline const float	_sufaceScale	= 0.5f;


	// methods
	public:
		OfflineVideoApp (const uint2 &size, EViewMode mode, EPixelFormat fmt, uint fps, uint bitrateKb);
		~OfflineVideoApp ();
		
		bool  Initialize (Shader_t shader, uint maxFrames, StringView videoName);
		void  Destroy ();


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
