// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
	const uint		fps	= 60;
	OfflineVideoApp	app{ uint2{8192, 4096}, OfflineVideoApp::EViewMode::VR360_Video, EPixelFormat::RGBA8_UNorm, fps, 360<<10 };
	CHECK_ERR( app.Initialize( Shaders::ShadertoyVR::Skyline, 60*fps, "skyline1.mp4" ), -1 );
#endif
	
	for (; app.Update(); ) {}

	app.Destroy();
	return 0;
}
