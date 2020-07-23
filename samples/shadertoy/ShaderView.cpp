// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ShaderView.h"

#include "scene/Loader/DevIL/DevILLoader.h"
#include "scene/Loader/DDS/DDSLoader.h"
#include "scene/Loader/Intermediate/IntermImage.h"

#include "stl/Algorithms/StringUtils.h"
#include "stl/Stream/FileStream.h"


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
		_surfaceSize{ desc._surfaceSize },				_format{ desc._format },
		_ipd{ desc._ipd }
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
			_frameGraph->ReleaseResource( INOUT _mipmapClampSampler );
			_frameGraph->ReleaseResource( INOUT _mipmapRepeatSampler );
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
	SetControllerPose
=================================================
*/
	void  ShaderView::SetControllerPose (const mat4x4 &left, const mat4x4 &right, uint mask)
	{
		_leftHand  = left;
		_rightHand = right;
		_ubData.iControllerMask = mask;
	}
	
/*
=================================================
	SetSliderState
=================================================
*/
	void  ShaderView::SetSliderState (const vec4 &value)
	{
		_ubData.iSliders = value;
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
	RecordShaderTrace
=================================================
*/
	void  ShaderView::RecordShaderTrace (const vec2 &coord)
	{
		_tracePixel = coord;
	}
	
/*
=================================================
	RecordShaderProfiling
=================================================
*/
	void  ShaderView::RecordShaderProfiling (const vec2 &coord)
	{
		_profilePixel = coord;
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
		uint2	view_size	= _frameGraph->GetDescription( pass.renderTarget ).dimension.xy();

		// update uniform buffer
		{
			if ( _viewMode == EViewMode::HMD_VR )
			{
				_vrCamera.GetFrustum()[eye]->GetRays( OUT _ubData.iCameraFrustumRayLB, OUT _ubData.iCameraFrustumRayLT,
													  OUT _ubData.iCameraFrustumRayRB, OUT _ubData.iCameraFrustumRayRT );
				_ubData.iCameraPos = _vrCamera.Position();

				const auto	view_mat = _vrCamera.ToViewMatrix()[eye];
				_ubData.iLeftControllerPose  = view_mat * _leftHand;
				_ubData.iRightControllerPose = view_mat * _rightHand;
			}
			else
			{
				_camera.GetFrustum().GetRays( OUT _ubData.iCameraFrustumRayLB, OUT _ubData.iCameraFrustumRayLT,
											  OUT _ubData.iCameraFrustumRayRB, OUT _ubData.iCameraFrustumRayRT );
				_ubData.iCameraPos			 = _camera.GetCamera().transform.position;
				_ubData.iControllerMask		 = 0;
				_ubData.iLeftControllerPose  = Mat4x4_Identity;
				_ubData.iRightControllerPose = Mat4x4_Identity;
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
			_ubData.iCameraIPD	= shader->_ipd; 

			_currTask = cmdBuffer->AddTask( UpdateBuffer{}.SetBuffer( eye_data.ubuffer ).AddData( &_ubData, 1 ).DependsOn( _currTask ));
		}

		RawGPipelineID	ppln = 
			_viewMode == EViewMode::Mono		? shader->_pipeline.mono.Get() :
			_viewMode == EViewMode::Mono360		? shader->_pipeline.mono360.Get() :
			_viewMode == EViewMode::HMD_VR		? shader->_pipeline.hmdVR.Get() :
			_viewMode == EViewMode::VR180_Video	? shader->_pipeline.vr180.Get() :
			_viewMode == EViewMode::VR360_Video	? shader->_pipeline.vr360.Get() : Default;

		if ( not ppln )
			return true;

		LogicalPassID	pass_id = cmdBuffer->CreateRenderPass( RenderPassDesc( view_size )
										.AddTarget( RenderTargetID::Color_0, pass.renderTargetMS ? pass.renderTargetMS : pass.renderTarget, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store )
										.AddViewport( view_size ));

		DrawVertices	draw_task;
		draw_task.SetPipeline( ppln );
		draw_task.AddResources( DescriptorSetID{"0"}, &pass.resources );
		draw_task.Draw( 3 ).SetTopology( EPrimitive::TriangleStrip );

		// shader debugger
		{
			if ( isLast and _tracePixel.has_value() )
			{
				const vec2	coord = vec2{pass.viewport.x, pass.viewport.y} * (*_tracePixel) + 0.5f;

				draw_task.EnableFragmentDebugTrace( int(coord.x), int(coord.y) );
				_tracePixel.reset();
			}
		
			if ( isLast and _profilePixel.has_value() )
			{
				const vec2	coord = vec2{pass.viewport.x, pass.viewport.y} * (*_profilePixel) + 0.5f;

				draw_task.EnableFragmentDebugTrace( int(coord.x), int(coord.y) );
				_profilePixel.reset();
			}
		}

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
		String	samplers;
		for (auto& ch : shader->_channels)
		{
			ImageID		image;
			EImage		type	= EImage::Tex2D;	// 2D render target by default

			if ( _LoadImage( cmdBuffer, ch.name, ch.flipY, OUT image ))
			{
				type = _frameGraph->GetDescription( image ).imageType;
				_frameGraph->ReleaseResource( image );
			}

			if ( type == EImage::Tex2D )
				samplers << "layout (binding=" << ToString(ch.index+1) << ") uniform sampler2D iChannel" << ToString(ch.index) << ";\n";
			else
			if ( type == EImage::Tex3D )
				samplers << "layout (binding=" << ToString(ch.index+1) << ") uniform sampler3D iChannel" << ToString(ch.index) << ";\n";
			else
			if ( type == EImage::TexCube )
				samplers << "layout (binding=" << ToString(ch.index+1) << ") uniform samplerCube iChannel" << ToString(ch.index) << ";\n";
			else
			if ( type == EImage::Unknown )
			{}
			else
				RETURN_ERR( "unsupported imag type" );
		}

		// compile pipeline
		if ( _viewMode == EViewMode::Mono and not shader->_pipeline.mono )
		{
			shader->_pipeline.mono = _Compile( shader->_pplnFilename, shader->_pplnDefines + "\n#define VIEW_MODE 0\n", samplers );
			if ( not shader->_pipeline.mono )
				shader->_pipeline.mono = _CreateDefault( samplers );
		}

		if ( _viewMode == EViewMode::Mono360 and not shader->_pipeline.mono360 )
		{
			shader->_pipeline.mono360 = _Compile( shader->_pplnFilename, shader->_pplnDefines + "\n#define VIEW_MODE 3601\n", samplers );
			if ( not shader->_pipeline.mono360 )
				shader->_pipeline.mono360 = _CreateDefault( samplers );
		}
		
		if ( _viewMode == EViewMode::HMD_VR and not shader->_pipeline.hmdVR )
		{
			shader->_pipeline.hmdVR = _Compile( shader->_pplnFilename, shader->_pplnDefines + "\n#define VIEW_MODE 1\n", samplers );
			if ( not shader->_pipeline.hmdVR )
				shader->_pipeline.hmdVR = _CreateDefault( samplers );
		}
		
		if ( _viewMode == EViewMode::VR180_Video and not shader->_pipeline.vr180 )
		{
			shader->_pipeline.vr180 = _Compile( shader->_pplnFilename, shader->_pplnDefines + "\n#define VIEW_MODE 1802\n", samplers );
			if ( not shader->_pipeline.vr180 )
				shader->_pipeline.vr180 = _CreateDefault( samplers );
		}
		
		if ( _viewMode == EViewMode::VR360_Video and not shader->_pipeline.vr360 )
		{
			shader->_pipeline.vr360 = _Compile( shader->_pplnFilename, shader->_pplnDefines + "\n#define VIEW_MODE 3602\n", samplers );
			if ( not shader->_pipeline.vr360 )
				shader->_pipeline.vr360 = _CreateDefault( samplers );
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
		{
			if ( _imageSamples > 1 )
			{
				ImageDesc		desc;
				desc.imageType	= EImage::Tex2DMS;
				desc.format		= shader->_format.value_or( _imageFormat );
				desc.usage		= EImageUsage::TransferSrc | EImageUsage::ColorAttachment;
				desc.samples	= MultiSamples{ _imageSamples };
				
				if ( shader->_surfaceSize.has_value() )
					desc.dimension = uint3( shader->_surfaceSize.value(), 1 );
				else
					desc.dimension = uint3( uint2( float2(_viewSize) * shader->_surfaceScale.value_or(1.0f) + 0.5f ), 1 );

				eye_data.renderTargetMS = _frameGraph->CreateImage( desc, Default, EResourceState::ColorAttachmentWrite, String(shader->Name()) << "-MSRT" );
				CHECK_ERR( eye_data.renderTargetMS );
			}

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
				desc.usage		= EImageUsage::Transfer | EImageUsage::Sampled | EImageUsage::ColorAttachment | EImageUsage::Storage;

				EResourceState	def_state	= is_vr ? EResourceState::TransferSrc : EResourceState::Unknown;
				const String	name		= String(shader->Name()) << "-RT-" << ToString(Distance( eye_data.passes.data(), &pass ))
												<< (is_vr ? (shader->_perEye.data() == &eye_data ? "-left" : "-right") : "");
			
				pass.renderTarget = _frameGraph->CreateImage( desc, Default, def_state, name );
				CHECK_ERR( pass.renderTarget );

				if ( _imageSamples > 1 )
				{
					pass.renderTargetMS = eye_data.renderTargetMS.Get();
				}
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
				case EViewMode::Mono :			ppln = shader->_pipeline.mono;		break;
				case EViewMode::Mono360 :		ppln = shader->_pipeline.mono360;	break;
				case EViewMode::HMD_VR :		ppln = shader->_pipeline.hmdVR;		break;
				case EViewMode::VR180_Video :	ppln = shader->_pipeline.vr180;		break;
				case EViewMode::VR360_Video :	ppln = shader->_pipeline.vr360;		break;
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
				UniformID		name	{"iChannel"s << ToString(ch.index)};
				RawSamplerID	samp	= ch.samp ? ch.samp : _linearClampSampler.Get();

				// find channel in shader passes
				ShadersMap_t::iterator	iter = _shaders.find( ch.name );
				if ( iter != _shaders.end() )
				{
					ASSERT( not ch.flipY );	// not supported for render target

					if ( iter->second == shader )
						// use image from previous pass
						image = _frameGraph->AcquireResource( shader->_perEye[eye].passes[(i-1) & 1].renderTarget );
					else
						image = _frameGraph->AcquireResource( iter->second->_perEye[eye].passes[i].renderTarget );
					
					pass.resources.BindTexture( name, image, samp );
					continue;
				}
				
				// find channel in loadable images
				if ( _LoadImage( cmdBuffer, ch.name, ch.flipY, OUT image ))
				{
					pass.resources.BindTexture( name, image, samp );
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
		
		if ( destroyPipeline )
		{
			_frameGraph->ReleaseResource( INOUT shader->_pipeline.mono );
			_frameGraph->ReleaseResource( INOUT shader->_pipeline.mono360 );
			_frameGraph->ReleaseResource( INOUT shader->_pipeline.hmdVR );
			_frameGraph->ReleaseResource( INOUT shader->_pipeline.vr180 );
			_frameGraph->ReleaseResource( INOUT shader->_pipeline.vr360 );
		}

		for (auto& eye_data : shader->_perEye)
		{
			_frameGraph->ReleaseResource( INOUT eye_data.renderTargetMS );
			_frameGraph->ReleaseResource( INOUT eye_data.ubuffer );

			for (auto& pass : eye_data.passes)
			{
				_frameGraph->ReleaseResource( INOUT pass.renderTarget );
			
				for (auto& img : pass.images)
				{
					_frameGraph->ReleaseResource( INOUT img );
				}
			}
		}
		shader->_perEye.clear();
	}
	
/*
=================================================
	_CreateDefault
=================================================
*/
	GPipelineID  ShaderView::_CreateDefault (StringView samplers) const
	{
		return _Compile( "st_shaders/default.glsl", "", samplers );
	}

/*
=================================================
	_Compile
=================================================
*/
	GPipelineID  ShaderView::_Compile (StringView name, StringView defs, StringView samplers) const
	{
		const char	vs_source[] = R"#(
			const vec2	g_Positions[] = {
				{ -1.0f, 3.0f },  { -1.0f, -1.0f },  { 3.0f, -1.0f }
			};

			void main() {
				gl_Position	= vec4( g_Positions[gl_VertexIndex], 0.0f, 1.0f );
			}
		)#";

		const char	fs_source[] = R"#(
			#extension GL_GOOGLE_include_directive : require
			#extension GL_EXT_control_flow_attributes : require

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
				float	iCameraIPD;				// (m) Interpupillary distance, the distance between the eyes.
				vec3	iCameraFrustumLB;		// frustum rays (left bottom, right bottom, left top, right top)
				vec3	iCameraFrustumRB;
				vec3	iCameraFrustumLT;
				vec3	iCameraFrustumRT;
				vec3	iCameraPos;				// camera position in world space
				int		iEyeIndex;
				int		iControllerMask;
				mat4x4	iLeftControllerPose;	// VR controllers
				mat4x4	iRightControllerPose;
				vec4	iSliders;
			};

			layout(location=0) out vec4	out_Color;

			#if VIEW_MODE == 1
				void mainVR (out vec4 fragColor, in vec2 fragCoord, in vec3 fragRayOri, in vec3 fragRayDir);

				void main ()
				{
					vec2 coord = gl_FragCoord.xy + gl_SamplePosition;
					vec2 uv    = coord / iResolution.xy;
					vec3 dir   = mix( mix( iCameraFrustumLB, iCameraFrustumRB, uv.x ),
									  mix( iCameraFrustumLT, iCameraFrustumRT, uv.x ),
									  uv.y );
					coord = vec2(coord.x - 0.5, iResolution.y - coord.y + 0.5);
					mainVR( out_Color, coord, iCameraPos, dir );
				}

			#elif (VIEW_MODE == 1802) || (VIEW_MODE == 3602) || (VIEW_MODE == 3601)
				void mainVR (out vec4 fragColor, in vec2 fragCoord, in vec3 fragRayOri, in vec3 fragRayDir);

				void main ()
				{
					// from https://developers.google.com/vr/jump/rendering-ods-content.pdf
					vec2	coord	= gl_FragCoord.xy + gl_SamplePosition;
					vec2	uv		= coord / iResolution.xy;
					float	pi		= 3.14159265358979323846f;

				#if   VIEW_MODE == 3602
					float	scale	= iCameraIPD * 0.5 * (uv.y < 0.5 ? -1.0 : 1.0);		// vr360 top-bottom
							uv		= vec2( uv.x, (uv.y < 0.5 ? uv.y : uv.y - 0.5) * 2.0 );
				#elif VIEW_MODE == 1802
					float	scale	= iCameraIPD * 0.5 * (uv.x < 0.5 ? -1.0 : 1.0);		// vr180 left-right
							uv		= vec2( (uv.x < 0.5 ? uv.x : uv.x - 0.5) * 0.5 + 0.25, uv.y );	// map [0, 1] to [0.25, 0.75]
				#elif VIEW_MODE == 3601
					float	scale	= 1.0;
				#endif

					float	theta	= (uv.x) * 2.0 * pi - pi;
					float	phi		= pi * 0.5 - uv.y * pi;

					vec3	origin	= vec3(cos(theta), 0.0, sin(theta)) * scale;
					vec3	dir		= vec3(sin(theta) * cos(phi), sin(phi), -cos(theta) * cos(phi));

					coord = vec2(coord.x - 0.5, iResolution.y - coord.y + 0.5);
					mainVR( out_Color, coord, iCameraPos + origin, dir );
				}

			#else
				void mainImage (out vec4 fragColor, in vec2 fragCoord);

				void main ()
				{
					vec2 coord = gl_FragCoord.xy + gl_SamplePosition;
					coord = vec2(coord.x - 0.5, iResolution.y - coord.y + 0.5);

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

		src0 << samplers;
		src0 << fs_source;
		src0 << "\n" << src1;

		GraphicsPipelineDesc	desc;
		desc.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_110, "main", vs_source );
		desc.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_110 | EShaderLangFormat::_DebugModeMask, "main", std::move(src0), name );

		GPipelineID ppln = _frameGraph->CreatePipeline( desc, name );

		if ( ppln )
			FG_LOGI( "Compiled pipeline '"s << name << "'" );

		return ppln;
	}
	
/*
=================================================
	Recompile
=================================================
*/
	bool  ShaderView::Recompile (const CommandBuffer &cmdBuffer)
	{
		FG_LOGI( "\n========================= Recompile shaders =========================\n" );

		for (auto& shader : _ordered)
		{
			String	samplers;
			for (auto& ch : shader->_channels)
			{
				ImageID		image;
				EImage		type	= EImage::Tex2D;	// 2D render target by default

				if ( _LoadImage( cmdBuffer, ch.name, ch.flipY, OUT image ))
				{
					type = _frameGraph->GetDescription( image ).imageType;
					_frameGraph->ReleaseResource( image );
				}

				if ( type == EImage::Tex2D )
					samplers << "layout (binding=" << ToString(ch.index+1) << ") uniform sampler2D iChannel" << ToString(ch.index) << ";\n";
				else
				if ( type == EImage::Tex3D )
					samplers << "layout (binding=" << ToString(ch.index+1) << ") uniform sampler3D iChannel" << ToString(ch.index) << ";\n";
				else
				if ( type == EImage::TexCube )
					samplers << "layout (binding=" << ToString(ch.index+1) << ") uniform samplerCube iChannel" << ToString(ch.index) << ";\n";
				else
				if ( type == EImage::Unknown )
				{}
				else
					RETURN_ERR( "unsupported imag type" );
			}

			if ( shader->_pipeline.mono )
			{
				if ( GPipelineID ppln = _Compile( shader->_pplnFilename, shader->_pplnDefines + "\n#define VIEW_MODE 0\n", samplers ))
				{
					_frameGraph->ReleaseResource( INOUT shader->_pipeline.mono );
					shader->_pipeline.mono = std::move(ppln);
				}
			}

			if ( shader->_pipeline.mono360 )
			{
				if ( GPipelineID ppln = _Compile( shader->_pplnFilename, shader->_pplnDefines + "\n#define VIEW_MODE 3601\n", samplers ))
				{
					_frameGraph->ReleaseResource( INOUT shader->_pipeline.mono360 );
					shader->_pipeline.mono360 = std::move(ppln);
				}
			}

			if ( shader->_pipeline.hmdVR )
			{
				if ( GPipelineID ppln = _Compile( shader->_pplnFilename, shader->_pplnDefines + "\n#define VIEW_MODE 1\n", samplers ))
				{
					_frameGraph->ReleaseResource( INOUT shader->_pipeline.hmdVR );
					shader->_pipeline.hmdVR = std::move(ppln);
				}
			}

			if ( shader->_pipeline.vr180 )
			{
				if ( GPipelineID ppln = _Compile( shader->_pplnFilename, shader->_pplnDefines + "\n#define VIEW_MODE 1802\n", samplers ))
				{
					_frameGraph->ReleaseResource( INOUT shader->_pipeline.vr180 );
					shader->_pipeline.vr180 = std::move(ppln);
				}
			}

			if ( shader->_pipeline.vr360 )
			{
				if ( GPipelineID ppln = _Compile( shader->_pplnFilename, shader->_pplnDefines + "\n#define VIEW_MODE 3602\n", samplers ))
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
		desc.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Nearest );
		_linearClampSampler = _frameGraph->CreateSampler( desc, "LinearClamp" );
		
		desc.SetAddressMode( EAddressMode::Repeat );
		desc.SetFilter( EFilter::Nearest, EFilter::Nearest, EMipmapFilter::Nearest );
		_nearestRepeatSampler = _frameGraph->CreateSampler( desc, "NearestRepeat" );
		
		desc.SetAddressMode( EAddressMode::Repeat );
		desc.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Nearest );
		_linearRepeatSampler = _frameGraph->CreateSampler( desc, "LinearRepeat" );
		
		desc.SetAddressMode( EAddressMode::ClampToEdge );
		desc.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear );
		_mipmapClampSampler = _frameGraph->CreateSampler( desc, "MipmapClamp" );
		
		desc.SetAddressMode( EAddressMode::Repeat );
		desc.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear );
		_mipmapRepeatSampler = _frameGraph->CreateSampler( desc, "MipmapRepeat" );
	}
	
/*
=================================================
	_LoadImage
=================================================
*/
	bool  ShaderView::_LoadImage (const CommandBuffer &cmdBuffer, const String &filename, bool flipY, OUT ImageID &id)
	{
	#ifdef FS_HAS_FILESYSTEM
		EImageType	img_type = _GetImageFileType( filename );
		FS::path	fpath	 = FS::path{FG_DATA_PATH}.append(filename);

		BEGIN_ENUM_CHECKS()
		switch ( img_type )
		{
			case EImageType::Unknown :	return false;
			case EImageType::DevIL :	return _LoadImage2D( cmdBuffer, fpath.string(), flipY, OUT id );
			case EImageType::DDS :		return _LoadDDS( cmdBuffer, fpath.string(), flipY, OUT id );
			case EImageType::Raw3D :	CHECK_ERR( not flipY );	return _LoadImage3D( cmdBuffer, fpath.string(), OUT id );
		}
		END_ENUM_CHECKS();

		RETURN_ERR( "unsupported image type" );

	#else
		return false;
	#endif
	}
	
/*
=================================================
	_LoadDDS
=================================================
*/
	bool  ShaderView::_LoadDDS (const CommandBuffer &cmdBuffer, const String &filename, bool flipY, OUT ImageID &id)
	{
	#ifdef FS_HAS_FILESYSTEM
		CHECK( not flipY );
		auto	iter = _imageCache.find( filename );
		
		if ( iter != _imageCache.end() and _frameGraph->IsResourceAlive( iter->second ))
		{
			id = _frameGraph->AcquireResource( iter->second );
			return true;
		}

		DDSLoader		loader;
		FS::path		fpath	{filename};
		auto			image	= MakeShared<IntermImage>( fpath.string() );
			
		CHECK_ERR( loader.LoadImage( image, {}, null, flipY ));
			
		auto&	level	 = image->GetData()[0][0];
		String	img_name = fpath.filename().string();

		id = _frameGraph->CreateImage( ImageDesc{ image->GetType(), level.dimension, level.format, EImageUsage::Transfer | EImageUsage::Sampled },
										Default, img_name );
		CHECK_ERR( id );

		_currTask = cmdBuffer->AddTask( UpdateImage{}.SetImage( id ).SetData( level.pixels, level.dimension, level.rowPitch, level.slicePitch ).DependsOn( _currTask ));
			
		_imageCache.insert_or_assign( filename, _frameGraph->AcquireResource(id) );
		return true;

	#else
		FG_UNUSED( cmdBuffer, filename, flipY );
		id = Default;
		return false;
	#endif
	}

/*
=================================================
	_LoadImage2D
=================================================
*/
	bool  ShaderView::_LoadImage2D (const CommandBuffer &cmdBuffer, const String &filename, bool flipY, OUT ImageID &id)
	{
	#if defined(FG_ENABLE_DEVIL) and defined(FS_HAS_FILESYSTEM)

		String	name = filename + (flipY ? "|flip" : "");
		auto	iter = _imageCache.find( name );
		
		if ( iter != _imageCache.end() and _frameGraph->IsResourceAlive( iter->second ))
		{
			id = _frameGraph->AcquireResource( iter->second );
			return true;
		}

		DevILLoader		loader;
		FS::path		fpath	{filename};
		auto			image	= MakeShared<IntermImage>( fpath.string() );

		CHECK_ERR( loader.LoadImage( image, {}, null, flipY ));

		auto&	level	 = image->GetData()[0][0];
		String	img_name = fpath.filename().string();

		id = _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, level.dimension, level.format, EImageUsage::Transfer | EImageUsage::Sampled },
										Default, img_name );
		CHECK_ERR( id );

		_currTask = cmdBuffer->AddTask( UpdateImage{}.SetImage( id ).SetData( level.pixels, level.dimension, level.rowPitch, level.slicePitch ).DependsOn( _currTask ));
		_currTask = cmdBuffer->AddTask( GenerateMipmaps{}.SetImage( id ).SetRange( 0_mipmap, UMax ).DependsOn( _currTask ));

		_imageCache.insert_or_assign( name, _frameGraph->AcquireResource(id) );
		return true;
		
	#else
		FG_UNUSED( cmdBuffer, filename, flipY );
		id = Default;
		return false;
	#endif
	}
	
/*
=================================================
	_LoadImage3D
=================================================
*/
	bool  ShaderView::_LoadImage3D (const CommandBuffer &cmdBuffer, const String &filename, OUT ImageID &id)
	{
		auto	iter = _imageCache.find( filename );
		
		if ( iter != _imageCache.end() and _frameGraph->IsResourceAlive( iter->second ))
		{
			id = _frameGraph->AcquireResource( iter->second );
			return true;
		}

		uint		header[5] = {};
		FileRStream	file		{filename};

		CHECK_ERR( file.IsOpen() );
		CHECK_ERR( file.Read( header, BytesU::SizeOf(header) ));

		Array<uint8_t>	buf;
		CHECK_ERR( file.Read( size_t(file.RemainingSize()), OUT buf ));

		CHECK_ERR( header[0] == 0x004e4942 );

		EPixelFormat	fmt;
		switch ( header[4] )
		{
			case 1 :	fmt = EPixelFormat::R8_UNorm;		break;
			case 4 :	fmt = EPixelFormat::RGBA8_UNorm;	break;
			default :	RETURN_ERR( "unknown format" );
		}

		FS::path	fpath	{filename};
		String		img_name = fpath.filename().string();

		id = _frameGraph->CreateImage( ImageDesc{ EImage::Tex3D, uint3{header[1], header[2], header[3]}, fmt, EImageUsage::Transfer | EImageUsage::Sampled },
									   Default, img_name );
		CHECK_ERR( id );

		_currTask = cmdBuffer->AddTask( UpdateImage{}.SetImage( id ).SetData( buf, uint3{header[1], header[2], header[3]} ).DependsOn( _currTask ));
		_currTask = cmdBuffer->AddTask( GenerateMipmaps{}.SetImage( id ).SetRange( 0_mipmap, UMax ).DependsOn( _currTask ));
		
		_imageCache.insert_or_assign( filename, _frameGraph->AcquireResource(id) );
		return true;
	}

/*
=================================================
	_HasImage
=================================================
*/
	bool  ShaderView::_HasImage (StringView filename) const
	{
	#ifdef FS_HAS_FILESYSTEM
		return FS::exists( FS::path{FG_DATA_PATH}.append( filename.begin(), filename.end() ));
	#else
		return true;
	#endif
	}
	
/*
=================================================
	_GetImageFileType
=================================================
*/
	ShaderView::EImageType  ShaderView::_GetImageFileType (StringView filename) const
	{
	#ifdef FS_HAS_FILESYSTEM
		FS::path	fpath {filename};
		auto		ext	= fpath.extension();

		if ( ext == ".bin" )
			return EImageType::Raw3D;
		
		if ( ext == ".dds" )
			return EImageType::DDS;

		if ( ext.empty() )
			return EImageType::Unknown;

		// TODO: cubemap

		return EImageType::DevIL;

	#else
		return EImageType::Unknown;
	#endif
	}


}	// FG
