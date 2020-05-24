// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Application.h"
#include "Shaders.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Stream/FileStream.h"

#include "video/FFmpegRecorder.h"

#include "scene/Loader/DevIL/DevILSaver.h"
#include "scene/Loader/DDS/DDSSaver.h"
#include "scene/Loader/Intermediate/IntermImage.h"

#ifdef FG_ENABLE_IMGUI
#	include "imgui_internal.h"
#endif

namespace FG
{
/*
=================================================
	_InitSamples
=================================================
*/
	void  Application::_InitSamples ()
	{
		_samples.push_back( Shaders::Shadertoy::Auroras );
		_samples.push_back( Shaders::Shadertoy::AlienBeacon );
		_samples.push_back( Shaders::Shadertoy::CloudFlight );
		_samples.push_back( Shaders::Shadertoy::CloudyTerrain );
		_samples.push_back( Shaders::Shadertoy::Canyon );
		_samples.push_back( Shaders::Shadertoy::CanyonPass );
		_samples.push_back( Shaders::Shadertoy::Dwarf );
		_samples.push_back( Shaders::Shadertoy::DesertPassage );
		_samples.push_back( Shaders::Shadertoy::DesertSand );
		_samples.push_back( Shaders::Shadertoy::Glowballs );
		_samples.push_back( Shaders::Shadertoy::GlowCity );
		_samples.push_back( Shaders::Shadertoy::Generators );
		_samples.push_back( Shaders::Shadertoy::Insect );
		_samples.push_back( Shaders::Shadertoy::Luminescence );
		_samples.push_back( Shaders::Shadertoy::Mesas );
		_samples.push_back( Shaders::Shadertoy::NovaMarble );
		_samples.push_back( Shaders::Shadertoy::Organix );
		_samples.push_back( Shaders::Shadertoy::PlasmaGlobe );
		_samples.push_back( Shaders::Shadertoy::SpaceEgg );
		_samples.push_back( Shaders::Shadertoy::SculptureIII );
		_samples.push_back( Shaders::Shadertoy::StructuredVolSampling );
		_samples.push_back( Shaders::Shadertoy::ServerRoom );
		_samples.push_back( Shaders::Shadertoy::Volcanic );

		_samples.push_back( Shaders::ShadertoyVR::AncientMars );
		_samples.push_back( Shaders::ShadertoyVR::Apollonian );
		_samples.push_back( Shaders::ShadertoyVR::AtTheMountains );
		_samples.push_back( Shaders::ShadertoyVR::Catacombs );
		_samples.push_back( Shaders::ShadertoyVR::CavePillars );
		_samples.push_back( Shaders::ShadertoyVR::DesertCanyon );
		_samples.push_back( Shaders::ShadertoyVR::FrozenWasteland );
		_samples.push_back( Shaders::ShadertoyVR::FractalExplorer );
		_samples.push_back( Shaders::ShadertoyVR::IveSeen );
		_samples.push_back( Shaders::ShadertoyVR::NebulousTunnel );
		_samples.push_back( Shaders::ShadertoyVR::NightMist );
		_samples.push_back( Shaders::ShadertoyVR::OpticalDeconstruction );
		_samples.push_back( Shaders::ShadertoyVR::ProteanClouds );
		_samples.push_back( Shaders::ShadertoyVR::PeacefulPostApocalyptic );
		_samples.push_back( Shaders::ShadertoyVR::SirenianDawn );
		_samples.push_back( Shaders::ShadertoyVR::SphereFBM );
		_samples.push_back( Shaders::ShadertoyVR::Skyline );
		_samples.push_back( Shaders::ShadertoyVR::Xyptonjtroz );

		_samples.push_back( Shaders::My::ConvexShape2D );
		_samples.push_back( Shaders::My::OptimizedSDF );
		_samples.push_back( Shaders::My::PrecalculatedRays );
		_samples.push_back( Shaders::My::VoronoiRecursion );
		_samples.push_back( Shaders::My::ThousandsOfStars );

		//_samples.push_back( Shaders::MyVR::ConvexShape3D );
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
		Destroy();
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
			cfg.shaderDirectories	= { FG_DATA_PATH "../shaderlib", FG_DATA_PATH };
			cfg.dbgOutputPath		= FG_DATA_PATH "_debug_output";
			//cfg.vrMode				= AppConfig::EVRMode::OpenVR;
			//cfg.enableDebugLayers	= false;
			CHECK_ERR( _CreateFrameGraph( cfg ));
		}
		
		// initialize imgui and renderer
		#ifdef FG_ENABLE_IMGUI
		{
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGui::StyleColorsDark();

			CHECK_ERR( _uiRenderer.Initialize( _frameGraph, GImGui ));
			_mouseJustPressed.fill( EKeyAction::Up );
		}
		#endif

		_screenshotDir = FG_DATA_PATH "screenshots";

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

			if ( key == "R" )		_recompile = true;
			if ( key == "T" )		_frameCounter = 0;
			if ( key == "P" )		_ResetPosition();

			if ( key == "F" )		{ _skipLastTime = _freeze;	_freeze = not _freeze;	}
			if ( key == "space" )	{ _skipLastTime = _pause;	_pause  = not _pause;	}

			if ( key == "M" )		_vrMirror = not _vrMirror;
			if ( key == "I" )		_makeScreenshot = true;
			if ( key == "Y" )		_settingsWndOpen = not _settingsWndOpen;

			// profiling
			if ( key == "G" )		_view->RecordShaderTrace( GetMousePos() / vec2{GetSurfaceSize().x, GetSurfaceSize().y} );
			if ( key == "H" )		_view->RecordShaderProfiling( GetMousePos() / vec2{GetSurfaceSize().x, GetSurfaceSize().y} );
			if ( key == "J" )		_showTimemap = not _showTimemap;
		}

		if ( action == EKeyAction::Up )
		{
			if ( key == "U" )		_StartStopRecording();
		}
		
	#ifdef FG_ENABLE_IMGUI
		if ( key == "left mb" )		_mouseJustPressed[0] = action;
		if ( key == "right mb" )	_mouseJustPressed[1] = action;
		if ( key == "middle mb" )	_mouseJustPressed[2] = action;
	#endif
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

			if ( IsActiveVR() )
			{
				_viewMode	= EViewMode::HMD_VR;
				_targetSize	= uint2(float2(GetVRDevice()->GetRenderTargetDimension()) * _vrSufaceScale + 0.5f);
				_view->SetCamera( GetVRCamera() );

				auto&	vr_cont = GetVRDevice()->GetControllers();
				auto	left	= vr_cont.find( ControllerID::LeftHand );
				auto	right	= vr_cont.find( ControllerID::RightHand );
				bool	l_valid	= left  != vr_cont.end() and left->second.isValid;
				bool	r_valid	= right != vr_cont.end() and right->second.isValid;
				mat4x4	l_mvp, r_mvp;

				if ( l_valid )
				{
					l_mvp = mat4x4(MatCast(left->second.pose));
					l_mvp[3] = vec4(VecCast(left->second.position), 1.0);
				}
				if ( r_valid )
				{
					r_mvp = mat4x4(MatCast(right->second.pose));
					r_mvp[3] = vec4(VecCast(right->second.position), 1.0);
				}

				_view->SetControllerPose( l_mvp, r_mvp, (l_valid ? 1 : 0) | (r_valid ? 2 : 0) );
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
		
		if ( _recompile )
		{
			_recompile = false;
			_view->Recompile( cmdbuf );
		}

		if ( _viewMode != EViewMode::HMD_VR and _showTimemap )
			cmdbuf->BeginShaderTimeMap( _targetSize, EShaderStages::Fragment );

		// draw shader viewer
		{
			const auto	time = TimePoint_t::clock::now();

			if ( _frameCounter == 0 )
				_shaderTime = {};
			
			const SecondsF	app_dt		= std::chrono::duration_cast<SecondsF>( _shaderTime );
			const SecondsF	frame_dt	= std::chrono::duration_cast<SecondsF>( time - _lastUpdateTime );

			std::tie(task, image_l, image_r) = _view->Draw( cmdbuf, _frameCounter, app_dt, frame_dt );
			CHECK_ERR( task and image_l );

			if ( _skipLastTime )
			{
				_skipLastTime = false;
			}
			else
			if ( not _freeze )
			{
				++_frameCounter;
				_shaderTime += std::chrono::duration_cast<Microsec>(time - _lastUpdateTime);
			}
				
			_lastUpdateTime	= time;
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
			if ( _showTimemap )
				task = cmdbuf->EndShaderTimeMap( image_l, Default, Default, {task} );

			const auto&	desc	= _frameGraph->GetDescription( image_l );
			const uint2	point	= { uint((desc.dimension.x * GetMousePos().x) / GetSurfaceSize().x + 0.5f),
									uint((desc.dimension.y * GetMousePos().y) / GetSurfaceSize().y + 0.5f) };

			// read pixel
			if ( point.x < desc.dimension.x and point.y < desc.dimension.y )
			{
				task = cmdbuf->AddTask( ReadImage{}.SetImage( image_l, point, uint2{1,1} )
									.SetCallback( [this, point] (const ImageView &view) { _OnPixelReadn( point, view ); })
									.DependsOn( task ));
			}

			// add video frame
			if ( _videoRecorder )
			{
				task = cmdbuf->AddTask( ReadImage{}.SetImage( image_l, uint2(0), _targetSize )
									.SetCallback( [this] (const ImageView &view)
									{
										if ( _videoRecorder )
											CHECK( _videoRecorder->AddFrame( view ));
									})
									.DependsOn( task ));
			}
			else
			// make screenshot
			if ( _makeScreenshot )
			{
				_makeScreenshot = false;
				task = cmdbuf->AddTask( ReadImage{}.SetImage( image_l, uint2(0), _targetSize )
									.SetCallback( [this] (const ImageView &view)
									{
										_SaveImage( view );
									})
									.DependsOn( task ));
			}
			
			// copy to swapchain image
			{
				RawImageID	sw_image = cmdbuf->GetSwapchainImage( GetSwapchain() );
				uint2		img_dim  = _frameGraph->GetDescription( image_l ).dimension.xy();
				uint2		sw_dim   = _frameGraph->GetDescription( sw_image ).dimension.xy();

				task = cmdbuf->AddTask( BlitImage{}.From( image_l ).To( sw_image ).SetFilter( EFilter::Linear )
											.AddRegion( {}, int2(0), int2(img_dim), {}, int2(0), int2(sw_dim) )
											.DependsOn( task ));
			}

			// draw ui
			#ifdef FG_ENABLE_IMGUI
			if ( _settingsWndOpen )
			{
				RawImageID	sw_image = cmdbuf->GetSwapchainImage( GetSwapchain() );

				_UpdateUI( _frameGraph->GetDescription( sw_image ).dimension.xy() );

				auto&	draw_data = *ImGui::GetDrawData();

				if ( draw_data.TotalVtxCount > 0 )
				{
					LogicalPassID	pass_id = cmdbuf->CreateRenderPass( RenderPassDesc{ int2{float2{ draw_data.DisplaySize.x, draw_data.DisplaySize.y }} }
													.AddViewport(float2{ draw_data.DisplaySize.x, draw_data.DisplaySize.y })
													.AddTarget( RenderTargetID::Color_0, sw_image, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store ));

					task = _uiRenderer.Draw( cmdbuf, pass_id, {task} );
				}
			}
			#endif

			CHECK_ERR( _frameGraph->Execute( cmdbuf ));
			CHECK_ERR( _frameGraph->Flush() );
		}

		_SetLastCommandBuffer( cmdbuf );
		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void  Application::Destroy ()
	{
		_view.reset();
		
		#ifdef FG_ENABLE_IMGUI
			_uiRenderer.Deinitialize( _frameGraph );
		#endif

		if ( _videoRecorder )
		{
			CHECK( _videoRecorder->End() );
			_videoRecorder.reset();
		}

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
			{
				IVideoRecorder::Config		cfg;
				cfg.format		= EVideoFormat::YUV420P;
				cfg.codec		= EVideoCodec::H264;
				cfg.hwAccelerated= true;
				cfg.preset		= EVideoPreset::Fast;
				cfg.bitrate		= 7552ull << 10;
				cfg.fps			= 30;
				cfg.size		= _targetSize;

				String	vname	= "video"s << _videoRecorder->GetExtension( cfg.codec );

				CHECK( _videoRecorder->Begin( cfg, vname ));
			}
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
	
/*
=================================================
	_ResetOrientation
=================================================
*/
	void  Application::_ResetOrientation ()
	{
		GetFPSCamera().SetRotation( Quat_Identity );
	}

/*
=================================================
	_SaveImage
=================================================
*/
	void  Application::_SaveImage (const ImageView &view)
	{
	#ifdef FS_HAS_FILESYSTEM
		FS::create_directory( FS::path{ _screenshotDir });
	#endif

	#ifdef FS_HAS_FILESYSTEM
		const auto	IsExists = [] (StringView path) { return FS::exists(FS::path{ path }); };
	#else
		const auto	IsExists = [] (StringView path) { return false; };	// TODO
	#endif

	#ifdef FG_ENABLE_DEVIL
		const char	ext[] = ".png";
		const auto	SaveImage = [] (StringView fname, const IntermImagePtr &image)
		{
			DevILSaver	saver;
			return saver.SaveImage( fname, image );
		};

	#else
		const char	ext[] = ".dds";
		const auto	SaveImage = [] (StringView fname, const IntermImagePtr &image)
		{
			DDSSaver	saver;
			return saver.SaveImage( fname, image );
		};
	#endif
	

		String	fname;
		const auto	BuildName = [this, &fname, &ext] (uint index)
		{
			fname = String(_screenshotDir) << "/scr_" << ToString(index) << ext;
		};
		
		auto		image		= MakeShared<IntermImage>( view );
		uint		min_index	= 0;
		uint		max_index	= 1;
		const uint	step		= 100;

		for (; min_index < max_index;)
		{
			BuildName( max_index );

			if ( not IsExists( fname ))
				break;

			min_index = max_index;
			max_index += step;
		}

		for (uint index = min_index; index <= max_index; ++index)
		{
			BuildName( index );

			if ( IsExists( fname ))
				continue;

			CHECK( SaveImage( fname, image ));
			
			FG_LOGI( "screenshot saved to '"s << fname << "'" );
			break;
		}
	}
	
/*
=================================================
	_UpdateUI
=================================================
*/
	bool  Application::_UpdateUI (const uint2 &dim)
	{
	#ifdef FG_ENABLE_IMGUI
		ImGuiIO &	io = ImGui::GetIO();
		CHECK_ERR( io.Fonts->IsBuilt() );

		io.DisplaySize	= ImVec2{float(dim.x), float(dim.y)};
		io.DeltaTime	= FrameTime().count();

		CHECK_ERR( _UpdateInput() );

		const ImVec4	red_col		{ 0.5f, 0.0f, 0.0f, 1.0f };
		const ImVec4	green_col	{ 0.0f, 0.7f, 0.0f, 1.0f };

		ImGui::NewFrame();
			
		if ( ImGui::Begin( "Shader settings", &_settingsWndOpen ))
		{
			// camera
			{
				ImGui::Text( "W/S - move camera forward/backward" );
				ImGui::Text( "A/D - move camera left/right" );
				ImGui::Text( "V/C - move camera up/down" );
				ImGui::Text( "Y   - show/hide UI" );
			}
			ImGui::Separator();
			
			// shader
			{
				ImGui::Text( "Switch shader" );

				if ( ImGui::Button( " << ##PrevShader" ))
					--_nextSample;
			
				ImGui::SameLine();

				if ( ImGui::Button( " >> ##NextShader" ))
					++_nextSample;
			
				// pause
				{
					String	text;

					if ( _freeze ) {
						text = "Resume (F)";
						ImGui::PushStyleColor( ImGuiCol_Button, red_col );
					} else {
						text = "Pause (F)";
						ImGui::PushStyleColor( ImGuiCol_Button, green_col );
					}

					if ( ImGui::Button( text.c_str() ))
					{
						_skipLastTime	= _freeze;
						_freeze			= not _freeze;
					}

					ImGui::PopStyleColor(1);
				}

				// pause rendering
				if ( ImGui::Button( "Stop rendering (space)" ))
				{
					_skipLastTime	= _pause;
					_pause			= not _pause;
				}

				if ( ImGui::Button( "Reload (R)" ))
					_recompile = true;

				if ( ImGui::Button( "Restart (T)" ))
					_frameCounter = 0;

				if ( ImGui::Button( "Reset position (P)" ))
					_ResetPosition();
				
				ImGui::SameLine();
				
				if ( ImGui::Button( "Reset orientation" ))
					_ResetOrientation();

				float	surf = _sufaceScale;
				ImGui::Text( "Surface scale" );
				ImGui::SliderFloat( "##SurfaceScaleSlider", &surf, 0.25f, 2.0f );

				// can't change surface scale when capturing video
				if ( not _videoRecorder )
					_sufaceScale = surf;
			}
			ImGui::Separator();

			// capture
			{
				// video
				{
					String	text;
			
					if ( _videoRecorder ) {
						text = "Stop capture (U)";
						ImGui::PushStyleColor( ImGuiCol_Button, red_col );
					} else {
						text = "Start capture (U)";
						ImGui::PushStyleColor( ImGuiCol_Button, green_col );
					}

					if ( ImGui::Button( text.c_str() ))
						_StartStopRecording();

					ImGui::PopStyleColor(1);
				}

				if ( ImGui::Button( "Screenshot (I)" ))
					_makeScreenshot = true;
			}
			ImGui::Separator();

			// debugger
			{
				ImGui::Text( "G  - run shader debugger for pixel under cursor" );
				ImGui::Text( "H  - run shader profiler for pixel under cursor" );
				
				if ( _showTimemap )
					ImGui::PushStyleColor( ImGuiCol_Button, red_col );
				else
					ImGui::PushStyleColor( ImGuiCol_Button, green_col );

				if ( ImGui::Button( "Timemap" ))
					_showTimemap = not _showTimemap;

				ImGui::PopStyleColor(1);
			}
			ImGui::Separator();

			// params
			{
				ImGui::Text( ("position:    "s << ToString( GetCamera().transform.position )).c_str() );
				ImGui::Text( ("rotation:    "s << ToString( GetCamera().transform.orientation )).c_str() );
				ImGui::Text( ("mouse coord: "s << ToString( GetMousePos() )).c_str() );
				ImGui::Text( ("pixel color: "s << ToString( _selectedPixel )).c_str() );
			}
		}

		ImGui::End();
		ImGui::Render();

		return true;

	#else
		return true;
	#endif
	}

/*
=================================================
	_UpdateInput
=================================================
*/
	bool Application::_UpdateInput ()
	{
	#ifdef FG_ENABLE_IMGUI
		ImGuiIO &	io = ImGui::GetIO();

		for (size_t i = 0; i < _mouseJustPressed.size(); ++i)
		{
			io.MouseDown[i] = (_mouseJustPressed[i] != EKeyAction::Up);
		}

		io.MousePos = { GetMousePos().x, GetMousePos().y };

		memset( io.NavInputs, 0, sizeof(io.NavInputs) );
		return true;

	#else
		return true;
	#endif
	}


}	// FG
