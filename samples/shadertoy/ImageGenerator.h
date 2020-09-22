// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "BaseSample.h"
#include "ShaderView.h"
#include "video/IVideoRecorder.h"

namespace FG
{

	//
	// Shadertoy Image Generator
	//

	class ImageGenerator final : public BaseSample
	{
	// types
	public:
		using EViewMode		= ShaderView::EViewMode;
		using ShaderDescr	= ShaderView::ShaderDescr;
		using Shader_t		= Function< void (Ptr<ShaderView> sv) >;

		struct Config
		{
			uint3				imageSize		= uint3{512, 512, 1};
			EViewMode			viewMode		= EViewMode::Mono;
			EPixelFormat		imageFormat		= EPixelFormat::RGBA8_UNorm;
			uint				imageSamples	= 1;
		};


	// variables
	private:
		UniquePtr<ShaderView>	_view;
		
		const Config			_config;
		uint					_frameCounter	= 0;
		String					_imageName;

		ImageID					_image;

		static inline const Rad	_cameraFov		= 60_deg;


	// methods
	public:
		explicit ImageGenerator (const Config &cfg);
		~ImageGenerator ();
		
		bool  Initialize (Shader_t shader, StringView imageName);


	// BaseSceneApp
	public:
		bool  DrawScene () override;


	// IWindowEventListener
	private:
		void  OnKey (StringView, EKeyAction) override;


	private:
		bool  _SaveImage ();
	};


}	// FG
