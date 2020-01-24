// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Application.h"
#include "OfflineVideoApp.h"
#include "Shaders.h"

/*
=================================================
	main
=================================================
*/
int main ()
{
	using namespace FG;

#if 1
	Application		app;
	CHECK_ERR( app.Initialize(), -1 );
#else

	using EViewMode = OfflineVideoApp::EViewMode;

	OfflineVideoApp::Config	cfg;
	cfg.imageFormat		= EPixelFormat::RGBA8_UNorm;
	cfg.imageSamples	= 8;
	cfg.fps				= 60;

	//cfg.imageSize	= uint2{1920, 1080};	cfg.viewMode = EViewMode::Mono;			cfg.bitrateKb = 12<<10;
	//cfg.imageSize	= uint2{4096, 2048};	cfg.viewMode = EViewMode::VR180_Video;	cfg.bitrateKb = 50<<10;
	cfg.imageSize	= uint2{4096, 2048};	cfg.viewMode = EViewMode::VR360_Video;	cfg.bitrateKb = 50<<10;
	
	OfflineVideoApp		app{ cfg };
	CHECK_ERR( app.Initialize( Shaders::ShadertoyVR::Skyline, 56*cfg.fps, "skyline.mp4" ), -1 );
#endif
	
	for (; app.Update(); ) {}

	app.Destroy();
	return 0;
}
