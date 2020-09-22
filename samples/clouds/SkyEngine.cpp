#include "SkyEngine.h"
#include "stl/Stream/FileStream.h"

namespace FG
{

	#define WORKGROUP_SIZE 32
	
/*
=================================================
	destructor
=================================================
*/
	SkyEngine::~SkyEngine ()
	{
		if ( _frameGraph )
		{
			_CleanupTextures();
			_CleanupRenderTargets();
			_CleanupShaders();
			_CleanupGeometry();
			_CleanupOffscreenPass();
		}
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool  SkyEngine::Initialize ()
	{
		// set path to resources
		{
			std::error_code	err;
			FS::current_path( FS::path{FG_DATA_PATH}, err );
		}

		{
			AppConfig	cfg;
			cfg.surfaceSize		= uint2(WIDTH, HEIGHT);
			cfg.windowTitle		= "Sky Engine";
			cfg.dbgOutputPath	= "_debug_output";

			CHECK_ERR( _CreateFrameGraph( cfg ));
		}

		_SetupCamera( 45_deg, {0.1f, 1000.0f} );
		_SetMouseSens({ -0.01f, 0.01f });

		_aspect = float(GetSurfaceSize().x) / GetSurfaceSize().y;

		_skySystem = SkyManager();

		CHECK_ERR( _InitializeTextures() );
		CHECK_ERR( _InitializeRenderTargets() );
		CHECK_ERR( _SetupOffscreenPass() );
		CHECK_ERR( _InitializeGeometry() );
		CHECK_ERR( _InitializeShaders() );

		_startTime			= CurrentTime();
		_renderTargetSize	= GetSurfaceSize();

		return true;
	}
	
/*
=================================================
	_InitializeTextures
=================================================
*/
	bool  SkyEngine::_InitializeTextures ()
	{
		_meshTexture.reset( new Texture(_frameGraph));
		CHECK_ERR( _meshTexture->initFromFile("Textures/rockColor.png"));

		_meshPBRInfo.reset( new Texture(_frameGraph));
		CHECK_ERR( _meshPBRInfo->initFromFile("Textures/rockPBRinfo.png"));

		_meshNormals.reset( new Texture(_frameGraph));
		CHECK_ERR( _meshNormals->initFromFile("Textures/rockNormal.png"));

		_cloudPlacementTexture.reset( new Texture(_frameGraph));
		CHECK_ERR( _cloudPlacementTexture->initFromFile("Textures/CloudPlacement.png"));

		_nightSkyTexture.reset( new Texture(_frameGraph));
		CHECK_ERR( _nightSkyTexture->initFromFile("Textures/NightSky/nightSky_noOrange.png"));

		_cloudCurlNoise.reset( new Texture(_frameGraph));
		CHECK_ERR( _cloudCurlNoise->initFromFile("Textures/CurlNoiseFBM.png"));

		_lowResCloudShapeTexture3D.reset( new Texture3D(_frameGraph, uint3{128, 128, 128}));
		CHECK_ERR( _lowResCloudShapeTexture3D->initFromFile("Textures/3DTextures/lowResCloudShape/lowResCloud")); // note: no .png

		_hiResCloudShapeTexture3D.reset( new Texture3D(_frameGraph, uint3{32, 32, 32}));
		CHECK_ERR( _hiResCloudShapeTexture3D->initFromFile("Textures/3DTextures/hiResCloudShape/hiResClouds ")); // note: no .png

		return true;
	}

/*
=================================================
	_CleanupTextures
=================================================
*/
	void  SkyEngine::_CleanupTextures ()
	{
		_meshTexture = null;
		_meshPBRInfo = null;
		_meshNormals = null;
		_cloudPlacementTexture = null;
		_nightSkyTexture = null;
		_cloudCurlNoise = null;
		_lowResCloudShapeTexture3D = null;
		_hiResCloudShapeTexture3D = null;
	}
	
/*
=================================================
	_InitializeRenderTargets
=================================================
*/
	bool  SkyEngine::_InitializeRenderTargets ()
	{
		if (not _backgroundTexture) _backgroundTexture.reset( new Texture(_frameGraph, EPixelFormat::RGBA32F));
		CHECK_ERR( _backgroundTexture->initForStorage( GetSurfaceSize() ));

		if (not _backgroundTexturePrev) _backgroundTexturePrev.reset( new Texture(_frameGraph, EPixelFormat::RGBA32F));
		CHECK_ERR( _backgroundTexturePrev->initForStorage( GetSurfaceSize() ));

		if (not _depthTexture) _depthTexture.reset( new Texture(_frameGraph));
		CHECK_ERR( _depthTexture->initForDepthAttachment( GetSurfaceSize() ));
		
		return true;
	}
	
/*
=================================================
	_CleanupRenderTargets
=================================================
*/
	void  SkyEngine::_CleanupRenderTargets ()
	{
		if (_backgroundTexture)		_backgroundTexture->cleanup();
		if (_backgroundTexturePrev)	_backgroundTexturePrev->cleanup();
		if (_depthTexture)			_depthTexture->cleanup();
	}
	
/*
=================================================
	_InitializeShaders
=================================================
*/
	bool  SkyEngine::_InitializeShaders ()
	{
		_meshShader.reset( new MeshShader(_frameGraph, 
			std::string("Shaders/model.vert"), std::string("Shaders/model.frag"), _meshTexture.get(), _meshPBRInfo.get(), _meshNormals.get(),
			_cloudPlacementTexture.get(), _lowResCloudShapeTexture3D.get()));
	
		_backgroundShader.reset( new BackgroundShader(_frameGraph,
			std::string("Shaders/background.vert"), std::string("Shaders/background.frag"), _backgroundTexture.get(), _backgroundTexturePrev.get()));

		// Note: we pass the background shader's texture with the intention of writing to it with the compute shader
		_reprojectShader.reset( new ReprojectShader(_frameGraph,
			std::string("Shaders/reproject.comp"), _backgroundTexture.get(), _backgroundTexturePrev.get()));

		_computeShader.reset( new ComputeShader(_frameGraph, _reprojection,
			std::string("Shaders/compute-clouds.comp"), _backgroundTexture.get(), _backgroundTexturePrev.get(), _cloudPlacementTexture.get(),
			_nightSkyTexture.get(), _cloudCurlNoise.get(), _lowResCloudShapeTexture3D.get(), _hiResCloudShapeTexture3D.get()));

		// Post shaders: there will be many
		// This is still offscreen, so the render pass is the offscreen render pass
		_godRayShader.reset( new PostProcessShader(_frameGraph,
			std::string("Shaders/post-pass.vert"), std::string("Shaders/god-ray.frag"), _offscreenPass.colorBuffer[0].get()));

		_radialBlurShader.reset( new PostProcessShader(_frameGraph,
			std::string("Shaders/post-pass.vert"), std::string("Shaders/radialBlur.frag"), _offscreenPass.colorBuffer[1].get()));

		_toneMapShader.reset( new PostProcessShader(_frameGraph,
			std::string("Shaders/post-pass.vert"), std::string("Shaders/tonemap.frag"), _offscreenPass.colorBuffer[2].get()));

		return true;
	}
	
/*
=================================================
	_CleanupShaders
=================================================
*/
	void  SkyEngine::_CleanupShaders ()
	{
		_meshShader = null;
		_backgroundShader = null;
		_computeShader = null;
		_reprojectShader = null;
		_toneMapShader = null;
		_godRayShader = null;
		_radialBlurShader = null;
	}
	
/*
=================================================
	_InitializeGeometry
=================================================
*/
	bool  SkyEngine::_InitializeGeometry ()
	{
		_sceneGeometry.reset( new Geometry(_frameGraph));
		CHECK_ERR( _sceneGeometry->setupFromMesh("Models/terrain.obj"));

		_backgroundGeometry.reset( new Geometry(_frameGraph));
		CHECK_ERR( _backgroundGeometry->setupAsBackgroundQuad() );

		return true;
	}
	
/*
=================================================
	_CleanupGeometry
=================================================
*/
	void  SkyEngine::_CleanupGeometry ()
	{
		_sceneGeometry = null;
		_backgroundGeometry = null;
	}

/*
=================================================
	_SetupOffscreenPass
=================================================
*/
	bool  SkyEngine::_SetupOffscreenPass ()
	{
		if ( not _offscreenPass.sampler )
		{
			SamplerDesc	desc;
			desc.SetAddressMode( EAddressMode::ClampToEdge );
			desc.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear );
			desc.mipLodBias = 0.0f;
			desc.minLod = 0.0f;
			desc.maxLod = 1.0f;
			desc.borderColor = EBorderColor::FloatOpaqueWhite;

			_offscreenPass.sampler = _frameGraph->CreateSampler( desc ).Release();
		}

		// color buffer
		for (size_t i = 0; i < CountOf(_offscreenPass.colorBuffer); ++i)
		{
			if (not _offscreenPass.colorBuffer[i])
				_offscreenPass.colorBuffer[i].reset( new Texture(_frameGraph, EPixelFormat::RGBA32F, _offscreenPass.sampler));

			CHECK_ERR( _offscreenPass.colorBuffer[i]->initForColorAttachment(GetSurfaceSize()));
		}

		// depth buffer
		for (size_t i = 0; i < CountOf(_offscreenPass.depthBuffer); ++i)
		{
			if (not _offscreenPass.depthBuffer[i])
				_offscreenPass.depthBuffer[i].reset( new Texture(_frameGraph, EPixelFormat::Depth32F, _offscreenPass.sampler));

			CHECK_ERR( _offscreenPass.depthBuffer[i]->initForDepthAttachment(GetSurfaceSize()));
		}

		return true;
	}
	
/*
=================================================
	_CleanupOffscreenPass
=================================================
*/
	void  SkyEngine::_CleanupOffscreenPass ()
	{
		for (size_t i = 0; i < CountOf(_offscreenPass.colorBuffer); ++i) {
			if (_offscreenPass.colorBuffer[i])
				_offscreenPass.colorBuffer[i]->cleanup();
		}
		for (size_t i = 0; i < CountOf(_offscreenPass.depthBuffer); ++i) {
			if (_offscreenPass.depthBuffer[i])
				_offscreenPass.depthBuffer[i]->cleanup();
		}
	}

/*
=================================================
	_UpdateUniformBuffer
=================================================
*/
	void  SkyEngine::_UpdateUniformBuffer (const CommandBuffer &cmdbuf)
	{
		const float	time = std::chrono::duration_cast<SecondsF>( CurrentTime() - _startTime ).count();

		_UpdateCamera();

		UniformCameraObject ucoPrev = {};
		ucoPrev.proj = _prevProjection;
		ucoPrev.proj[1][1] *= -1;
		ucoPrev.view = _prevView;
		ucoPrev.cameraPosition = glm::vec4(_prevPos, 1.0f);

		UniformCameraObject uco = {};
		uco.proj = GetCamera().projection;
		uco.proj[1][1] *= -1; // :(
		uco.view = GetCamera().ToViewMatrix();
		uco.cameraPosition = glm::vec4(GetCamera().transform.position, 1.0f);
		uco.cameraParams.x = _aspect;
		uco.cameraParams.y = Tan(0.5f * GetCameraFov());

		UniformModelObject umo = {};
		umo.model = glm::mat4(1.0f);
		umo.model[0][0] = 100.0f;
		umo.model[2][2] = 100.0f;
		umo.invTranspose = glm::inverse(glm::transpose(umo.model));
		float interp = sin(time * 0.025f);

		_skySystem.rebuildSkyFromNewSun(interp * 0.5f, 0.25f);
		_skySystem.setTime(time * 2.f);

		UniformSkyObject sky = _skySystem.getSky();
		UniformSunObject& sun = _skySystem.getSun(); // by reference so we can update the pixel counter in sun.color.a below
	
		// Pass a uniform value in sun.color.a indicating which of the 16 pixels should be updated.
		// Yes, this should have its own uniform but you would have to pad to sizeof(vec4) and we are not even using 
		// this channel already. Will probably change later.
		sun.color.a = float((int(sun.color.a) + 1) % 16); // update every 16th pixel

		_computeShader->updateUniformBuffers(cmdbuf, uco, ucoPrev, sky, sun);
		_reprojectShader->updateUniformBuffers(cmdbuf, uco, ucoPrev, sky, sun);
		_meshShader->updateUniformBuffers(cmdbuf, uco, umo, sun, sky);
		_godRayShader->updateUniformBuffers(cmdbuf, uco, sun);
		_radialBlurShader->updateUniformBuffers(cmdbuf, uco, sun);

		_prevProjection	= GetCamera().projection;
		_prevView		= GetCamera().ToViewMatrix();
		_prevPos		= GetCamera().transform.position;
	}

/*
=================================================
	DrawScene
=================================================
*/
	bool  SkyEngine::DrawScene ()
	{
		CommandBuffer	cmdbuf		= _frameGraph->Begin( CommandBufferDesc{ EQueueType::Graphics });
		RawImageID		sw_image	= cmdbuf->GetSwapchainImage( GetSwapchain() );
		const uint2		sw_dim		= _frameGraph->GetDescription( sw_image ).dimension.xy();
		LogicalPassID	pass_id;
		
		_UpdateUniformBuffer( cmdbuf );

		if ( Any( _renderTargetSize != sw_dim ))
		{
			_renderTargetSize = sw_dim;
			_CleanupRenderTargets();
			_CleanupOffscreenPass();
			CHECK_ERR( _InitializeRenderTargets() );
			CHECK_ERR( _SetupOffscreenPass() );
		}

		// reprojection pass
		if ( _reprojection )
		{
			DispatchCompute	task;
			task.SetLocalSize( WORKGROUP_SIZE, WORKGROUP_SIZE );
			task.Dispatch({ (sw_dim.x + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE, (sw_dim.y + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE, 1 });

			_reprojectShader->bindShader( INOUT task );
			cmdbuf->AddTask( task );
		}

		// cloud ray tracing
		{
			DispatchCompute	task;
			const uint2		dim = _reprojection ? sw_dim / 4 : sw_dim;
			task.SetLocalSize( WORKGROUP_SIZE, WORKGROUP_SIZE );
			task.Dispatch({ (dim.x + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE, (dim.y + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE, 1 });

			_computeShader->bindShader( INOUT task );
			
			if ( _debugPixel )
			{
				task.EnableDebugTrace(uint3{ uint(_debugPixel->x), uint(_debugPixel->y), 0u });
				_debugPixel.reset();
			}

			cmdbuf->AddTask( task );
		}


		// draw background
		pass_id = cmdbuf->CreateRenderPass( RenderPassDesc{sw_dim }
								.AddViewport( sw_dim )
								.AddTarget( RenderTargetID::Color_0, _offscreenPass.colorBuffer[0]->Image(), RGBA32f{0.0f}, EAttachmentStoreOp::Store )
								.AddTarget( RenderTargetID::Depth, _offscreenPass.depthBuffer[0]->Image(), DepthStencil{1.0f}, EAttachmentStoreOp::Store ));
		{
			DrawIndexed	task = _backgroundGeometry->enqueueDrawCommands();
			_backgroundShader->bindShader( INOUT task );
			task.SetVertexInput( VertexInputState{}
				.Bind( Default, SizeOf<Vertex> )
				.Add( VertexID{"inPosition"}, &Vertex::pos )
				.Add( VertexID{"inColor"}, &Vertex::col )
				.Add( VertexID{"inUV"},  &Vertex::uv ));
			cmdbuf->AddTask( pass_id, task );
		}
		cmdbuf->AddTask( SubmitRenderPass{ pass_id });


		// god rays and mesh drawing
		pass_id = cmdbuf->CreateRenderPass( RenderPassDesc{sw_dim }
								.AddViewport( sw_dim )
								.AddTarget( RenderTargetID::Color_0, _offscreenPass.colorBuffer[1]->Image(), RGBA32f{0.0f}, EAttachmentStoreOp::Store )
								.AddTarget( RenderTargetID::Depth, _offscreenPass.depthBuffer[1]->Image(), DepthStencil{1.0f}, EAttachmentStoreOp::Store ));
		{
			DrawIndexed	task = _backgroundGeometry->enqueueDrawCommands();
			_godRayShader->bindShader( INOUT task );
			task.SetVertexInput( VertexInputState{}
				.Bind( Default, SizeOf<Vertex> )
				.Add( VertexID{"inPosition"}, &Vertex::pos )
				.Add( VertexID{"inUV"},  &Vertex::uv ));
			cmdbuf->AddTask( pass_id, task );
		}
		cmdbuf->AddTask( SubmitRenderPass{ pass_id });


		// radial blur
		pass_id = cmdbuf->CreateRenderPass( RenderPassDesc{sw_dim }
								.AddViewport( sw_dim )
								.AddTarget( RenderTargetID::Color_0, _offscreenPass.colorBuffer[2]->Image(), RGBA32f{0.0f}, EAttachmentStoreOp::Store )
								.AddTarget( RenderTargetID::Depth, _offscreenPass.depthBuffer[2]->Image(), DepthStencil{1.0f}, EAttachmentStoreOp::Store ));
		{
			DrawIndexed	task = _backgroundGeometry->enqueueDrawCommands();
			_radialBlurShader->bindShader( INOUT task );
			task.SetVertexInput( VertexInputState{}
				.Bind( Default, SizeOf<Vertex> )
				.Add( VertexID{"inPosition"}, &Vertex::pos )
				.Add( VertexID{"inUV"},  &Vertex::uv ));
			cmdbuf->AddTask( pass_id, task );
		}


		// draw scene
		/*{
			DrawIndexed	task = _sceneGeometry->enqueueDrawCommands();
			_meshShader->bindShader( INOUT task );
			task.SetVertexInput( VertexInputState{}
				.Bind( Default, SizeOf<Vertex> )
				.Add( VertexID{"inPosition"}, &Vertex::pos )
				.Add( VertexID{"inColor"}, &Vertex::col )
				.Add( VertexID{"inUV"},  &Vertex::uv )
				.Add( VertexID{"inNormal"}, &Vertex::nor ));
			cmdbuf->AddTask( pass_id, task );
		}*/
		cmdbuf->AddTask( SubmitRenderPass{ pass_id });

		
		// Run the final post process that renders to the screen
		pass_id = cmdbuf->CreateRenderPass( RenderPassDesc{sw_dim }
								.AddViewport( sw_dim )
								.AddTarget( RenderTargetID::Color_0, sw_image, RGBA32f{0.0f}, EAttachmentStoreOp::Store ));
		{
			DrawIndexed	task = _backgroundGeometry->enqueueDrawCommands();
			_toneMapShader->bindShader( INOUT task );
			task.SetVertexInput( VertexInputState{}
				.Bind( Default, SizeOf<Vertex> )
				.Add( VertexID{"inPosition"}, &Vertex::pos )
				.Add( VertexID{"inUV"},  &Vertex::uv ));
			cmdbuf->AddTask( pass_id, task );
		}
		cmdbuf->AddTask( SubmitRenderPass{ pass_id });


		CHECK_ERR( _frameGraph->Execute( cmdbuf ));
		_SetLastCommandBuffer( cmdbuf );

		return true;
	}
	
/*
=================================================
	OnKey
=================================================
*/
	void  SkyEngine::OnKey (StringView key, EKeyAction action)
	{
		BaseSample::OnKey( key, action );

		if ( action == EKeyAction::Down )
		{
			if ( key == "R" )
			{
				_CleanupShaders();
				CHECK( _InitializeShaders() );
			}

			if ( key == "T" )
			{
				_reprojection = not _reprojection;
				_CleanupShaders();
				CHECK( _InitializeShaders() );
			}

			if ( key == "U" )	_debugPixel = GetMousePos();
		}
	}
	
/*
=================================================
	OnResize
=================================================
*/
	void  SkyEngine::OnResize (const uint2 &newSize)
	{
		BaseSceneApp::OnResize( newSize );
		
		if ( Any( newSize == uint2(0) ))
			return;
		
		_aspect = float(newSize.x) / newSize.y;
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

	auto	app = MakeShared<SkyEngine>();

	CHECK_ERR( app->Initialize(), -1 );

	for (; app->Update();) {}
	
	return 0;
}
