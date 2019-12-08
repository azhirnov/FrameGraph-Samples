// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ShaderView.h"

#include "scene/Loader/DevIL/DevILLoader.h"
#include "scene/Loader/Intermediate/IntermImage.h"

#include "stl/Algorithms/StringUtils.h"
#include "stl/Stream/FileStream.h"

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
	ShaderView::Shader::Shader (StringView name, ShaderDescr &&desc) :
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
	ShaderView::ShaderView (const FrameGraph &fg) :
		_passIdx{0}
	{
		_frameGraph	= fg;
		_CreateSamplers();
	}
	
/*
=================================================
	destructor
=================================================
*/
	ShaderView::~ShaderView ()
	{
		if ( _frameGraph )
		{
			_frameGraph->WaitIdle();

			ResetShaders();

			for (auto& img : _imageCache) {
				_frameGraph->ReleaseResource( INOUT img.second );
			}
			_imageCache.clear();
			
			_frameGraph->ReleaseResource( INOUT _nearestClampSampler );
			_frameGraph->ReleaseResource( INOUT _linearClampSampler );
			_frameGraph->ReleaseResource( INOUT _nearestRepeatSampler );
			_frameGraph->ReleaseResource( INOUT _linearRepeatSampler );
		}
	}
	
/*
=================================================
	SetCamera
=================================================
*/
	void  ShaderView::SetCamera (const FPSCamera &value)
	{
		_camera = value;
		_vrCamera.SetPosition( _camera.GetCamera().transform.position );
	}
	
	void  ShaderView::SetCamera (const VRCamera &value)
	{
		_vrCamera = value;
		_camera.SetPosition( _vrCamera.Position() );
		_camera.SetRotation( _vrCamera.Orientation() );
	}

	void  ShaderView::SetFov (Rad value)
	{
		_cameraFov = value;
		_camera.SetPerspective( _cameraFov, float(_viewSize.x) / _viewSize.y, 0.1f, 100.0f );
	}

/*
=================================================
	SetMode
=================================================
*/
	void  ShaderView::SetMode (const uint2 &viewSize, EViewMode mode)
	{
		CHECK_ERR( All( viewSize > uint2(0) ), void());
		
		if ( Any(_viewSize != viewSize) or (mode != _viewMode) )
		{
			_viewSize			= viewSize;
			_viewMode			= mode;
			_recreateShaders	= true;
		}
	}
	
/*
=================================================
	SetImageFormat
=================================================
*/
	void  ShaderView::SetImageFormat (EPixelFormat value, uint msaa)
	{
		if ( _imageFormat == value and _imageSamples == msaa )
			return;

		_imageFormat	 = value;
		_imageSamples	 = msaa;
		_recreateShaders = true;
	}

/*
=================================================
	SetMouse
=================================================
*/
	void  ShaderView::SetMouse (const vec2 &pos, bool pressed)
	{
		_lastMousePos	= pos;
		_mousePressed	= pressed;
	}

/*
=================================================
	Draw
=================================================
*/
	ShaderView::DrawResult_t  ShaderView::Draw (const CommandBuffer &cmdBuffer, uint frameId, SecondsF time, SecondsF dt)
	{
		CHECK_ERR( _frameGraph );
		CHECK_ERR( cmdBuffer );

		DrawResult_t	result;
		
		if ( _recreateShaders )
		{
			_recreateShaders = false;
			CHECK_ERR( _RecreateShaders( cmdBuffer ));
			
			_camera.SetPerspective( _cameraFov, float(_viewSize.x) / _viewSize.y, 0.1f, 100.0f );
		}

		if ( _ordered.size() )
		{
			// update shader data
			{
				_ubData.iTime			= time.count();
				_ubData.iTimeDelta		= dt.count();
				_ubData.iFrame			= int(frameId);
				_ubData.iMouse			= vec4( _mousePressed ? _lastMousePos : vec2{}, vec2{} );	// TODO: click
				//_ubData.iDate			= float4(uint3( date.Year(), date.Month(), date.DayOfMonth()).To<float3>(), float(date.Second()) + float(date.Milliseconds()) * 0.001f);
				_ubData.iSampleRate		= 0.0f;	// not supported yet
			}

			const uint	pass_idx = _passIdx;

			// run shaders
			for (uint eye = 0; eye < (1 + uint(_viewMode == EViewMode::HMD_VR)); ++eye)
			{
				for (size_t i = 0; i < _ordered.size(); ++i)
				{
					_DrawWithShader( cmdBuffer, _ordered[i], uint(eye), pass_idx, i+1 == _ordered.size() );
				}
			}
		
			// present
			{
				ShadersMap_t::iterator	iter = _shaders.find( "main" );
				CHECK_ERR( iter != _shaders.end() );

				if ( _viewMode == EViewMode::HMD_VR )
				{
					RawImageID	image_l	= iter->second->_perEye[0].passes[pass_idx].renderTarget;
					RawImageID	image_r	= iter->second->_perEye[1].passes[pass_idx].renderTarget;

					result = { _currTask, image_l, image_r };
				}
				else
				{
					const auto&	image	= iter->second->_perEye[0].passes[pass_idx].renderTarget;

					result = { _currTask, image, RawImageID{} };
				}
			}
		}
	
		_currTask = null;
		++_passIdx;

		return result;
	}

/*
=================================================
	_DrawWithShader
=================================================
*/
	bool  ShaderView::_DrawWithShader (const CommandBuffer &cmdBuffer, const ShaderPtr &shader, uint eye, uint passIndex, bool isLast)
	{
		auto&	eye_data	= shader->_perEye[eye];
		auto&	pass		= eye_data.passes[passIndex];
		auto	view_size	= _frameGraph->GetDescription( pass.renderTarget ).dimension.xy();

		// update uniform buffer
		{
			if ( _viewMode == EViewMode::HMD_VR )
			{
				_vrCamera.GetFrustum()[eye]->GetRays( OUT _ubData.iCameraFrustumRayLB, OUT _ubData.iCameraFrustumRayLT,
													  OUT _ubData.iCameraFrustumRayRB, OUT _ubData.iCameraFrustumRayRT );
				_ubData.iCameraPos	= _vrCamera.Position();
			}
			else
			{
				_camera.GetFrustum().GetRays( OUT _ubData.iCameraFrustumRayLB, OUT _ubData.iCameraFrustumRayLT,
									  OUT _ubData.iCameraFrustumRayRB, OUT _ubData.iCameraFrustumRayRT );
				_ubData.iCameraPos	= _camera.GetCamera().transform.position;
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

			_currTask = cmdBuffer->AddTask( UpdateBuffer{}.SetBuffer( eye_data.ubuffer ).AddData( &_ubData, 1 ).DependsOn( _currTask ));
		}


		LogicalPassID	pass_id = cmdBuffer->CreateRenderPass( RenderPassDesc( view_size )
										.AddTarget( RenderTargetID::Color_0, pass.renderTargetMS ? pass.renderTargetMS : pass.renderTarget, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store )
										.AddViewport( view_size ));

		DrawVertices	draw_task;
		draw_task.SetPipeline(
			_viewMode == EViewMode::Mono		? shader->_pipeline.mono.Get() :
			_viewMode == EViewMode::HMD_VR		? shader->_pipeline.hmdVR.Get() :
			_viewMode == EViewMode::VR180_Video	? shader->_pipeline.vr180.Get() :
			_viewMode == EViewMode::VR360_Video	? shader->_pipeline.vr360.Get() : Default );
		draw_task.AddResources( DescriptorSetID{"0"}, &pass.resources );
		draw_task.Draw( 4 ).SetTopology( EPrimitive::TriangleStrip );

		// TODO
		/*if ( isLast and _debugPixel.has_value() )
		{
			const vec2	coord = vec2{pass.viewport.x, pass.viewport.y} * (*_debugPixel) + 0.5f;

			draw_task.EnableFragmentDebugTrace( int(coord.x), int(coord.y) );
			_debugPixel.reset();
		}*/

		cmdBuffer->AddTask( pass_id, draw_task );

		_currTask = cmdBuffer->AddTask( SubmitRenderPass{ pass_id }.DependsOn( _currTask ));

		if ( pass.renderTargetMS )
		{
			_currTask = cmdBuffer->AddTask( ResolveImage{}.From( pass.renderTargetMS ).To( pass.renderTarget )
											.AddRegion( Default, int2{}, Default, int2{}, view_size )
											.DependsOn( _currTask ));
		}
		return true;
	}
	
/*
=================================================
	_RecreateShaders
=================================================
*/
	bool  ShaderView::_RecreateShaders (const CommandBuffer &cmdBuffer)
	{
		CHECK_ERR( not _shaders.empty() );
		
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
				if ( _CreateShader( cmdBuffer, *iter ))
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
	bool  ShaderView::_CreateShader (const CommandBuffer &cmdBuffer, const ShaderPtr &shader)
	{
		// compile pipeline
		if ( _viewMode == EViewMode::Mono and not shader->_pipeline.mono )
		{
			shader->_pipeline.mono = _Compile( shader->_pplnFilename, shader->_pplnDefines + "\n#define VR_MODE 0\n" );
			CHECK_ERR( shader->_pipeline.mono );
		}
		
		if ( _viewMode == EViewMode::HMD_VR and not shader->_pipeline.hmdVR )
		{
			shader->_pipeline.hmdVR = _Compile( shader->_pplnFilename, shader->_pplnDefines + "\n#define VR_MODE 1\n" );
			CHECK_ERR( shader->_pipeline.hmdVR );
		}
		
		if ( _viewMode == EViewMode::VR180_Video and not shader->_pipeline.vr180 )
		{
			shader->_pipeline.vr180 = _Compile( shader->_pplnFilename, shader->_pplnDefines + "\n#define VR_MODE 180\n" );
			CHECK_ERR( shader->_pipeline.vr180 );
		}
		
		if ( _viewMode == EViewMode::VR360_Video and not shader->_pipeline.vr360 )
		{
			shader->_pipeline.vr360 = _Compile( shader->_pplnFilename, shader->_pplnDefines + "\n#define VR_MODE 360\n" );
			CHECK_ERR( shader->_pipeline.vr360 );
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
		
		const bool	is_vr = (_viewMode == EViewMode::HMD_VR);

		shader->_perEye.resize( is_vr ? 2 : 1 );

		// create render targets
		for (auto& eye_data : shader->_perEye)
		for (auto& pass : eye_data.passes)
		{
			if ( shader->_surfaceSize.has_value() )
				pass.viewport = shader->_surfaceSize.value();
			else
				pass.viewport = uint2( float2(_viewSize) * shader->_surfaceScale.value_or(1.0f) + 0.5f );

			ImageDesc		desc;
			desc.imageType	= EImage::Tex2D;
			desc.dimension	= uint3{ pass.viewport, 1 };
			desc.format		= shader->_format.value_or( _imageFormat );
			desc.usage		= EImageUsage::Transfer | EImageUsage::Sampled | EImageUsage::ColorAttachment;

			EResourceState	def_state	= is_vr ? EResourceState::TransferSrc : EResourceState::Unknown;
			const String	name		= String(shader->Name()) << "-RT-" << ToString(Distance( eye_data.passes.data(), &pass ))
											<< (is_vr ? (shader->_perEye.data() == &eye_data ? "-left" : "-right") : "");
			
			pass.renderTarget = _frameGraph->CreateImage( desc, Default, def_state, name );
			CHECK_ERR( pass.renderTarget );

			if ( _imageSamples > 1 )
			{
				desc.imageType	= EImage::Tex2DMS;
				desc.samples	= MultiSamples{ _imageSamples };

				pass.renderTargetMS = _frameGraph->CreateImage( desc, Default, def_state, name );
				CHECK_ERR( pass.renderTargetMS );
			}
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
			auto&			pass = shader->_perEye[eye].passes[i];
			RawGPipelineID	ppln;

			BEGIN_ENUM_CHECKS();
			switch ( _viewMode ) {
				case EViewMode::Mono :			ppln = shader->_pipeline.mono;	break;
				case EViewMode::HMD_VR :		ppln = shader->_pipeline.hmdVR;	break;
				case EViewMode::VR180_Video :	ppln = shader->_pipeline.vr180;	break;
				case EViewMode::VR360_Video :	ppln = shader->_pipeline.vr360;	break;
			}
			END_ENUM_CHECKS();
			CHECK_ERR( ppln );
			
			CHECK( _frameGraph->InitPipelineResources( ppln, DescriptorSetID{"0"}, OUT pass.resources ));

			pass.resources.BindBuffer( UniformID{"ShadertoyUB"}, shader->_perEye[eye].ubuffer );
			pass.images.resize( shader->_channels.size() );

			for (size_t j = 0; j < shader->_channels.size(); ++j)
			{
				auto&			ch		= shader->_channels[j];
				auto&			image	= pass.images[j];
				UniformID		name	{"iChannel"s << ToString(j)};
				RawSamplerID	samp	= ch.samp ? ch.samp : _linearClampSampler.Get();

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
				if ( _LoadImage( cmdBuffer, ch.name, OUT image ))
				{
					pass.resources.BindTexture( name, image, _linearClampSampler );
					continue;
				}
				
				RETURN_ERR( "unknown channel type, it is not a shader pass and not a file" );
			}
		}
		return true;
	}
	
/*
=================================================
	_DestroyShader
=================================================
*/
	void  ShaderView::_DestroyShader (const ShaderPtr &shader, bool destroyPipeline)
	{
		_frameGraph->WaitIdle();
		
		if ( destroyPipeline ) {
			_frameGraph->ReleaseResource( INOUT shader->_pipeline.mono );
			_frameGraph->ReleaseResource( INOUT shader->_pipeline.hmdVR );
			_frameGraph->ReleaseResource( INOUT shader->_pipeline.vr180 );
			_frameGraph->ReleaseResource( INOUT shader->_pipeline.vr360 );
		}

		for (auto& eye_data : shader->_perEye)
		{
			for (auto& pass : eye_data.passes)
			{
				_frameGraph->ReleaseResource( INOUT pass.renderTargetMS );
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
	GPipelineID  ShaderView::_Compile (StringView name, StringView defs) const
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

			#if VR_MODE == 1
				void mainVR (out vec4 fragColor, in vec2 fragCoord, in vec3 fragRayOri, in vec3 fragRayDir);

				void main ()
				{
					vec2 coord = gl_FragCoord.xy + gl_SamplePosition;
					vec2 uv    = coord / iResolution.xy;
					vec3 dir   = mix( mix( iCameraFrustumLB, iCameraFrustumRB, uv.x ),
									  mix( iCameraFrustumLT, iCameraFrustumRT, uv.x ),
									  uv.y );
					mainVR( out_Color, coord, iCameraPos, dir );
				}

			#elif (VR_MODE == 180) || (VR_MODE == 360)
				void mainVR (out vec4 fragColor, in vec2 fragCoord, in vec3 fragRayOri, in vec3 fragRayDir);

				void main ()
				{
					// from https://developers.google.com/vr/jump/rendering-ods-content.pdf
					vec2	coord	= gl_FragCoord.xy + gl_SamplePosition;
					vec2	uv		= coord / iResolution.xy;
					float	pi		= 3.14159265358979323846f;
					float	IPD		= 64.0e-4;	// (m) Interpupillary distance, the distance between the eyes.

				#if VR_MODE == 360
					float	scale	= IPD * 0.5 * (uv.y < 0.5 ? -1.0 : 1.0);		// vr360 top-bottom
							uv		= vec2( uv.x, fract(uv.y * 2.0) );
				#else
					float	scale	= IPD * 0.5 * (uv.x < 0.5 ? -1.0 : 1.0);		// vr180 left-right
							uv		= vec2( fract(uv.x * 2.0), uv.y ) * 0.5 + 0.25;	// map [0, 1] to [0.25, 0.75]
				#endif

					float	theta	= uv.x * 2.0 * pi - pi;
					float	phi		= pi * 0.5 - uv.y * pi;

					vec3	origin	= vec3(cos(theta), 0.0, sin(theta)) * scale;
					vec3	dir		= vec3(sin(theta) * cos(phi), sin(phi), -cos(theta) * cos(phi));

					mainVR( out_Color, coord, iCameraPos + origin, dir );
				}

			#else
				void mainImage (out vec4 fragColor, in vec2 fragCoord);

				void main ()
				{
					vec2 coord = gl_FragCoord.xy + gl_SamplePosition;
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
	Recompile
=================================================
*/
	bool  ShaderView::Recompile ()
	{
		for (auto& shader : _ordered)
		{
			if ( shader->_pipeline.mono )
			{
				if ( GPipelineID ppln = _Compile( shader->_pplnFilename, shader->_pplnDefines + "\n#define VR_MODE 0\n" ))
				{
					_frameGraph->ReleaseResource( INOUT shader->_pipeline.mono );
					shader->_pipeline.mono = std::move(ppln);
				}
			}

			if ( shader->_pipeline.hmdVR )
			{
				if ( GPipelineID ppln = _Compile( shader->_pplnFilename, shader->_pplnDefines + "\n#define VR_MODE 1\n" ))
				{
					_frameGraph->ReleaseResource( INOUT shader->_pipeline.hmdVR );
					shader->_pipeline.hmdVR = std::move(ppln);
				}
			}

			if ( shader->_pipeline.vr180 )
			{
				if ( GPipelineID ppln = _Compile( shader->_pplnFilename, shader->_pplnDefines + "\n#define VR_MODE 180\n" ))
				{
					_frameGraph->ReleaseResource( INOUT shader->_pipeline.vr180 );
					shader->_pipeline.vr180 = std::move(ppln);
				}
			}

			if ( shader->_pipeline.vr360 )
			{
				if ( GPipelineID ppln = _Compile( shader->_pplnFilename, shader->_pplnDefines + "\n#define VR_MODE 360\n" ))
				{
					_frameGraph->ReleaseResource( INOUT shader->_pipeline.vr360 );
					shader->_pipeline.vr360 = std::move(ppln);
				}
			}
		}
		return true;
	}

/*
=================================================
	AddShader
=================================================
*/
	void  ShaderView::AddShader (const String &name, ShaderDescr &&desc)
	{
		_shaders.insert_or_assign( name, MakeShared<Shader>( name, std::move(desc) ));
	}
	
	void  ShaderView::AddShader (String &&fname)
	{
		ShaderDescr	sh_main;
		sh_main.Pipeline( std::move(fname) );
		AddShader( "main", std::move(sh_main) );
	}

/*
=================================================
	ResetShaders
=================================================
*/
	void  ShaderView::ResetShaders ()
	{
		for (auto& sh : _shaders) {
			_DestroyShader( sh.second, true );
		}
		_shaders.clear();
		_ordered.clear();

		_recreateShaders = true;
	}
	
/*
=================================================
	_CreateSamplers
=================================================
*/
	void  ShaderView::_CreateSamplers ()
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
	bool  ShaderView::_LoadImage (const CommandBuffer &cmdBuffer, const String &filename, OUT ImageID &id)
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

		_currTask = cmdBuffer->AddTask( UpdateImage{}.SetImage( id ).SetData( level.pixels, level.dimension, level.rowPitch, level.slicePitch ).DependsOn( _currTask ));

		_imageCache.insert_or_assign( filename, ImageID{id.Get()} );
		return true;

#	else
		FG_UNUSED( cmdBuffer, filename );
		id = Default;
		return false;
#	endif
	}
	
/*
=================================================
	_HasImage
=================================================
*/
	bool  ShaderView::_HasImage (StringView filename) const
	{
	#ifdef FG_STD_FILESYSTEM
		return FS::exists( FS::path{FG_DATA_PATH}.append(filename) );
	#else
		return true;
	#endif
	}

}	// FG
