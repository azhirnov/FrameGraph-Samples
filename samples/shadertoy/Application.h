// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ShaderView.h"
#include "video/IVideoRecorder.h"
#include "ui/ImguiRenderer.h"

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
		using Microsec		= std::chrono::microseconds;
		
		using KeyStates_t	= StaticArray< EKeyAction, 3 >;


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
		float					_sufaceScale	= 0.5f;
		vec4					_sliders		{0.0f};

		UniquePtr<IVideoRecorder>	_videoRecorder;
		String						_screenshotDir;
		
		#ifdef FG_ENABLE_IMGUI
		ImguiRenderer			_uiRenderer;
		KeyStates_t				_mouseJustPressed;
		bool					_settingsWndOpen	= true;
		#endif

		static inline const Rad			_cameraFov		= 60_deg;
		static inline const float		_vrSufaceScale	= 1.0f;
		static constexpr EPixelFormat	_imageFormat	= EPixelFormat::RGBA8_UNorm;


	// methods
	public:
		Application ();
		~Application ();
		
		bool  Initialize ();
		void  Destroy ();


	// BaseSceneApp
	public:
		bool  DrawScene () override;


	// IWindowEventListener
	private:
		void  OnKey (StringView, EKeyAction) override;


	private:
		void  _InitSamples ();
		void  _OnPixelReadn (const uint2 &point, const ImageView &view);
		void  _StartStopRecording ();
		void  _ResetPosition ();
		void  _ResetOrientation ();
		
		bool  _UpdateUI (const uint2 &dim);
		bool  _UpdateInput ();

		void  _SaveImage (const ImageView &view);
	};


}	// FG
