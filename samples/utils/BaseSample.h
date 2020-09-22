// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/BaseSceneApp.h"
#include "ui/ImguiRenderer.h"

namespace FG
{

	//
	// Base Sample
	//

	class BaseSample : public BaseSceneApp
	{
	// types
	private:
		using KeyStates_t	= StaticArray< EKeyAction, 3 >;


	// variables
	protected:
		struct {
			RawSamplerID			linear;
			RawSamplerID			anisotropy;
			RawSamplerID			linearClamp;
			RawSamplerID			shadow;
		}						_sampler;
		
	private:
		#ifdef FG_ENABLE_IMGUI
		ImguiRenderer			_uiRenderer;
		KeyStates_t				_mouseJustPressed;
		bool					_settingsWndOpen	= true;
		bool					_mouseOverUI		= false;
		RectF					_uiWindowRect;
		#endif

		// TODO:
		//	- video recording
		//	- screenshots
		//	- histogram ?

		
	// methods
	public:
		BaseSample ();
		~BaseSample ();
		

	// IWindowEventListener
	protected:
		void  OnKey (StringView, EKeyAction) override;
		void  OnMouseMove (const float2 &) override;


	protected:
		bool  _InitUI ();
		void  _DrawUI (const CommandBuffer &cmdbuf, RawImageID renderTarget, ArrayView<Task> dependencies = Default);

		virtual void OnUpdateUI () {};

	private:
		bool  _UpdateUI (const uint2 &dim);
		bool  _UpdateInput ();

	protected:
		bool  _CreateSamplers ();
		
		ND_ static uint2		_ScaleSurface (const uint2 &size, int scaleIdx);
		ND_ static const char*	_SurfaceScaleName (int scaleIdx);

		ND_ static String  _LoadShader (NtStringView filename);
	};

}	// FG
