// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Application.h"
#include "Shaders.h"
#include "stl/Algorithms/StringUtils.h"
#include "video/FFmpegRecorder.h"

namespace FG
{
/*
=================================================
	_InitSamples
=================================================
*/
	void  Application::_InitSamples ()
	{
		_samples.push_back( Shaders::Shadertoy::SirenianDawn );
		_samples.push_back( Shaders::Shadertoy::NovaMarble );
		_samples.push_back( Shaders::Shadertoy::GlowCity );
		_samples.push_back( Shaders::ShadertoyVR::Skyline );
		_samples.push_back( Shaders::ShadertoyVR::SkylineFreeCam );
		_samples.push_back( Shaders::ShadertoyVR::NightMist );
		_samples.push_back( Shaders::ShadertoyVR::Catacombs );
		_samples.push_back( Shaders::ShadertoyVR::DesertCanyon );
		_samples.push_back( Shaders::ShadertoyVR::FrozenWasteland );
		_samples.push_back( Shaders::ShadertoyVR::Apollonian );
		_samples.push_back( Shaders::ShadertoyVR::ProteanClouds );

		_samples.push_back( Shaders::My::ConvexShape2D );
		_samples.push_back( Shaders::My::OptimizedSDF );

		_samples.push_back( Shaders::MyVR::ConvexShape3D );
		_samples.push_back( Shaders::MyVR::Building_1 );
		_samples.push_back( Shaders::MyVR::Building_2 );
	}

/*
=================================================
	constructor
=================================================
*/
	Application::Application ()
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	Application::~Application ()
	{
		if ( _videoRecorder )
			CHECK( _videoRecorder->End() );
	}

/*
=================================================
	Initialize
=================================================
*/
	bool  Application::Initialize ()
	{
		{
			AppConfig	cfg;
			cfg.surfaceSize			= uint2(1024, 768);
			cfg.windowTitle			= "Shadertoy";
			cfg.shaderDirectories	= { FG_DATA_PATH "../shaderlib" };
			cfg.dbgOutputPath		= FG_DATA_PATH "_debug_output";
			//cfg.vrMode			= AppConfig::EVRMode::Emulator;
			cfg.enableDebugLayers	= false;
			CHECK_ERR( _CreateFrameGraph( cfg ));
		}

		_view.reset( new ShaderView{_frameGraph} );

		_InitSamples();
		
		_viewMode	= EViewMode::Mono;
		_targetSize	= uint2(float2(GetSurfaceSize()) * _sufaceScale + 0.5f);

		_ResetPosition();
		GetFPSCamera().SetPerspective( _cameraFov, 1.0f, 0.1f, 100.0f );
		//GetVRCamera().SetHmdOffsetScale( 0.1f );

		_view->SetMode( _targetSize, _viewMode );
		_view->SetCamera( GetFPSCamera() );
		_view->SetFov( _cameraFov );
		_view->SetImageFormat( _imageFormat );

		return true;
	}

/*
=================================================
	OnKey
=================================================
*/
	void  Application::OnKey (StringView key, EKeyAction action)
	{
		BaseSceneApp::OnKey( key, action );

		if ( action == EKeyAction::Down )
		{
			if ( key == "[" )		--_nextSample;		else
			if ( key == "]" )		++_nextSample;

			if ( key == "R" )		_view->Recompile();
			if ( key == "T" )		_frameCounter = 0;
			if ( key == "U" )		_view->DebugPixel( GetMousePos() / vec2{GetSurfaceSize().x, GetSurfaceSize().y} );
			if ( key == "P" )		_ResetPosition();

			if ( key == "F" )		_freeze = not _freeze;
			if ( key == "space" )	_pause = not _pause;

			if ( key == "M" )		_vrMirror = not _vrMirror;
		}

		if ( action == EKeyAction::Up )
		{
			if ( key == "L" )		_StartStopRecording();
		}
	}
		
/*
=================================================
	DrawScene
=================================================
*/
	bool  Application::DrawScene ()
	{
		CHECK_ERR( _view );

		if ( _pause )
		{
			std::this_thread::sleep_for(SecondsF{0.01f});
			return true;
		}

		// select sample
		if ( _currSample != _nextSample )
		{
			_nextSample = _nextSample % _samples.size();
			_currSample = _nextSample;

			_frameCounter = 0;
			_view->ResetShaders();
			_samples[_currSample]( _view.get() );
		}


		// update camera & view mode
		{
			_UpdateCamera();

			if ( GetVRDevice() and GetVRDevice()->GetHmdStatus() == IVRDevice::EHmdStatus::Mounted )
			{
				_viewMode	= EViewMode::HMD_VR;
				_targetSize	= uint2(float2(GetVRDevice()->GetRenderTargetDimension()) * _sufaceScale + 0.5f);
				_view->SetCamera( GetVRCamera() );
			}
			else
			{
				_viewMode	= EViewMode::Mono;
				_targetSize	= uint2(float2(GetSurfaceSize()) * _sufaceScale + 0.5f);
				_view->SetCamera( GetFPSCamera() );
			}

			_view->SetMode( _targetSize, _viewMode );
			_view->SetMouse( GetMousePos() / vec2(VecCast(GetSurfaceSize())), IsMousePressed() );
		}


		// draw
		Task			task;
		RawImageID		image_l, image_r;

		CommandBuffer	cmdbuf = _frameGraph->Begin( CommandBufferDesc{ EQueueType::Graphics });
		CHECK_ERR( cmdbuf );
		{
			const auto	time = TimePoint_t::clock::now();

			if ( _frameCounter == 0 )
				_startTime = time;

			const SecondsF	app_dt		= std::chrono::duration_cast<SecondsF>( (_freeze ? _lastUpdateTime : time) - _startTime );
			const SecondsF	frame_dt	= std::chrono::duration_cast<SecondsF>( time - _lastUpdateTime );

			std::tie(task, image_l, image_r) = _view->Draw( cmdbuf, _frameCounter, app_dt, frame_dt );
			CHECK_ERR( task and image_l );

			if ( not _freeze ) {
				_lastUpdateTime	= time;
				++_frameCounter;
			}
		}


		// present
		if ( _viewMode == EViewMode::HMD_VR )
		{
			if ( _vrMirror )
				cmdbuf->AddTask( Present{ GetSwapchain(), image_l }.DependsOn( task ));
			
			CHECK_ERR( _frameGraph->Execute( cmdbuf ));
			CHECK_ERR( _frameGraph->Flush() );

			const auto&			queue		= GetVulkan().GetVkQueues()[0];
			const auto&			vk_desc_l	= std::get<VulkanImageDesc>( _frameGraph->GetApiSpecificDescription( image_l ));
			const auto&			vk_desc_r	= std::get<VulkanImageDesc>( _frameGraph->GetApiSpecificDescription( image_r ));
			IVRDevice::VRImage	vr_img;

			vr_img.currQueue		= queue.handle;
			vr_img.queueFamilyIndex	= queue.familyIndex;
			vr_img.dimension		= vk_desc_l.dimension.xy();
			vr_img.bounds			= RectF{ 0.0f, 0.0f, 1.0f, 1.0f };
			vr_img.format			= BitCast<VkFormat>(vk_desc_l.format);
			vr_img.sampleCount		= vk_desc_l.samples;

			vr_img.handle = BitCast<VkImage>(vk_desc_l.image);
			GetVRDevice()->Submit( vr_img, IVRDevice::Eye::Left );
					
			vr_img.handle = BitCast<VkImage>(vk_desc_r.image);
			GetVRDevice()->Submit( vr_img, IVRDevice::Eye::Right );
		}
		else
		{
			const auto&	desc	= _frameGraph->GetDescription( image_l );
			const uint2	point	= { uint((desc.dimension.x * GetMousePos().x) / GetSurfaceSize().x + 0.5f),
									uint((desc.dimension.y * GetMousePos().y) / GetSurfaceSize().y + 0.5f) };

			// read pixel
			if ( point.x < desc.dimension.x and point.y < desc.dimension.y )
			{
				cmdbuf->AddTask( ReadImage{}.SetImage( image_l, point, uint2{1,1} )
									.SetCallback( [this, point] (const ImageView &view) { _OnPixelReadn( point, view ); })
									.DependsOn( task ));
			}

			// add video frame
			if ( _videoRecorder )
			{
				cmdbuf->AddTask( ReadImage{}.SetImage( image_l, uint2(0), _targetSize )
									.SetCallback( [this] (const ImageView &view)
									{
										if ( _videoRecorder )
											CHECK( _videoRecorder->AddFrame( view ));
									})
									.DependsOn( task ));
			}
			
			cmdbuf->AddTask( Present{ GetSwapchain(), image_l }.DependsOn( task ));

			CHECK_ERR( _frameGraph->Execute( cmdbuf ));
			CHECK_ERR( _frameGraph->Flush() );
		}

		_SetLastCommandBuffer( cmdbuf );
		return true;
	}
	
/*
=================================================
	OnUpdateFrameStat
=================================================
*/
	void  Application::OnUpdateFrameStat (OUT String &str) const
	{
		str << ", Pixel: " << ToString( _selectedPixel );
	}

/*
=================================================
	Destroy
=================================================
*/
	void  Application::Destroy ()
	{
		_view.reset();

		_DestroyFrameGraph();
	}
	
/*
=================================================
	_OnPixelReadn
=================================================
*/
	void  Application::_OnPixelReadn (const uint2 &, const ImageView &view)
	{
		view.Load( uint3{0,0,0}, OUT _selectedPixel );
	}

/*
=================================================
	_StartStopRecording
=================================================
*/
	void  Application::_StartStopRecording ()
	{
		if ( _videoRecorder )
		{
			CHECK( _videoRecorder->End() );
			_videoRecorder.reset();
		}
		else
		{
			#if defined(FG_ENABLE_FFMPEG)
				_videoRecorder.reset( new FFmpegVideoRecorder{} );
			#endif

			if ( _videoRecorder )
				CHECK( _videoRecorder->Begin( _targetSize, 60, 7552, EVideoFormat::YUV_420P, "video.mp4" ));
		}
	}
	
/*
=================================================
	_ResetPosition
=================================================
*/
	void  Application::_ResetPosition ()
	{
		GetFPSCamera().SetPosition({ 0.0f, 0.0f, 0.0f });
	}


}	// FG
