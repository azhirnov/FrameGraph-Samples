// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ParticlesApp.h"
#include "pipeline_compiler/VPipelineCompiler.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Stream/FileStream.h"

#ifdef FG_ENABLE_IMGUI
#	include "imgui_internal.h"
#endif

namespace FG
{
	
/*
=================================================
	destructor
=================================================
*/
	ParticlesApp::~ParticlesApp ()
	{
		if ( _frameGraph )
		{
		#ifdef FG_ENABLE_IMGUI
			_uiRenderer.Deinitialize( _frameGraph );
		#endif

			_frameGraph->ReleaseResource( _colorBuffer1 );
			_frameGraph->ReleaseResource( _colorBuffer2 );
			_frameGraph->ReleaseResource( _depthBuffer );

			_frameGraph->ReleaseResource( _cameraUB );
			_frameGraph->ReleaseResource( _particlesUB );
			_frameGraph->ReleaseResource( _particlesBuf );
			
			_frameGraph->ReleaseResource( _updateParticlesPpln );
			_frameGraph->ReleaseResource( _dotsParticlesPpln );
			_frameGraph->ReleaseResource( _raysParticlesPpln );
		}
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool  ParticlesApp::Initialize ()
	{
		AppConfig	cfg;
		cfg.surfaceSize			= uint2(1024, 768);
		cfg.windowTitle			= "Particle renderer";
		cfg.shaderDirectories	= { FG_DATA_PATH "../shaderlib", FG_DATA_PATH "shaders" };
		cfg.dbgOutputPath		= FG_DATA_PATH "_debug_output";
		cfg.vsync				= true;

		CHECK_ERR( _CreateFrameGraph( cfg ));

		_linearSampler = _frameGraph->CreateSampler( SamplerDesc{}.SetAddressMode( EAddressMode::Repeat )
								.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear )).Release();
		
		_linearClampSampler = _frameGraph->CreateSampler( SamplerDesc{}.SetAddressMode( EAddressMode::ClampToEdge )
								.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear )).Release();

		_cameraUB = _frameGraph->CreateBuffer( BufferDesc{ SizeOf<CameraUB>, EBufferUsage::Uniform | EBufferUsage::Transfer }, Default, "CameraUB" );
		CHECK_ERR( _cameraUB );
		
		_particlesUB = _frameGraph->CreateBuffer( BufferDesc{ SizeOf<ParticlesUB>, EBufferUsage::Uniform | EBufferUsage::Transfer }, Default, "ParticlesUB" );
		CHECK_ERR( _particlesUB );

		_particlesBuf = _frameGraph->CreateBuffer( BufferDesc{ SizeOf<ParticleVertex> * _maxParticles, EBufferUsage::Storage | EBufferUsage::Vertex }, Default, "Particles" );
		CHECK_ERR( _particlesBuf );
		
		_initialized	= false;
		_reloadShaders	= true;
		_particleMode	= EParticleDrawMode::Rays;
		_blendMode		= EBlendMode::Additive;
		_numParticles	= _maxParticles / 4;
		_startTime		= CurrentTime();
		_numSteps		= 20;
		
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
		
		GetFPSCamera().SetPosition({ 0.0f, 0.0f, 10.0f });
		return true;
	}

/*
=================================================
	_LoadShader
=================================================
*/
	String  ParticlesApp::_LoadShader (StringView filename)
	{
		FileRStream		file{ String{FG_DATA_PATH} << filename };
		CHECK_ERR( file.IsOpen() );

		String	str;
		CHECK_ERR( file.Read( size_t(file.Size()), OUT str ));

		return str;
	}

/*
=================================================
	DrawScene
=================================================
*/
	bool  ParticlesApp::DrawScene ()
	{
		CommandBuffer	cmdbuf		= _frameGraph->Begin( CommandBufferDesc{ EQueueType::Graphics });
		const uint2		sw_dim		= GetSurfaceSize();
		uint2			surf_dim	= uint2(float2(sw_dim) * (_halfSurfaceSize ? 0.5f : 1.0f) + 0.5f);
		
		if ( IsActiveVR() )
		{
			surf_dim = uint2(float2(GetVRDevice()->GetRenderTargetDimension()) * (_halfSurfaceSize ? 0.5f : 1.0f) + 0.5f);
		}

		// update
		{
			_UpdateUI();
			_UpdateCamera();
			
			CameraUB			camera;
			camera.proj			= GetCamera().projection;
			camera.modelView	= GetCamera().ToModelViewMatrix();
			camera.modelViewProj= GetCamera().ToModelViewProjMatrix();
			camera.viewport		= float2(surf_dim);
			camera.clipPlanes	= VecCast(GetViewRange());

			ParticlesUB			particle;
			particle.timeDelta	= _GetTimeStep();
			particle.steps		= Clamp( uint(FrameTime().count() / particle.timeDelta + 0.5f), 1u, _numSteps );
			particle.globalTime	= std::chrono::duration_cast<SecondsF>(CurrentTime() - _startTime).count();

			cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _cameraUB ).AddData( &camera, 1 ));
			cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _particlesUB ).AddData( &particle, 1 ));
		}

		_ReloadShaders( cmdbuf );

		// update
		if ( _updateParticlesPpln )
		{
			DispatchCompute		comp;
			comp.SetPipeline( _updateParticlesPpln );
			comp.AddResources( DescriptorSetID{"0"}, &_updateParticlesRes );
			comp.SetLocalSize( uint2{_localSize, 1} );
			comp.Dispatch( uint2{(_numParticles + _localSize - 1) / _localSize, 1} );

			cmdbuf->AddTask( comp );
		}

		// draw
		{
			// resize
			if ( not _colorBuffer1 or Any( _frameGraph->GetDescription(_colorBuffer1).dimension.xy() != surf_dim ))
			{
				_frameGraph->ReleaseResource( _colorBuffer1 );
				_frameGraph->ReleaseResource( _colorBuffer2 );
				_frameGraph->ReleaseResource( _depthBuffer );

				_colorBuffer1 = _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{surf_dim}, EPixelFormat::RGBA8_UNorm,
																	EImageUsage::ColorAttachment | EImageUsage::Sampled | EImageUsage::Transfer | EImageUsage::Storage },
															Default, "ColorBuffer1" );
				_colorBuffer2 = _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{surf_dim}, EPixelFormat::RGBA8_UNorm,
																	EImageUsage::ColorAttachment | EImageUsage::Sampled | EImageUsage::Transfer | EImageUsage::Storage },
															Default, "ColorBuffer2" );
				_depthBuffer = _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{surf_dim}, EPixelFormat::Depth24_Stencil8,
																	EImageUsage::DepthStencilAttachment | EImageUsage::Sampled },
															Default, "DepthBuffer" );
				CHECK( _colorBuffer1 and _colorBuffer2 and _depthBuffer );
			}

			if ( IsActiveVR() )
			{
				_DrawParticles( cmdbuf, _colorBuffer1 );
				_DrawParticles( cmdbuf, _colorBuffer2 );

				CHECK_ERR( _frameGraph->Execute( cmdbuf ));
				CHECK_ERR( _frameGraph->Flush() );

				const auto&			queue		= GetVulkan().GetVkQueues()[0];
				const auto&			vk_desc_l	= std::get<VulkanImageDesc>( _frameGraph->GetApiSpecificDescription( _colorBuffer1 ));
				const auto&			vk_desc_r	= std::get<VulkanImageDesc>( _frameGraph->GetApiSpecificDescription( _colorBuffer2 ));
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
				_DrawParticles( cmdbuf, _colorBuffer1 );

				// copy to swapchain image
				{
					RawImageID	sw_image = cmdbuf->GetSwapchainImage( GetSwapchain() );

					cmdbuf->AddTask( BlitImage{}.From( _colorBuffer1 ).To( sw_image ).SetFilter( EFilter::Linear )
												.AddRegion( {}, int2(0), int2(surf_dim), {}, int2(0), int2(sw_dim) ));
				}
			
				// draw ui
				#ifdef FG_ENABLE_IMGUI
				{
					auto&	draw_data = *ImGui::GetDrawData();

					if ( draw_data.TotalVtxCount > 0 )
					{
						RawImageID		sw_image = cmdbuf->GetSwapchainImage( GetSwapchain() );

						LogicalPassID	pass_id = cmdbuf->CreateRenderPass( RenderPassDesc{ int2{float2{ draw_data.DisplaySize.x, draw_data.DisplaySize.y }} }
														.AddViewport(float2{ draw_data.DisplaySize.x, draw_data.DisplaySize.y })
														.AddTarget( RenderTargetID::Color_0, sw_image, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store ));

						_uiRenderer.Draw( cmdbuf, pass_id );
					}
				}
				#endif
				
				CHECK_ERR( _frameGraph->Execute( cmdbuf ));
				CHECK_ERR( _frameGraph->Flush() );
			}

			_SetLastCommandBuffer( cmdbuf );
		}
		return true;
	}
	
/*
=================================================
	_DrawParticles
=================================================
*/
	void  ParticlesApp::_DrawParticles (const CommandBuffer &cmdbuf, RawImageID colorBuffer)
	{
		if ( _dotsParticlesPpln and _raysParticlesPpln )
		{
			const uint2		surf_dim = _frameGraph->GetDescription( colorBuffer ).dimension.xy();

			LogicalPassID	pass_id = cmdbuf->CreateRenderPass( RenderPassDesc{ surf_dim }
										.AddViewport( surf_dim )
										.AddTarget( RenderTargetID::Color_0, colorBuffer, RGBA32f{0.0f}, EAttachmentStoreOp::Store )
										.AddTarget( RenderTargetID::Depth, _depthBuffer, DepthStencil{1.0f}, EAttachmentStoreOp::Store ));
			CHECK_ERR( pass_id, void());

			DrawVertices	draw;
			draw.AddBuffer( Default, _particlesBuf );
			draw.SetVertexInput( VertexInputState{}.Bind( Default, SizeOf<ParticleVertex> )
									.Add( VertexID{"in_Position"},	&ParticleVertex::position )
									.Add( VertexID{"in_Color"},		&ParticleVertex::color )
									.Add( VertexID{"in_Size"},		&ParticleVertex::size )
									.Add( VertexID{"in_Velocity"},	&ParticleVertex::velocity ));
			draw.SetTopology( EPrimitive::Point );
			draw.AddResources( DescriptorSetID{"0"}, &_drawParticlesRes );
			draw.SetDepthTestEnabled( false );
			draw.SetDepthWriteEnabled( false );
			draw.Draw( _numParticles );
				
			BEGIN_ENUM_CHECKS();
			switch ( _particleMode )
			{
				case EParticleDrawMode::Dots :		draw.SetPipeline( _dotsParticlesPpln );		break;
				case EParticleDrawMode::Rays :		draw.SetPipeline( _raysParticlesPpln );		break;
				case EParticleDrawMode::Unknown :
				default :							return;
			}
			END_ENUM_CHECKS();

			BEGIN_ENUM_CHECKS();
			switch ( _blendMode )
			{
				case EBlendMode::None :		break;
				case EBlendMode::Additive :	draw.AddColorBuffer( RenderTargetID::Color_0, EBlendFactor::One, EBlendFactor::One, EBlendOp::Add );	break;
			}
			END_ENUM_CHECKS();

			cmdbuf->AddTask( pass_id, draw );

			cmdbuf->AddTask( SubmitRenderPass{ pass_id });
		}
		else
		{
			cmdbuf->AddTask( ClearColorImage{}.SetImage( colorBuffer ).Clear( RGBA32f{0.2f} ).AddRange( 0_mipmap, 1, 0_layer, 1 ));
		}
	}

/*
=================================================
	OnKey
=================================================
*/
	void  ParticlesApp::OnKey (StringView key, EKeyAction action)
	{
		BaseSceneApp::OnKey( key, action );

		if ( action == EKeyAction::Down )
		{
			if ( key == "R" )	{ _reloadShaders = true; }
			if ( key == "I" )	{ _reloadShaders = true;  _initialized = false; }
			if ( key == "U" )	{ _debugPixel = GetMousePos() / vec2(GetSurfaceSize().x, GetSurfaceSize().y); }
			if ( key == "P" )	_ResetPosition();
		}
		
	#ifdef FG_ENABLE_IMGUI
		if ( key == "left mb" )		_mouseJustPressed[0] = action;
		if ( key == "right mb" )	_mouseJustPressed[1] = action;
		if ( key == "middle mb" )	_mouseJustPressed[2] = action;
	#endif
	}
	
/*
=================================================
	_ReloadShaders
=================================================
*/
	void  ParticlesApp::_ReloadShaders (const CommandBuffer &cmdbuf)
	{
		if ( not _reloadShaders )
			return;
		
		FG_LOGI( "\n========================= Reload shaders =========================\n" );

		_reloadShaders = false;

		// init particles for simulation
		if ( not _initialized )
		{
			ComputePipelineDesc	desc;
			desc.AddShader( EShaderLangFormat::VKSL_110, "main", "#define MODE "s + ToString(_curMode) + "\n"s + _LoadShader("shaders/init_simulation.glsl") );

			CPipelineID	ppln = _frameGraph->CreatePipeline( desc );
			if ( ppln )
			{
				PipelineResources	res;
				CHECK( _frameGraph->InitPipelineResources( ppln, DescriptorSetID{"0"}, OUT res ));

				res.BindBuffer( UniformID{"ParticleSSB"}, _particlesBuf );
				
				DispatchCompute		comp;
				comp.SetPipeline( ppln );
				comp.AddResources( DescriptorSetID{"0"}, &res );
				comp.SetLocalSize( uint2{_localSize, 1} );
				comp.Dispatch( uint2{(_maxParticles + _localSize - 1) / _localSize, 1} );
				cmdbuf->AddTask( comp );

				_frameGraph->ReleaseResource( ppln );

				_initialized	= true;
				_startTime		= CurrentTime();
			}
		}

		// particle simulation
		if ( _initialized )
		{
			ComputePipelineDesc	desc;
			desc.AddShader( EShaderLangFormat::VKSL_110, "main", "#define MODE "s + ToString(_curMode) + "\n"s + _LoadShader("shaders/simulation.glsl") );

			CPipelineID	ppln = _frameGraph->CreatePipeline( desc );
			if ( ppln )
			{
				_frameGraph->ReleaseResource( _updateParticlesPpln );
				_updateParticlesPpln = std::move(ppln);
				
				CHECK( _frameGraph->InitPipelineResources( _updateParticlesPpln, DescriptorSetID{"0"}, OUT _updateParticlesRes ));
				_updateParticlesRes.BindBuffer( UniformID{"ParticleSSB"}, _particlesBuf );
				_updateParticlesRes.BindBuffer( UniformID{"ParticleUB"},  _particlesUB );
			}
		}

		// dot particles
		{
			GraphicsPipelineDesc	desc;

			String	sh_source = _LoadShader("shaders/particles_dots.glsl");
			desc.AddShader( EShader::Vertex,   EShaderLangFormat::VKSL_110, "main", "#define SHADER SH_VERTEX\n"   + sh_source );
			desc.AddShader( EShader::Geometry, EShaderLangFormat::VKSL_110, "main", "#define SHADER SH_GEOMETRY\n" + sh_source );
			desc.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_110, "main", "#define SHADER SH_FRAGMENT\n" + sh_source );

			GPipelineID	ppln = _frameGraph->CreatePipeline( desc );
			if ( ppln )
			{
				_frameGraph->ReleaseResource( _dotsParticlesPpln );
				_dotsParticlesPpln = std::move(ppln);
				CHECK( _frameGraph->InitPipelineResources( _dotsParticlesPpln, DescriptorSetID{"0"}, OUT _drawParticlesRes ));
				
				_drawParticlesRes.BindBuffer( UniformID{"CameraUB"}, _cameraUB );
			}
		}

		// ray particles
		{
			GraphicsPipelineDesc	desc;

			String	sh_source = _LoadShader("shaders/particles_rays.glsl");
			desc.AddShader( EShader::Vertex,   EShaderLangFormat::VKSL_110, "main", "#define SHADER SH_VERTEX\n"   + sh_source );
			desc.AddShader( EShader::Geometry, EShaderLangFormat::VKSL_110, "main", "#define SHADER SH_GEOMETRY\n" + sh_source );
			desc.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_110, "main", "#define SHADER SH_FRAGMENT\n" + sh_source );

			GPipelineID	ppln = _frameGraph->CreatePipeline( desc );
			if ( ppln )
			{
				_frameGraph->ReleaseResource( _raysParticlesPpln );
				_raysParticlesPpln = std::move(ppln);
				CHECK( _frameGraph->InitPipelineResources( _raysParticlesPpln, DescriptorSetID{"0"}, OUT _drawParticlesRes ));
				
				_drawParticlesRes.BindBuffer( UniformID{"CameraUB"}, _cameraUB );
			}
		}
	}

/*
=================================================
	_ResetPosition
=================================================
*/
	void  ParticlesApp::_ResetPosition ()
	{
		GetFPSCamera().SetPosition({ 0.0f, 0.0f, 0.0f });
	}
	
/*
=================================================
	_ResetOrientation
=================================================
*/
	void  ParticlesApp::_ResetOrientation ()
	{
		GetFPSCamera().SetRotation( Quat_Identity );
	}
	
/*
=================================================
	_GetTimeStep
=================================================
*/
	float  ParticlesApp::_GetTimeStep () const
	{
		return 0.01f * (_timeScale >= -0.0f ? Lerp( 1.0f, 10.0f, _timeScale ) : Lerp( 0.001f, 1.0f, 1.0f + _timeScale ));
	}


#ifdef FG_ENABLE_IMGUI
/*
=================================================
	_UpdateUI
=================================================
*/
	bool  ParticlesApp::_UpdateUI ()
	{
		ImGuiIO &	io = ImGui::GetIO();
		CHECK_ERR( io.Fonts->IsBuilt() );

		io.DisplaySize	= ImVec2{float(GetSurfaceSize().x), float(GetSurfaceSize().y)};
		io.DeltaTime	= FrameTime().count();

		CHECK_ERR( _UpdateInput() );

		ImGui::NewFrame();
		//ImGui::SetNextWindowSize( ImVec2{ 400, 300 });
			
		if ( ImGui::Begin( "Particle settings", &_settingsWndOpen ))
		{
			ImGui::Text( "Particle mode:" );
			ImGui::RadioButton( " dots", Cast<int>(&_particleMode), int(EParticleDrawMode::Dots) );
			ImGui::RadioButton( " rays", Cast<int>(&_particleMode), int(EParticleDrawMode::Rays) );
			ImGui::Separator();

			ImGui::Text( "Blend mode:" );
			ImGui::RadioButton( " none",     Cast<int>(&_blendMode), int(EBlendMode::None) );
			ImGui::RadioButton( " additive", Cast<int>(&_blendMode), int(EBlendMode::Additive) );
			ImGui::Separator();
			
			ImGui::Text( "Particle count:" );
			ImGui::SliderInt( "##ParticleCount", Cast<int>(&_numParticles), 1, _maxParticles );
			ImGui::Text( ("Time step: "s + ToString(_GetTimeStep())).c_str() );
			ImGui::SliderFloat( "##TimeScale", &_timeScale, -1.0f, 1.0f );
			ImGui::Text( "Max steps:" );
			ImGui::SliderInt( "##MaxSteps", Cast<int>(&_numSteps), 1, _maxSteps );
			ImGui::Separator();

			ImGui::Checkbox( "Surface half size", &_halfSurfaceSize );
			ImGui::Separator();
			
			ImGui::Text( "Sample:" );
			ImGui::RadioButton( " 1", Cast<int>(&_newMode), 1 );
			ImGui::RadioButton( " 2", Cast<int>(&_newMode), 3 );
			ImGui::RadioButton( " 3", Cast<int>(&_newMode), 4 );
			ImGui::RadioButton( " 4", Cast<int>(&_newMode), 5 );
			
			ImGui::Separator();

			if ( ImGui::Button( "Restart (I)" ))
			{
				_reloadShaders	= true;
				_initialized	= false;
			}

			if ( ImGui::Button( "Reset position (P)" ))
				_ResetPosition();
				
			if ( ImGui::Button( "Reset orientation" ))
				_ResetOrientation();

			ImGui::Separator();
		}

		ImGui::End();
		ImGui::Render();

		if ( _newMode != _curMode )
		{
			_curMode		= _newMode;
			_reloadShaders	= true;
			_initialized	= false;
		}

		return true;
	}

/*
=================================================
	_UpdateInput
=================================================
*/
	bool ParticlesApp::_UpdateInput ()
	{
		ImGuiIO &	io = ImGui::GetIO();

		for (size_t i = 0; i < _mouseJustPressed.size(); ++i)
		{
			io.MouseDown[i] = (_mouseJustPressed[i] != EKeyAction::Up);
		}

		io.MousePos = { GetMousePos().x, GetMousePos().y };

		memset( io.NavInputs, 0, sizeof(io.NavInputs) );
		return true;
	}

#else
	
	bool  ParticlesApp::_UpdateUI ()
	{
		return false;
	}

#endif

}	// FG

/*
=================================================
	main
=================================================
*/
int main ()
{
	using namespace FG;

	auto	app = MakeShared<ParticlesApp>();

	CHECK_ERR( app->Initialize(), -1 );

	for (; app->Update();) {}
	
	return 0;
}
