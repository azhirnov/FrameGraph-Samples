// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "FGShadertoyApp.h"
#include "scene/Loader/DevIL/DevILLoader.h"
#include "scene/Loader/Intermediate/IntermImage.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Stream/FileStream.h"
#include <thread>

#ifdef FG_STD_FILESYSTEM
#include <filesystem>
namespace FS = std::filesystem;
#endif

namespace FG
{
	
/*
=================================================
	constructor
=================================================
*/
	FGShadertoyApp::Shader::Shader (StringView name, ShaderDescr &&desc) :
		_name{ name },
		_pplnFilename{ std::move(desc._pplnFilename) },	_pplnDefines{ std::move(desc._pplnDefines) },
		_channels{ desc._channels },					_surfaceScale{ desc._surfaceScale },
		_surfaceSize{ desc._surfaceSize },				_format{ desc._format }
	{}
//-----------------------------------------------------------------------------


/*
=================================================
	constructor
=================================================
*/
	FGShadertoyApp::FGShadertoyApp () :
		_passIdx{0}
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	FGShadertoyApp::~FGShadertoyApp ()
	{
	}

/*
=================================================
	_InitSamples
=================================================
*/
	void FGShadertoyApp::_InitSamples ()
	{
		const auto	SinglePass = [this] (String&& fname) { ShaderDescr sh_main;  sh_main.Pipeline( std::move(fname) );  CHECK( _AddShader( "main", std::move(sh_main) )); };
		
		_samples.push_back( [SinglePass] ()  { SinglePass("st_shaders/Skyline2.glsl"); });
		_samples.push_back( [SinglePass] ()  { SinglePass("st_shaders/Skyline.glsl"); });
		_samples.push_back( [SinglePass] ()  { SinglePass("my_shaders/ConvexShape2D.glsl"); });
		_samples.push_back( [SinglePass] ()  { SinglePass("my_shaders/ConvexShape3D.glsl"); });
		_samples.push_back( [SinglePass] ()  { SinglePass("my_shaders/Building_1.glsl"); });
		_samples.push_back( [SinglePass] ()  { SinglePass("my_shaders/Building_2.glsl"); });

		_samples.push_back( [this] ()
		{
			const uint2	map_size{ 128 };
			const uint	radius	= 32;

			ShaderDescr sh_bufA;
			sh_bufA.Pipeline( "my_shaders/OptSDF_1.glsl" );
			sh_bufA.SetFormat( EPixelFormat::R8_UNorm );
			sh_bufA.SetDimension( map_size );
			CHECK( _AddShader( "bufA", std::move(sh_bufA) ));

			ShaderDescr sh_bufB;
			sh_bufB.Pipeline( "my_shaders/OptSDF_2.glsl", "#define RADIUS "s << ToString(radius) );
			sh_bufB.InChannel( "bufA", 0 );
			sh_bufB.SetFormat( EPixelFormat::RG32F );
			sh_bufB.SetDimension( map_size );
			CHECK( _AddShader( "bufB", std::move(sh_bufB) ));
			
			ShaderDescr sh_main;
			sh_main.Pipeline( "my_shaders/OptSDF_3.glsl", "#define RADIUS "s << ToString(radius) );
			sh_main.InChannel( "bufA", 0 );
			sh_main.InChannel( "bufB", 1 );
			CHECK( _AddShader( "main", std::move(sh_main) ));
		});

		_samples.push_back( [this] ()
		{
			ShaderDescr sh_main;
			sh_main.Pipeline( "st_shaders/Glowballs.glsl" );
			sh_main.InChannel( "main", 0 );
			CHECK( _AddShader( "main", std::move(sh_main) ));
		});
	}
	
/*
=================================================
	OnKey
=================================================
*/
	void FGShadertoyApp::OnKey (StringView key, EKeyAction action)
	{
		BaseSceneApp::OnKey( key, action );

		if ( action == EKeyAction::Down )
		{
			if ( key == "[" )		--_nextSample;		else
			if ( key == "]" )		++_nextSample;

			if ( key == "R" )		_Recompile();
			if ( key == "T" )		_frameCounter = 0;
			if ( key == "U" )		_debugPixel = GetMousePos() / vec2{GetSurfaceSize().x, GetSurfaceSize().y};

			if ( key == "F" )		_freeze = not _freeze;
			if ( key == "space" )	_pause = not _pause;

			if ( key == "M" )		_vrMirror = not _vrMirror;
		}
	}

/*
=================================================
	Initialize
=================================================
*/
	bool FGShadertoyApp::Initialize ()
	{
		{
			AppConfig	cfg;
			cfg.surfaceSize			= uint2(1024, 768);
			cfg.windowTitle			= "Shadertoy";
			cfg.shaderDirectories	= { FG_DATA_PATH "../shaderlib" };
			cfg.dbgOutputPath		= FG_DATA_PATH "_debug_output";
			cfg.enableDebugLayers	= false;

			CHECK_ERR( _CreateFrameGraph( cfg ));
		}

		_CreateSamplers();
		_InitSamples();
		
		GetFPSCamera().SetPosition({ 0.0f, 0.0f, 2.0f });
		GetVRCamera().SetHmdOffsetScale( 0.1f );
		return true;
	}
		
/*
=================================================
	DrawScene
=================================================
*/
	bool FGShadertoyApp::DrawScene ()
	{
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

			_ResetShaders();
			_samples[_currSample]();
		}

		_cmdBuffer = _frameGraph->Begin( CommandBufferDesc{ EQueueType::Graphics });

		uint2	required_size;
		bool	enable_vr;

		if ( GetVRDevice() and GetVRDevice()->GetHmdStatus() == IVRDevice::EHmdStatus::Mounted ) {
			enable_vr		= true;
			required_size	= GetVRDevice()->GetRenderTargetDimension();
		} else {
			enable_vr		= false;
			required_size	= GetSurfaceSize();
		}

		if ( Any(_surfaceSize != required_size) or (enable_vr != _vrMode) )
		{
			CHECK_ERR( _RecreateShaders( required_size, enable_vr ));

			GetFPSCamera().SetPerspective( GetCameraFov(), float(_surfaceSize.x) / _surfaceSize.y, 0.1f, 100.0f );
		}
		
		if ( _ordered.size() )
		{
			_UpdateShaderData();
		
			const uint	pass_idx = _passIdx;

			// run shaders
			for (uint eye = 0; eye < (1 + uint(_vrMode)); ++eye)
			for (size_t i = 0; i < _ordered.size(); ++i)
			{
				_DrawWithShader( _ordered[i], uint(eye), pass_idx, i+1 == _ordered.size() );
			}
		
			// present
			{
				ShadersMap_t::iterator	iter = _shaders.find( "main" );
				CHECK_ERR( iter != _shaders.end() );

				const auto&	image	= iter->second->_perEye[0].passes[pass_idx].renderTarget;
				const auto&	desc	= _frameGraph->GetDescription( image );
				const uint2	point	= { uint((desc.dimension.x * GetMousePos().x) / GetSurfaceSize().x + 0.5f),
										uint((desc.dimension.y * GetMousePos().y) / GetSurfaceSize().y + 0.5f) };

				if ( point.x < desc.dimension.x and point.y < desc.dimension.y )
				{
					_cmdBuffer->AddTask( ReadImage{}.SetImage( image, point, uint2{1,1} )
														.SetCallback( [this, point] (const ImageView &view) { _OnPixelReadn( point, view ); })
														.DependsOn( _currTask ));
				}

				if ( not _vrMode or _vrMirror )
				{
					_currTask = _cmdBuffer->AddTask( Present{ GetSwapchain(), image }.DependsOn( _currTask ));
				}
				
				CHECK_ERR( _frameGraph->Execute( _cmdBuffer ));
				CHECK_ERR( _frameGraph->Flush() );

				if ( _vrMode )
				{
					RawImageID			src0		= iter->second->_perEye[0].passes[pass_idx].renderTarget;
					RawImageID			src1		= iter->second->_perEye[1].passes[pass_idx].renderTarget;
					const auto&			queue		= GetVulkan().GetVkQueues()[0];
					const auto&			vk_desc0	= std::get<VulkanImageDesc>( _frameGraph->GetApiSpecificDescription( src0 ));
					const auto&			vk_desc1	= std::get<VulkanImageDesc>( _frameGraph->GetApiSpecificDescription( src1 ));
					IVRDevice::VRImage	vr_img;

					vr_img.currQueue		= queue.handle;
					vr_img.queueFamilyIndex	= queue.familyIndex;
					vr_img.dimension		= vk_desc0.dimension.xy();
					vr_img.bounds			= RectF{ 0.0f, 0.0f, 1.0f, 1.0f };
					vr_img.format			= BitCast<VkFormat>(vk_desc0.format);
					vr_img.sampleCount		= vk_desc0.samples;

					vr_img.handle = BitCast<VkImage>(vk_desc0.image);
					GetVRDevice()->Submit( vr_img, IVRDevice::Eye::Left );
					
					vr_img.handle = BitCast<VkImage>(vk_desc1.image);
					GetVRDevice()->Submit( vr_img, IVRDevice::Eye::Right );
				}
			}
		}

		_SetLastCommandBuffer( _cmdBuffer );
	
		_cmdBuffer = null;
		_currTask = null;
		++_passIdx;

		return true;
	}
	
/*
=================================================
	OnUpdateFrameStat
=================================================
*/
	void FGShadertoyApp::OnUpdateFrameStat (OUT String &str) const
	{
		str << ", Pixel: " << ToString( _selectedPixel );
	}

/*
=================================================
	_UpdateShaderData
=================================================
*/
	void FGShadertoyApp::_UpdateShaderData ()
	{
		const auto	time		= TimePoint_t::clock::now();
		const auto	velocity	= 1.0f;

		if ( _frameCounter == 0 )
			_startTime = time;

		const float	app_dt		= std::chrono::duration_cast<SecondsF>( (_freeze ? _lastUpdateTime : time) - _startTime ).count();
		const float	frame_dt	= std::chrono::duration_cast<SecondsF>( time - _lastUpdateTime ).count();
		
		_UpdateCamera();

		_ubData.iTime				= app_dt;
		_ubData.iTimeDelta			= frame_dt;
		_ubData.iFrame				= int(_frameCounter);
		_ubData.iMouse				= vec4( IsMousePressed() ? GetMousePos() : vec2{}, vec2{} );	// TODO: click
		//_ubData.iDate				= float4(uint3( date.Year(), date.Month(), date.DayOfMonth()).To<float3>(), float(date.Second()) + float(date.Milliseconds()) * 0.001f);
		_ubData.iSampleRate			= 0.0f;	// not supported yet

		if ( not _freeze ) {
			_lastUpdateTime	= time;
			++_frameCounter;
		}
	}

/*
=================================================
	_DrawWithShader
=================================================
*/
	bool FGShadertoyApp::_DrawWithShader (const ShaderPtr &shader, uint eye, uint passIndex, bool isLast)
	{
		auto&	eye_data	= shader->_perEye[eye];
		auto&	pass		= eye_data.passes[passIndex];
		auto	view_size	= _frameGraph->GetDescription( pass.renderTarget ).dimension.xy();

		// update uniform buffer
		{
			if ( _vrMode )
			{
				GetVRCamera().GetFrustum()[eye]->GetRays( OUT _ubData.iCameraFrustumRayLB, OUT _ubData.iCameraFrustumRayLT,
														  OUT _ubData.iCameraFrustumRayRB, OUT _ubData.iCameraFrustumRayRT );
				_ubData.iCameraPos	= GetVRCamera().Position();
			}
			else
			{
				GetFrustum().GetRays( OUT _ubData.iCameraFrustumRayLB, OUT _ubData.iCameraFrustumRayLT,
									  OUT _ubData.iCameraFrustumRayRB, OUT _ubData.iCameraFrustumRayRT );
				_ubData.iCameraPos	= GetCamera().transform.position;
			}


			for (size_t i = 0; i < pass.images.size(); ++i)
			{
				if ( pass.resources.HasTexture(UniformID{ "iChannel"s << ToString(i) }) )
				{
					auto	dim	= _frameGraph->GetDescription( pass.images[i] ).dimension;

					_ubData.iChannelResolution[i]	= vec4{ float(dim.x), float(dim.y), 0.0f, 0.0f };
					_ubData.iChannelTime[i]			= vec4{ _ubData.iTime };
				}
			}
		
			_ubData.iResolution = vec3{ float(view_size.x), float(view_size.y), 0.0f };
			_ubData.iEyeIndex	= eye;

			_currTask = _cmdBuffer->AddTask( UpdateBuffer{}.SetBuffer( eye_data.ubuffer ).AddData( &_ubData, 1 ).DependsOn( _currTask ));
		}


		LogicalPassID	pass_id = _cmdBuffer->CreateRenderPass( RenderPassDesc( view_size )
										.AddTarget( RenderTargetID::Color_0, pass.renderTarget, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store )
										.AddViewport( view_size ));

		DrawVertices	draw_task;
		draw_task.SetPipeline( _vrMode ? shader->_pipelineVR : shader->_pipeline );
		draw_task.AddResources( DescriptorSetID{"0"}, &pass.resources );
		draw_task.Draw( 4 ).SetTopology( EPrimitive::TriangleStrip );

		if ( isLast and _debugPixel.has_value() )
		{
			const vec2	coord = vec2{pass.viewport.x, pass.viewport.y} * (*_debugPixel) + 0.5f;

			draw_task.EnableFragmentDebugTrace( int(coord.x), int(coord.y) );
			_debugPixel.reset();
		}

		_cmdBuffer->AddTask( pass_id, draw_task );

		_currTask = _cmdBuffer->AddTask( SubmitRenderPass{ pass_id }.DependsOn( _currTask ));
		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void FGShadertoyApp::Destroy ()
	{
		if ( _frameGraph )
		{
			_frameGraph->WaitIdle();

			_ResetShaders();
			_samples.clear();

			for (auto& img : _imageCache) {
				_frameGraph->ReleaseResource( INOUT img.second );
			}
			_imageCache.clear();
			
			_frameGraph->ReleaseResource( INOUT _nearestClampSampler );
			_frameGraph->ReleaseResource( INOUT _linearClampSampler );
			_frameGraph->ReleaseResource( INOUT _nearestRepeatSampler );
			_frameGraph->ReleaseResource( INOUT _linearRepeatSampler );
		}

		_DestroyFrameGraph();
	}
	
/*
=================================================
	_RecreateShaders
=================================================
*/
	bool FGShadertoyApp::_RecreateShaders (const uint2 &newSize, bool enableVR)
	{
		CHECK_ERR( not _shaders.empty() );

		_surfaceSize		= newSize;
		_scaledSurfaceSize	= uint2( float2(newSize) * _sufaceScale + 0.5f );
		
		Array<ShaderPtr>	sorted;

		// destroy all
		for (auto& sh : _shaders)
		{
			sorted.push_back( sh.second );
			_DestroyShader( sh.second, false );
		}
		_ordered.clear();

		// create all
		for (uint i = 0; not sorted.empty() and i < 1000; ++i)
		{
			for (auto iter = sorted.begin(); iter != sorted.end();)
			{
				if ( _CreateShader( *iter, enableVR ) )
				{
					_ordered.push_back( *iter );
					iter = sorted.erase( iter );
				}
				else
					++iter;
			}
		}

		CHECK_ERR( sorted.empty() );
		return true;
	}
	
/*
=================================================
	_CreateShader
=================================================
*/
	bool FGShadertoyApp::_CreateShader (const ShaderPtr &shader, bool enableVR)
	{
		// compile pipeline
		if ( not enableVR and not shader->_pipeline )
		{
			shader->_pipeline = _Compile( shader->_pplnFilename, shader->_pplnDefines );
			CHECK_ERR( shader->_pipeline );
		}

		if ( enableVR and not shader->_pipelineVR )
		{
			shader->_pipelineVR = _Compile( shader->_pplnFilename, shader->_pplnDefines + "\n#define VR_MODE\n" );
			CHECK_ERR( shader->_pipelineVR );
		}

		// check dependencies
		for (auto& ch : shader->_channels)
		{
			// find channel in shader passes
			ShadersMap_t::iterator	iter = _shaders.find( ch.name );
			if ( iter != _shaders.end() )
			{
				if ( iter->second == shader )
					continue;

				if ( iter->second->_perEye.empty() )
					return false;

				for (auto& eye_data : iter->second->_perEye)
				for (auto& pass : eye_data.passes)
				{
					// wait until all dependencies will be initialized
					if ( not pass.renderTarget.IsValid() )
						return false;
				}
				continue;
			}

			// find channel in loadable images
			if ( _HasImage( ch.name ) )
				continue;

			RETURN_ERR( "unknown channel type, it is not a shader pass and not a file" );
		}
		
		shader->_perEye.resize( enableVR ? 2 : 1 );

		// create render targets
		for (auto& eye_data : shader->_perEye)
		for (auto& pass : eye_data.passes)
		{
			if ( shader->_surfaceSize.has_value() )
				pass.viewport = shader->_surfaceSize.value();
			else
				pass.viewport = uint2( float2(_scaledSurfaceSize) * shader->_surfaceScale.value_or(1.0f) + 0.5f );

			ImageDesc		desc;
			desc.imageType	= EImage::Tex2D;
			desc.dimension	= uint3{ pass.viewport, 1 };
			desc.format		= shader->_format.value_or( ImageFormat );
			desc.usage		= EImageUsage::TransferSrc | EImageUsage::Sampled | EImageUsage::ColorAttachment;

			EResourceState	def_state	= enableVR ? EResourceState::TransferSrc : EResourceState::Unknown;
			const String	name		= String(shader->Name()) << "-RT-" << ToString(Distance( eye_data.passes.data(), &pass ))
											<< (enableVR ? (shader->_perEye.data() == &eye_data ? "-left" : "-right") : "");

			pass.renderTarget = _frameGraph->CreateImage( desc, Default, def_state, name );
			CHECK_ERR( pass.renderTarget );
		}
		
		// create uniform buffers
		for (auto& eye_data : shader->_perEye)
		{
			BufferDesc	desc;
			desc.size	= SizeOf<ShadertoyUB>;
			desc.usage	= EBufferUsage::Uniform | EBufferUsage::TransferDst;

			eye_data.ubuffer = _frameGraph->CreateBuffer( desc, Default, "ShadertoyUB" );
			CHECK_ERR( eye_data.ubuffer );
		}
		
		// setup pipeline resource table
		for (size_t eye = 0; eye < shader->_perEye.size(); ++eye)
		for (size_t i = 0; i < shader->_perEye[eye].passes.size(); ++i)
		{
			auto&	pass = shader->_perEye[eye].passes[i];
			
			CHECK( _frameGraph->InitPipelineResources( enableVR ? shader->_pipelineVR : shader->_pipeline, DescriptorSetID{"0"}, OUT pass.resources ));

			pass.resources.BindBuffer( UniformID{"ShadertoyUB"}, shader->_perEye[eye].ubuffer );
			pass.images.resize( shader->_channels.size() );

			for (size_t j = 0; j < shader->_channels.size(); ++j)
			{
				auto&		ch		= shader->_channels[j];
				auto&		image	= pass.images[j];
				UniformID	name	{"iChannel"s << ToString(j)};

				// find channel in shader passes
				ShadersMap_t::iterator	iter = _shaders.find( ch.name );
				if ( iter != _shaders.end() )
				{
					if ( iter->second == shader )
						// use image from previous pass
						image = ImageID{ shader->_perEye[eye].passes[(i-1) & 1].renderTarget.Get() };
					else
						image = ImageID{ iter->second->_perEye[eye].passes[i].renderTarget.Get() };
					
					pass.resources.BindTexture( name, image, _linearClampSampler );	// TODO: sampler
					continue;
				}
				
				// find channel in loadable images
				if ( _LoadImage( ch.name, OUT image ) )
				{
					pass.resources.BindTexture( name, image, _linearClampSampler );
					continue;
				}
				
				RETURN_ERR( "unknown channel type, it is not a shader pass and not a file" );
			}
		}

		_vrMode = enableVR;
		return true;
	}
	
/*
=================================================
	_DestroyShader
=================================================
*/
	void FGShadertoyApp::_DestroyShader (const ShaderPtr &shader, bool destroyPipeline)
	{
		_frameGraph->WaitIdle();
		
		if ( destroyPipeline ) {
			_frameGraph->ReleaseResource( INOUT shader->_pipeline );
			_frameGraph->ReleaseResource( INOUT shader->_pipelineVR );
		}

		for (auto& eye_data : shader->_perEye)
		{
			for (auto& pass : eye_data.passes)
			{
				_frameGraph->ReleaseResource( INOUT pass.renderTarget );
			
				for (auto& img : pass.images)
				{
					if ( img.IsValid() )
						FG_UNUSED( img.Release() );
				}
			}
			_frameGraph->ReleaseResource( INOUT eye_data.ubuffer );
		}
		shader->_perEye.clear();
	}

/*
=================================================
	_Compile
=================================================
*/
	GPipelineID  FGShadertoyApp::_Compile (StringView name, StringView defs) const
	{
		const char	vs_source[] = R"#(
			const vec2	g_Positions[] = {
				{ -1.0f,  1.0f },  { -1.0f, -1.0f },
				{  1.0f,  1.0f },  {  1.0f, -1.0f }
			};

			void main() {
				gl_Position	= vec4( g_Positions[gl_VertexIndex], 0.0f, 1.0f );
			}
		)#";

		const char	fs_source[] = R"#(
			#extension GL_GOOGLE_include_directive : require

			layout(binding=0, std140) uniform  ShadertoyUB
			{
				vec3	iResolution;			// viewport resolution (in pixels)
				float	iTime;					// shader playback time (in seconds)
				float	iTimeDelta;				// render time (in seconds)
				int		iFrame;					// shader playback frame
				float	iChannelTime[4];		// channel playback time (in seconds)
				vec3	iChannelResolution[4];	// channel resolution (in pixels)
				vec4	iMouse;					// mouse pixel coords. xy: current (if MLB down), zw: click
				vec4	iDate;					// (year, month, day, time in seconds)
				float	iSampleRate;			// sound sample rate (i.e., 44100)
				vec3	iCameraFrustumLB;		// frustum rays (left bottom, right bottom, left top, right top)
				vec3	iCameraFrustumRB;
				vec3	iCameraFrustumLT;
				vec3	iCameraFrustumRT;
				vec3	iCameraPos;				// camera position in world space
				int		iEyeIndex;
			};

			layout(location=0) out vec4	out_Color;

			#ifdef VR_MODE
				void mainVR (out vec4 fragColor, in vec2 fragCoord, in vec3 fragRayOri, in vec3 fragRayDir);

				void main ()
				{
					vec2 uv    = gl_FragCoord.xy / iResolution.xy;
					vec3 dir   = mix( mix( iCameraFrustumLB, iCameraFrustumRB, uv.x ),
									  mix( iCameraFrustumLT, iCameraFrustumRT, uv.x ),
									  uv.y );
					mainVR( out_Color, gl_FragCoord.xy, iCameraPos, dir );
				}
			#else
				void mainImage (out vec4 fragColor, in vec2 fragCoord);

				void main ()
				{
					vec2 coord = gl_FragCoord.xy;
					coord.y = iResolution.y - coord.y - 1.0;

					mainImage( out_Color, coord );
				}
			#endif
		)#";

		String	src0, src1;
		{
			FileRStream		file{ String{FG_DATA_PATH} << name };
			CHECK_ERR( file.IsOpen() );
			CHECK_ERR( file.Read( size_t(file.Size()), OUT src1 ));
		}

		if ( not defs.empty() )
			src0 << defs << '\n';

		if ( HasSubString( src1, "iChannel0" ) )
			src0 << "layout(binding=1) uniform sampler2D  iChannel0;\n";

		if ( HasSubString( src1, "iChannel1" ) )
			src0 << "layout(binding=2) uniform sampler2D  iChannel1;\n";

		if ( HasSubString( src1, "iChannel2" ) )
			src0 << "layout(binding=3) uniform sampler2D  iChannel2;\n";

		src0 << fs_source;
		src0 << src1;

		GraphicsPipelineDesc	desc;
		desc.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_110, "main", vs_source );
		desc.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_110 | EShaderLangFormat::EnableDebugTrace, "main", std::move(src0), name );

		return _frameGraph->CreatePipeline( desc, name );
	}
	
/*
=================================================
	_Recompile
=================================================
*/
	bool FGShadertoyApp::_Recompile ()
	{
		for (auto& shader : _ordered)
		{
			if ( _vrMode )
			{
				if ( GPipelineID ppln = _Compile( shader->_pplnFilename, shader->_pplnDefines + "\n#define VR_MODE\n" ))
				{
					_frameGraph->ReleaseResource( INOUT shader->_pipelineVR );
					shader->_pipelineVR = std::move(ppln);
				}
			}
			else
			{
				if ( GPipelineID ppln = _Compile( shader->_pplnFilename, shader->_pplnDefines ))
				{
					_frameGraph->ReleaseResource( INOUT shader->_pipeline );
					shader->_pipeline = std::move(ppln);
				}
			}
		}
		return true;
	}

/*
=================================================
	_AddShader
=================================================
*/
	bool FGShadertoyApp::_AddShader (const String &name, ShaderDescr &&desc)
	{
		_shaders.insert_or_assign( name, MakeShared<Shader>( name, std::move(desc) ));
		return true;
	}
	
/*
=================================================
	_ResetShaders
=================================================
*/
	void FGShadertoyApp::_ResetShaders ()
	{
		for (auto& sh : _shaders) {
			_DestroyShader( sh.second, true );
		}
		_shaders.clear();
		_ordered.clear();

		_surfaceSize	= Default;
		_frameCounter	= 0;
	}
	
/*
=================================================
	_CreateSamplers
=================================================
*/
	void FGShadertoyApp::_CreateSamplers ()
	{
		SamplerDesc		desc;
		desc.SetAddressMode( EAddressMode::ClampToEdge );
		desc.SetFilter( EFilter::Nearest, EFilter::Nearest, EMipmapFilter::Nearest );
		_nearestClampSampler = _frameGraph->CreateSampler( desc, "NearestClamp" );
		
		desc.SetAddressMode( EAddressMode::ClampToEdge );
		desc.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear );
		_linearClampSampler = _frameGraph->CreateSampler( desc, "LinearClamp" );
		
		desc.SetAddressMode( EAddressMode::Repeat );
		desc.SetFilter( EFilter::Nearest, EFilter::Nearest, EMipmapFilter::Nearest );
		_nearestRepeatSampler = _frameGraph->CreateSampler( desc, "NearestRepeat" );
		
		desc.SetAddressMode( EAddressMode::Repeat );
		desc.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear );
		_linearRepeatSampler = _frameGraph->CreateSampler( desc, "LinearRepeat" );
	}
	
/*
=================================================
	_LoadImage
=================================================
*/
	bool FGShadertoyApp::_LoadImage (const String &filename, OUT ImageID &id)
	{
#	if defined(FG_ENABLE_DEVIL) and defined(FG_STD_FILESYSTEM)
		auto	iter = _imageCache.find( filename );
		
		if ( iter != _imageCache.end() )
		{
			id = ImageID{ iter->second.Get() };
			return true;
		}

		DevILLoader		loader;
		FS::path		fpath	= FS::path{FG_DATA_PATH}.append(filename);
		auto			image	= MakeShared<IntermImage>( fpath.string() );

		CHECK_ERR( loader.LoadImage( image, {}, null ));

		auto&	level	 = image->GetData()[0][0];
		String	img_name = fpath.filename().string();

		id = _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, level.dimension, level.format, EImageUsage::TransferDst | EImageUsage::Sampled },
											Default, img_name );
		CHECK_ERR( id );

		_currTask = _cmdBuffer->AddTask( UpdateImage{}.SetImage( id ).SetData( level.pixels, level.dimension, level.rowPitch, level.slicePitch ).DependsOn( _currTask ));

		_imageCache.insert_or_assign( filename, ImageID{id.Get()} );
		return true;
#	else
		return false;
#	endif
	}
	
/*
=================================================
	_HasImage
=================================================
*/
	bool FGShadertoyApp::_HasImage (StringView filename) const
	{
	#ifdef FG_STD_FILESYSTEM
		return FS::exists( FS::path{FG_DATA_PATH}.append(filename) );
	#else
		return true;
	#endif
	}
	
/*
=================================================
	_OnPixelReadn
=================================================
*/
	void FGShadertoyApp::_OnPixelReadn (const uint2 &, const ImageView &view)
	{
		view.Load( uint3{0,0,0}, OUT _selectedPixel );
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

	FGShadertoyApp	app;
	
	CHECK_FATAL( app.Initialize() );

	for (; app.Update(); ) {}

	app.Destroy();
	return 0;
}
