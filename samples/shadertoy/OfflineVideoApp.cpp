// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "OfflineVideoApp.h"
#include "stl/Algorithms/StringUtils.h"
#include "video/FFmpegRecorder.h"

#ifdef FG_STD_FILESYSTEM
#	include <filesystem>
	namespace FS = std::filesystem;
#endif

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	OfflineVideoApp::OfflineVideoApp (const Config &cfg) : _config{cfg}
	{}
	
/*
=================================================
	destructor
=================================================
*/
	OfflineVideoApp::~OfflineVideoApp ()
	{}

/*
=================================================
	Initialize
=================================================
*/
	bool  OfflineVideoApp::Initialize (Shader_t shader, uint maxFrames, StringView videoName)
	{
		{
			AppConfig	cfg;
			cfg.surfaceSize			= uint2(1024, 768);
			cfg.windowTitle			= "Shadertoy";
			cfg.shaderDirectories	= { FG_DATA_PATH "../shaderlib" };
			//cfg.enableDebugLayers	= false;
			CHECK_ERR( _CreateFrameGraph( cfg ));
		}

		_view.reset( new ShaderView{_frameGraph} );

		GetFPSCamera().SetPosition({ 0.0f, 0.0f, 0.0f });

		_view->SetMode( _config.imageSize, _config.viewMode );
		_view->SetCamera( GetFPSCamera() );
		_view->SetFov( _cameraFov );
		_view->SetImageFormat( _config.imageFormat, _config.imageSamples );
		
		#if defined(FG_ENABLE_FFMPEG)
			_videoRecorder.reset( new FFmpegVideoRecorder{} );
		#else
			RETURN_ERR( "no video recorder!" );
		#endif

		if ( _videoRecorder )
			CHECK( _videoRecorder->Begin( _config.imageSize, _config.fps, _config.bitrateKb, EVideoFormat::YUV_420P, videoName ));

		_videoName	= videoName;
		_maxFrames	= maxFrames;
		shader( _view.get() );

		return true;
	}

/*
=================================================
	OnKey
=================================================
*/
	void  OfflineVideoApp::OnKey (StringView key, EKeyAction action)
	{
		if ( action == EKeyAction::Down )
		{
			if ( key == "space" )	_pause = not _pause;

			if ( key == "escape" and GetWindow() )	GetWindow()->Quit();

			if ( key == "M" )		_mirror = not _mirror;
		}
	}
		
/*
=================================================
	DrawScene
=================================================
*/
	bool  OfflineVideoApp::DrawScene ()
	{
		CHECK_ERR( _view );

		if ( _pause )
		{
			std::this_thread::sleep_for(SecondsF{0.01f});
			return true;
		}

		// finish recording and exit
		if ( _frameCounter >= _maxFrames )
		{
			_StopRecording();

			if ( GetWindow() )
				GetWindow()->Quit();

			return true;
		}

		CommandBuffer	cmdbuf = _frameGraph->Begin( CommandBufferDesc{ EQueueType::Graphics });
		CHECK_ERR( cmdbuf );

		_UpdateCamera();
		_view->SetCamera( GetFPSCamera() );

		// draw
		const SecondsF	time	{ float(_frameCounter) / _config.fps };
		const SecondsF	dt		{ 1.0f / _config.fps };

		auto[task, image_l, image_r] = _view->Draw( cmdbuf, _frameCounter, time, dt );
		CHECK_ERR( task and image_l );

		++_frameCounter;

		// present
		{
			// add video frame
			if ( _videoRecorder )
			{
				cmdbuf->AddTask( ReadImage{}.SetImage( image_l, uint2(0), _config.imageSize )
									.SetCallback( [this] (const ImageView &view)
									{
										if ( _videoRecorder )
											CHECK( _videoRecorder->AddFrame( view ));
									})
									.DependsOn( task ));
			}
			
			if ( _mirror )
				cmdbuf->AddTask( Present{ GetSwapchain(), image_l }.DependsOn( task ));

			CHECK_ERR( _frameGraph->Execute( cmdbuf ));
			CHECK_ERR( _frameGraph->Flush() );
		}

		_SetLastCommandBuffer( cmdbuf );
		
		// to prevent overheating
		_frameGraph->WaitIdle();
		std::this_thread::sleep_for(SecondsF{0.1f});

		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void  OfflineVideoApp::Destroy ()
	{
		_view.reset();

		_DestroyFrameGraph();
		_StopRecording();
	}

/*
=================================================
	OnUpdateFrameStat
=================================================
*/
	void  OfflineVideoApp::OnUpdateFrameStat (OUT String &str) const
	{
		str << ", Rec: " << ToString( 100.0f * float(_frameCounter) / _maxFrames, 1 ) << '%';
	}
	
/*
=================================================
	_StopRecording
=================================================
*/
	void  OfflineVideoApp::_StopRecording ()
	{
		if ( _videoRecorder )
		{
			CHECK( _videoRecorder->End() );
			_videoRecorder.reset();
		}
	}

}	// FG
