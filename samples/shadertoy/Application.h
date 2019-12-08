// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ShaderView.h"
#include "video/IVideoRecorder.h"

namespace FG
{

	//
	// Shadertoy Application
	//

	class Application final : public BaseSceneApp
	{
	// types
	private:
		using Samples_t		= Array< std::function< void (Ptr<ShaderView> sv) > >;

		using EViewMode		= ShaderView::EViewMode;
		using ShaderDescr	= ShaderView::ShaderDescr;


	// variables
	private:
		UniquePtr<ShaderView>	_view;
		
		Samples_t				_samples;
		size_t					_currSample		= UMax;
		size_t					_nextSample		= 0;
		
		uint2					_targetSize;
		EViewMode				_viewMode		= EViewMode::Mono;

		RGBA32f					_selectedPixel;
		
		TimePoint_t				_startTime;
		TimePoint_t				_lastUpdateTime;
		uint					_frameCounter	= 0;

		bool					_pause			= false;
		bool					_freeze			= false;
		bool					_vrMirror		= false;

		UniquePtr<IVideoRecorder>	_videoRecorder;
		
		static inline const Rad			_cameraFov		= 60_deg;
		static inline const float		_sufaceScale	= 0.5f;
		static constexpr EPixelFormat	_imageFormat	= EPixelFormat::RGBA16F;


	// methods
	public:
		Application ();
		~Application ();
		
		bool  Initialize ();
		void  Destroy ();


	// BaseSceneApp
	public:
		bool  DrawScene () override;
		void  OnUpdateFrameStat (OUT String &) const;


	// IWindowEventListener
	private:
		void  OnKey (StringView, EKeyAction) override;


	private:
		void  _InitSamples ();
		void  _OnPixelReadn (const uint2 &point, const ImageView &view);
		void  _StartStopRecording ();
	};


}	// FG
