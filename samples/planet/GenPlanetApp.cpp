// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "GenPlanetApp.h"
#include "pipeline_compiler/VPipelineCompiler.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Stream/FileStream.h"

namespace FG
{

// config
namespace {
	static constexpr uint	Lod			= 9;
	static constexpr uint	FaceSize	= 2048;
	static constexpr float	TessLevel	= 12.0f;

	static const String		ShaderProjection = 
		IsSameTypes< SphericalCube::Projection_t, IdentitySphericalCube >   ?	"#define PROJECTION  CM_IdentitySC_Forward\n\n"s :
		IsSameTypes< SphericalCube::Projection_t, OriginCube >				?	"#define PROJECTION  CM_IdentitySC_Forward\n\n"s :
		IsSameTypes< SphericalCube::Projection_t, TangentialSphericalCube > ?	"#define PROJECTION  CM_TangentialSC_Forward\n\n"s :
																				"unknown projection\n\n"s;
}
	
/*
=================================================
	destructor
=================================================
*/
	GenPlanetApp::~GenPlanetApp ()
	{
		if ( _frameGraph )
		{
			_planet.cube.Destroy( _frameGraph );

			_frameGraph->ReleaseResource( _planet.pipeline );
			_frameGraph->ReleaseResource( _planet.heightMap );
			_frameGraph->ReleaseResource( _planet.normalMap );
			_frameGraph->ReleaseResource( _planet.albedoMap );
			_frameGraph->ReleaseResource( _planet.emissionMap );
			_frameGraph->ReleaseResource( _planet.ubuffer );
			
			_frameGraph->ReleaseResource( _colorBuffer );
			_frameGraph->ReleaseResource( _depthBuffer );
		}
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool  GenPlanetApp::Initialize ()
	{
		AppConfig	cfg;
		cfg.surfaceSize			= uint2(1024, 768);
		cfg.windowTitle			= "Planet generator";
		cfg.shaderDirectories	= { FG_DATA_PATH "../shaderlib", FG_DATA_PATH "shaders" };
		cfg.dbgOutputPath		= FG_DATA_PATH "_debug_output";

		CHECK_ERR( _CreateFrameGraph( cfg ));

		_linearSampler = _frameGraph->CreateSampler( SamplerDesc{}.SetAddressMode( EAddressMode::Repeat )
								.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear )).Release();
		
		_linearClampSampler = _frameGraph->CreateSampler( SamplerDesc{}.SetAddressMode( EAddressMode::ClampToEdge )
								.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear )).Release();

		GetFPSCamera().SetPosition({ 0.0f, 0.0f, 20.0f });
		_SetupCamera( 60_deg, vec2(0.1f, 200.0f) );
		
		return true;
	}
	
/*
=================================================
	_CreatePlanet
=================================================
*/
	bool  GenPlanetApp::_CreatePlanet (const CommandBuffer &cmdbuf)
	{
		const uint2		face_size { FaceSize };

		CHECK_ERR( _planet.cube.Create( cmdbuf, Lod, Lod, true ));

		// create height map
		if ( not _planet.heightMap )
		{
			_planet.heightMap = _frameGraph->CreateImage( ImageDesc{ EImage::TexCube, uint3{face_size}, EPixelFormat::R16F,
																	 EImageUsage::Storage | EImageUsage::Transfer | EImageUsage::Sampled, 6_layer },
														  Default, "Planet.HeightMap" );
			CHECK_ERR( _planet.heightMap );
		}
		
		// create normal map
		if ( not _planet.normalMap )
		{
			_planet.normalMap = _frameGraph->CreateImage( ImageDesc{ EImage::TexCube, uint3{face_size}, EPixelFormat::RGBA16F,
																	 EImageUsage::Storage | EImageUsage::Transfer | EImageUsage::Sampled, 6_layer },
														  Default, "Planet.NormalMap" );
			CHECK_ERR( _planet.normalMap );
		}

		// create albedo map
		if ( not _planet.albedoMap )
		{
			_planet.albedoMap = _frameGraph->CreateImage( ImageDesc{ EImage::TexCube, uint3{face_size}, EPixelFormat::RGBA8_UNorm,
																	 EImageUsage::Storage | EImageUsage::Transfer | EImageUsage::Sampled, 6_layer },
														  Default, "Planet.AlbedoMap" );
			CHECK_ERR( _planet.albedoMap );
		}

		// create material map
		if ( not _planet.emissionMap )
		{
			_planet.emissionMap = _frameGraph->CreateImage( ImageDesc{ EImage::TexCube, uint3{face_size}, EPixelFormat::RG16F,
																	   EImageUsage::Storage | EImageUsage::Transfer | EImageUsage::Sampled, 6_layer },
														    Default, "Planet.EmissionMap" );
			CHECK_ERR( _planet.emissionMap );
		}

		// create uniform buffer
		if ( not _planet.ubuffer )
		{
			_planet.ubuffer = _frameGraph->CreateBuffer( BufferDesc{ SizeOf<PlanetData>, EBufferUsage::Uniform | EBufferUsage::TransferDst },
														 Default, "Planet.UB" );
			CHECK_ERR( _planet.ubuffer );
		}

		// create pipeline
		{
			const String			shader = _LoadShader( "shaders/planet.glsl" );
			GraphicsPipelineDesc	ppln;

			ppln.AddShader( EShader::Vertex,		 EShaderLangFormat::VKSL_110, "main", "#define SHADER SH_VERTEX\n#define USE_QUADS 1\n"s + shader );
			ppln.AddShader( EShader::TessControl,	 EShaderLangFormat::VKSL_110, "main", "#define SHADER SH_TESS_CONTROL\n#define USE_QUADS 1\n"s + shader );
			ppln.AddShader( EShader::TessEvaluation, EShaderLangFormat::VKSL_110, "main", "#define SHADER SH_TESS_EVALUATION\n#define USE_QUADS 1\n"s + shader );
			ppln.AddShader( EShader::Fragment,		 EShaderLangFormat::VKSL_110, "main", "#define SHADER SH_FRAGMENT\n#define USE_QUADS 1\n"s + shader );

			GPipelineID	id = _frameGraph->CreatePipeline( ppln );
			if ( id )
			{
				_frameGraph->ReleaseResource( _planet.pipeline );
				_planet.pipeline = std::move(id);
			}
		}

		// setup descriptor set
		if ( _planet.pipeline )
		{
			CHECK_ERR( _frameGraph->InitPipelineResources( _planet.pipeline, DescriptorSetID{"0"}, OUT _planet.resources ));

			_planet.resources.BindBuffer(  UniformID{"un_PlanetData"},  _planet.ubuffer );
			_planet.resources.BindTexture( UniformID{"un_HeightMap"},   _planet.heightMap,   _linearSampler );
			_planet.resources.BindTexture( UniformID{"un_NormalMap"},   _planet.normalMap,   _linearSampler );
			_planet.resources.BindTexture( UniformID{"un_AlbedoMap"},   _planet.albedoMap,   _linearSampler );
			_planet.resources.BindTexture( UniformID{"un_EmissionMap"}, _planet.emissionMap, _linearSampler );
		}

		return true;
	}
	
/*
=================================================
	_GenerateHeightMap
=================================================
*/
	bool  GenPlanetApp::_GenerateHeightMap (const CommandBuffer &cmdbuf)
	{
		ComputePipelineDesc	desc;
		desc.AddShader( EShaderLangFormat::VKSL_110,
						"main",
						ShaderProjection + _LoadShader( "shaders/gen_height.glsl" ));

		CPipelineID		gen_height_ppln = _frameGraph->CreatePipeline( desc );
		if ( not gen_height_ppln )
			return false;

		PipelineResources	ppln_res;
		CHECK_ERR( _frameGraph->InitPipelineResources( gen_height_ppln, DescriptorSetID{"0"}, OUT ppln_res ));

		const uint2		local_size	{8,8};
		const uint2		face_size	= _frameGraph->GetDescription( _planet.heightMap ).dimension.xy();
		const uint2		group_count	= (face_size + local_size - 3) / (local_size - 2);

		for (uint face = 0; face < 6; ++face)
		{
			ppln_res.BindImage( UniformID{"un_OutHeight"}, _planet.heightMap, ImageViewDesc{}.SetArrayLayers( face, 1 ));
			ppln_res.BindImage( UniformID{"un_OutNormal"}, _planet.normalMap, ImageViewDesc{}.SetArrayLayers( face, 1 ));

			DispatchCompute	comp;
			const uint3		pc_data{ face_size.x, face_size.y, face };

			comp.AddPushConstant( PushConstantID{"PushConst"}, pc_data );
			comp.AddResources( DescriptorSetID{"0"}, &ppln_res );
			comp.SetLocalSize( local_size );
			comp.Dispatch( group_count );
			comp.SetPipeline( gen_height_ppln );

			cmdbuf->AddTask( comp );
		}

		_frameGraph->ReleaseResource( gen_height_ppln );
		return true;
	}
	
/*
=================================================
	_GenerateColorMap
=================================================
*/
	bool  GenPlanetApp::_GenerateColorMap (const CommandBuffer &cmdbuf)
	{
		ComputePipelineDesc	desc;
		desc.AddShader( EShaderLangFormat::VKSL_110,
						"main",
						ShaderProjection + _LoadShader( "shaders/gen_color.glsl" ));

		CPipelineID		gen_color_ppln = _frameGraph->CreatePipeline( desc );
		if ( not gen_color_ppln )
			return false;

		PipelineResources	ppln_res;
		CHECK_ERR( _frameGraph->InitPipelineResources( gen_color_ppln, DescriptorSetID{"0"}, OUT ppln_res ));

		const uint2		local_size	{8,8};
		const uint2		face_size	= _frameGraph->GetDescription( _planet.heightMap ).dimension.xy();
		const uint2		group_count	= IntCeil( face_size, local_size );

		for (uint face = 0; face < 6; ++face)
		{
			ppln_res.BindImage( UniformID{"un_HeightMap"},   _planet.heightMap, ImageViewDesc{}.SetArrayLayers( face, 1 ));
			ppln_res.BindImage( UniformID{"un_NormalMap"},   _planet.normalMap, ImageViewDesc{}.SetArrayLayers( face, 1 ));
			ppln_res.BindImage( UniformID{"un_OutAlbedo"},   _planet.albedoMap, ImageViewDesc{}.SetArrayLayers( face, 1 ));
			ppln_res.BindImage( UniformID{"un_OutEmission"}, _planet.emissionMap, ImageViewDesc{}.SetArrayLayers( face, 1 ));

			DispatchCompute	comp;
			const uint3		pc_data{ face_size.x, face_size.y, face };

			comp.AddPushConstant( PushConstantID{"PushConst"}, pc_data );
			comp.AddResources( DescriptorSetID{"0"}, &ppln_res );
			comp.SetLocalSize( local_size );
			comp.Dispatch( group_count );
			comp.SetPipeline( gen_color_ppln );

			cmdbuf->AddTask( comp );
		}

		_frameGraph->ReleaseResource( gen_color_ppln );
		return true;
	}

/*
=================================================
	_LoadShader
=================================================
*/
	String  GenPlanetApp::_LoadShader (StringView filename)
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
	bool  GenPlanetApp::DrawScene ()
	{
		PlanetData		planet_data;

		// update
		{
			_UpdateCamera();

			planet_data.viewProj		= GetCamera().ToViewProjMatrix();
			planet_data.position		= vec4{ GetCamera().transform.position, 0.0f };
			planet_data.clipPlanes		= GetViewRange();
			planet_data.tessLevel		= TessLevel;
			planet_data.radius			= 10.0f;
			planet_data.lightDirection	= glm::inverse( GetCamera().transform.orientation ) * normalize(vec3( 0.0f, 0.0f, -1.0f ));
		}

		// generate
		CommandBuffer	gen_cmdbuf;

		if ( _recreatePlanet )
		{
			FG_LOGI( "\n========================= Reload shaders =========================\n" );
			_recreatePlanet = false;

			gen_cmdbuf = _frameGraph->Begin( CommandBufferDesc{ EQueueType::Graphics });

			CHECK_ERR( _CreatePlanet( gen_cmdbuf ));
			CHECK( _GenerateHeightMap( gen_cmdbuf ));
			CHECK( _GenerateColorMap( gen_cmdbuf ));
			CHECK_ERR( _frameGraph->Execute( gen_cmdbuf ));
		}

		// draw
		{
			CommandBuffer	cmdbuf		= _frameGraph->Begin( CommandBufferDesc{ EQueueType::Graphics });
			const uint2		sw_dim		= GetSurfaceSize();
			const uint2		surf_dim	= uint2(float2(sw_dim) * _surfaceScale);
			LogicalPassID	pass_id;
			
			// resize
			if ( not _colorBuffer or Any( _frameGraph->GetDescription(_colorBuffer).dimension.xy() != surf_dim ))
			{
				_frameGraph->WaitIdle();
				_frameGraph->ReleaseResource( _colorBuffer );
				_frameGraph->ReleaseResource( _depthBuffer );

				_colorBuffer = _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{surf_dim}, EPixelFormat::RGBA8_UNorm,
																	EImageUsage::ColorAttachment | EImageUsage::Sampled | EImageUsage::Transfer | EImageUsage::Storage },
														 Default, "ColorBuffer" );
				_depthBuffer = _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{surf_dim}, EPixelFormat::Depth24_Stencil8,
																	EImageUsage::DepthStencilAttachment | EImageUsage::Sampled },
														 Default, "DepthBuffer" );
				CHECK( _colorBuffer and _depthBuffer );
			}

			if ( gen_cmdbuf )
				cmdbuf->AddDependency( gen_cmdbuf );

			if ( _showTimemap )
				cmdbuf->BeginShaderTimeMap( surf_dim, EShaderStages::Fragment );

			// planet rendering pass
			if ( _planet.pipeline )
			{
				pass_id = cmdbuf->CreateRenderPass( RenderPassDesc{ surf_dim }
									.AddViewport( surf_dim )
									.AddTarget( RenderTargetID::Color_0, _colorBuffer, RGBA32f{0.0f}, EAttachmentStoreOp::Store )
									.AddTarget( RenderTargetID::Depth, _depthBuffer, DepthStencil{1.0f}, EAttachmentStoreOp::Store )
									.SetDepthCompareOp( ECompareOp::LEqual ).SetDepthTestEnabled( true ).SetDepthWriteEnabled( true )
									//.SetPolygonMode( EPolygonMode::Line )
								);
				CHECK_ERR( pass_id );

				cmdbuf->AddTask( pass_id, _planet.cube.Draw( Lod ).SetPipeline( _planet.pipeline )
									.AddResources( DescriptorSetID{"0"}, &_planet.resources ));

				cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _planet.ubuffer ).AddData( &planet_data, 1, 0_b ));
				cmdbuf->AddTask( SubmitRenderPass{ pass_id });
			}
		
			if ( _showTimemap )
				cmdbuf->EndShaderTimeMap( _colorBuffer );

			// present
			{
				RawImageID	sw_image = cmdbuf->GetSwapchainImage( GetSwapchain() );

				cmdbuf->AddTask( BlitImage{}.From( _colorBuffer ).To( sw_image ).SetFilter( EFilter::Linear )
											.AddRegion( {}, int2(0), int2(surf_dim),
													    {}, int2(0), int2(sw_dim) ));
			}

			CHECK_ERR( _frameGraph->Execute( cmdbuf ));

			_SetLastCommandBuffer( cmdbuf );
		}
		return true;
	}
	
/*
=================================================
	OnKey
=================================================
*/
	void  GenPlanetApp::OnKey (StringView key, EKeyAction action)
	{
		BaseSceneApp::OnKey( key, action );

		if ( action == EKeyAction::Down )
		{
			//if ( key == "right mb" )	_AddDecal( GetMousePos(), 0.1f );

			if ( key == "R" )	_recreatePlanet = true;
			if ( key == "U" )	_debugPixel = GetMousePos() / vec2(GetSurfaceSize().x, GetSurfaceSize().y);
			if ( key == "T" )	_showTimemap = not _showTimemap;
		}
	}

}	// FG


// unit tests
extern void UnitTest_SphericalCubeMath ();


/*
=================================================
	main
=================================================
*/
int main ()
{
	using namespace FG;

	UnitTest_SphericalCubeMath();

	auto	app = MakeShared<GenPlanetApp>();

	CHECK_ERR( app->Initialize(), -1 );

	for (; app->Update();) {}
	
	return 0;
}
