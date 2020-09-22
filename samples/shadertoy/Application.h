// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "BaseSample.h"
#include "ShaderView.h"
#include "video/IVideoRecorder.h"

namespace FG
{

	//
	// Shadertoy Application
	//

	class Application final : public BaseSample
	{
	// types
	private:
		using Samples_t		= Array< Function< void (Ptr<ShaderView> sv) > >;

		using EViewMode		= ShaderView::EViewMode;
		using ShaderDescr	= ShaderView::ShaderDescr;
		using Microsec		= std::chrono::microseconds;


	// variables
	private:
		UniquePtr<ShaderView>	_view;
		
		Samples_t				_samples;
		size_t					_currSample		= UMax;
		size_t					_nextSample		= 0;
		
		uint2					_targetSize;
		EViewMode				_viewMode		= EViewMode::Mono;

		RGBA32f					_selectedPixel;
		
		Microsec				_shaderTime;
		TimePoint_t				_lastUpdateTime;
		uint					_frameCounter	= 0;

		bool					_pause			= false;
		bool					_freeze			= false;
		bool					_skipLastTime	= false;
		bool					_vrMirror		= false;
		bool					_showTimemap	= false;
		bool					_makeScreenshot	= false;
		bool					_recompile		= false;
		int						_sufaceScaleIdx	= -1;
		vec4					_sliders		{0.0f};

		UniquePtr<IVideoRecorder>	_videoRecorder;
		String						_screenshotDir;

		static inline const float		_fovStep			= float(0.9_deg);
		static inline const int			_vrSufaceScaleIdx	= -1;
		static constexpr EPixelFormat	_imageFormat		= EPixelFormat::RGBA8_UNorm;


	// methods
	public:
		Application ();
		~Application ();
		
		bool  Initialize ();


	// BaseSceneApp
	public:
		bool  DrawScene () override;


	// IWindowEventListener
	private:
		void  OnKey (StringView, EKeyAction) override;
		

	// BaseSample
	private:
		void  OnUpdateUI () override;


	private:
		void  _InitSamples ();
		void  _OnPixelReadn (const uint2 &point, const ImageView &view);
		void  _StartStopRecording ();
		void  _ResetPosition ();
		void  _ResetOrientation ();

		void  _SaveImage (const ImageView &view);
	};


}	// FG
