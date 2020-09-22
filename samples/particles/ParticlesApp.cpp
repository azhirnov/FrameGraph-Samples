// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ParticlesApp.h"
#include "pipeline_compiler/VPipelineCompiler.h"
#include "stl/Algorithms/StringUtils.h"

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
			_frameGraph->ReleaseResource( _colorBuffer[0] );
			_frameGraph->ReleaseResource( _colorBuffer[1] );
			_frameGraph->ReleaseResource( _depthBuffer );

			_frameGraph->ReleaseResource( _cameraUB[0] );
			_frameGraph->ReleaseResource( _cameraUB[1] );
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
		//cfg.vrMode				= AppConfig::EVRMode::Emulator;
		//cfg.enableDebugLayers	= false;
		//cfg.vsync				= true;

		CHECK_ERR( _CreateFrameGraph( cfg ));

		CHECK_ERR( _InitUI() );

		_cameraUB[0] = _frameGraph->CreateBuffer( BufferDesc{ SizeOf<CameraUB>, EBufferUsage::Uniform | EBufferUsage::Transfer }, Default, "CameraUB" );
		_cameraUB[1] = _frameGraph->CreateBuffer( BufferDesc{ SizeOf<CameraUB>, EBufferUsage::Uniform | EBufferUsage::Transfer }, Default, "CameraUB" );
		CHECK_ERR( _cameraUB[0] and _cameraUB[1] );
		
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

		_ResetPosition();
		return true;
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
		uint2			surf_dim	= _ScaleSurface( sw_dim, _sufaceScaleIdx );
		
		_UpdateCamera();

		// update camera
		if ( IsActiveVR() )
		{
			surf_dim = _ScaleSurface( GetVRDevice()->GetRenderTargetDimension(), _sufaceScaleIdx );

			auto&		vr = GetVRCamera();
			CameraUB	camera;

			camera.proj			= vr.ToProjectionMatrix()[0];
			camera.modelView	= vr.ToModelViewMatrix()[0];
			camera.modelViewProj= vr.ToModelViewProjMatrix()[0];
			camera.viewport		= float2(surf_dim);
			camera.clipPlanes	= VecCast(GetViewRange());
			cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _cameraUB[0] ).AddData( &camera, 1 ));
			
			camera.proj			= vr.ToProjectionMatrix()[1];
			camera.modelView	= vr.ToModelViewMatrix()[1];
			camera.modelViewProj= vr.ToModelViewProjMatrix()[1];
			cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _cameraUB[1] ).AddData( &camera, 1 ));
		}
		else
		{
			CameraUB	camera;
			camera.proj			= GetCamera().projection;
			camera.modelView	= GetCamera().ToModelViewMatrix();
			camera.modelViewProj= GetCamera().ToModelViewProjMatrix();
			camera.viewport		= float2(surf_dim);
			camera.clipPlanes	= VecCast(GetViewRange());
			cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _cameraUB[0] ).AddData( &camera, 1 ));
		}

		// update particles
		{
			ParticlesUB			particle;
			particle.timeDelta	= _GetTimeStep();
			particle.steps		= Clamp( uint(FrameTime().count() / particle.timeDelta + 0.5f), 1u, _numSteps );
			particle.globalTime	= std::chrono::duration_cast<SecondsF>(CurrentTime() - _startTime).count();
			cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _particlesUB ).AddData( &particle, 1 ));
		}

		_ReloadShaders( cmdbuf );

		// update
		if ( _updateParticlesPpln )
		{
			DispatchCompute		comp;
			comp.SetPipeline( _updateParticlesPpln );
			comp.AddResources( DescriptorSetID{"0"}, _updateParticlesRes );
			comp.SetLocalSize( uint2{_localSize, 1} );
			comp.Dispatch( uint2{(_numParticles + _localSize - 1) / _localSize, 1} );

			cmdbuf->AddTask( comp );
		}

		// draw
		{
			// resize
			if ( not _colorBuffer[0] or Any( _frameGraph->GetDescription(_colorBuffer[0]).dimension.xy() != surf_dim ))
			{
				_frameGraph->ReleaseResource( _colorBuffer[0] );
				_frameGraph->ReleaseResource( _colorBuffer[1] );
				_frameGraph->ReleaseResource( _depthBuffer );

				_colorBuffer[0] = _frameGraph->CreateImage( ImageDesc{}.SetDimension( surf_dim ).SetFormat( EPixelFormat::RGBA8_UNorm )
																.SetUsage( EImageUsage::ColorAttachment | EImageUsage::Sampled | EImageUsage::Transfer | EImageUsage::Storage ),
															Default, "ColorBuffer1" );
				_colorBuffer[1] = _frameGraph->CreateImage( ImageDesc{}.SetDimension( surf_dim ).SetFormat( EPixelFormat::RGBA8_UNorm )
																.SetUsage( EImageUsage::ColorAttachment | EImageUsage::Sampled | EImageUsage::Transfer | EImageUsage::Storage ),
															Default, "ColorBuffer2" );
				_depthBuffer = _frameGraph->CreateImage( ImageDesc{}.SetDimension( surf_dim ).SetFormat( EPixelFormat::Depth24_Stencil8 )
																.SetUsage( EImageUsage::DepthStencilAttachment | EImageUsage::Sampled ),
															Default, "DepthBuffer" );
				CHECK( _colorBuffer[0] and _colorBuffer[1] and _depthBuffer );
			}

			if ( IsActiveVR() )
			{
				_DrawParticles( cmdbuf, 0 );
				_DrawParticles( cmdbuf, 1 );

				CHECK_ERR( _frameGraph->Execute( cmdbuf ));
				CHECK_ERR( _frameGraph->Flush() );

				_VRPresent( GetVulkan().GetVkQueues()[0], _colorBuffer[0], _colorBuffer[1], true );
			}
			else
			{
				_DrawParticles( cmdbuf, 0 );

				// copy to swapchain image
				{
					RawImageID	sw_image = cmdbuf->GetSwapchainImage( GetSwapchain() );

					cmdbuf->AddTask( BlitImage{}.From( _colorBuffer[0] ).To( sw_image ).SetFilter( EFilter::Linear )
												.AddRegion( {}, int2(0), int2(surf_dim), {}, int2(0), int2(sw_dim) ));

					_DrawUI( cmdbuf, sw_image );
				}

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
	void  ParticlesApp::_DrawParticles (const CommandBuffer &cmdbuf, uint eye)
	{
		if ( _dotsParticlesPpln and _raysParticlesPpln )
		{
			const uint2		surf_dim = _frameGraph->GetDescription( _colorBuffer[eye] ).dimension.xy();

			LogicalPassID	pass_id = cmdbuf->CreateRenderPass( RenderPassDesc{ surf_dim }
										.AddViewport( surf_dim )
										.AddTarget( RenderTargetID::Color_0, _colorBuffer[eye], RGBA32f{0.0f}, EAttachmentStoreOp::Store )
										.AddTarget( RenderTargetID::Depth, _depthBuffer, DepthStencil{1.0f}, EAttachmentStoreOp::Store ));
			CHECK_ERRV( pass_id );
			
			_drawParticlesRes.BindBuffer( UniformID{"CameraUB"}, _cameraUB[eye] );

			DrawVertices	draw;
			draw.AddVertexBuffer( Default, _particlesBuf );
			draw.SetVertexInput( VertexInputState{}.Bind( Default, SizeOf<ParticleVertex> )
									.Add( VertexID{"in_Position"},	&ParticleVertex::position )
									.Add( VertexID{"in_Color"},		&ParticleVertex::color )
									.Add( VertexID{"in_Size"},		&ParticleVertex::size )
									.Add( VertexID{"in_Velocity"},	&ParticleVertex::velocity ));
			draw.SetTopology( EPrimitive::Point );
			draw.AddResources( DescriptorSetID{"0"}, _drawParticlesRes );
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
			cmdbuf->AddTask( ClearColorImage{}.SetImage( _colorBuffer[eye] ).Clear( RGBA32f{0.2f} ).AddRange( 0_mipmap, 1, 0_layer, 1 ));
		}
	}

/*
=================================================
	OnKey
=================================================
*/
	void  ParticlesApp::OnKey (StringView key, EKeyAction action)
	{
		BaseSample::OnKey( key, action );

		if ( action == EKeyAction::Down )
		{
			if ( key == "R" )	{ _reloadShaders = true; }
			if ( key == "I" )	{ _reloadShaders = true;  _initialized = false; }
			if ( key == "U" )	{ _debugPixel = GetMousePos() / vec2(GetSurfaceSize().x, GetSurfaceSize().y); }
			if ( key == "P" )	_ResetPosition();
		}
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
				comp.AddResources( DescriptorSetID{"0"}, res );
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
		GetFPSCamera().SetPosition({ 0.0f, 0.0f, 10.0f });
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


/*
=================================================
	OnUpdateUI
=================================================
*/
	void  ParticlesApp::OnUpdateUI ()
	{
	#ifdef FG_ENABLE_IMGUI
		ImGui::Text( "Particle mode:" );
		ImGui::RadioButton( " dots", INOUT Cast<int>(&_particleMode), int(EParticleDrawMode::Dots) );
		ImGui::RadioButton( " rays", INOUT Cast<int>(&_particleMode), int(EParticleDrawMode::Rays) );
		ImGui::Separator();

		ImGui::Text( "Blend mode:" );
		ImGui::RadioButton( " none",     INOUT Cast<int>(&_blendMode), int(EBlendMode::None) );
		ImGui::RadioButton( " additive", INOUT Cast<int>(&_blendMode), int(EBlendMode::Additive) );
		ImGui::Separator();
			
		ImGui::Text( "Particle count:" );
		ImGui::SliderInt( "##ParticleCount", INOUT Cast<int>(&_numParticles), 1, _maxParticles );
		ImGui::Text( ("Time step: "s + ToString(_GetTimeStep())).c_str() );
		ImGui::SliderFloat( "##TimeScale", INOUT &_timeScale, -1.0f, 1.0f );
		ImGui::Text( "Max steps:" );
		ImGui::SliderInt( "##MaxSteps", INOUT Cast<int>(&_numSteps), 1, _maxSteps );
		ImGui::Separator();
			
		ImGui::Text( "Surface scale" );
		ImGui::SliderInt( "##SurfaceScaleSlider", INOUT &_sufaceScaleIdx, -2, 1, _SurfaceScaleName( _sufaceScaleIdx ));
		ImGui::Separator();
			
		ImGui::Text( "Sample:" );
		ImGui::RadioButton( " 1", INOUT Cast<int>(&_newMode), 1 );
		ImGui::RadioButton( " 2", INOUT Cast<int>(&_newMode), 3 );
		ImGui::RadioButton( " 3", INOUT Cast<int>(&_newMode), 4 );
		ImGui::RadioButton( " 4", INOUT Cast<int>(&_newMode), 5 );
			
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

		if ( _newMode != _curMode )
		{
			_curMode		= _newMode;
			_reloadShaders	= true;
			_initialized	= false;
		}

	#endif
	}

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
