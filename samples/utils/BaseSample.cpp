// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "BaseSample.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Stream/FileStream.h"

#ifdef FG_ENABLE_IMGUI
#	include "imgui_internal.h"
#endif

namespace FG
{
	
/*
=================================================
	constructor
=================================================
*/
	BaseSample::BaseSample ()
	{
	#ifdef FG_ENABLE_IMGUI
		_mouseJustPressed.fill( EKeyAction::Up );
	#endif
	}
	
/*
=================================================
	destructor
=================================================
*/
	BaseSample::~BaseSample ()
	{
	#ifdef FG_ENABLE_IMGUI
		if ( _frameGraph )
		{
			_uiRenderer.Deinitialize( _frameGraph );
		}
	#endif
	}

/*
=================================================
	_CreateSamplers
=================================================
*/
	bool  BaseSample::_CreateSamplers ()
	{
		_sampler.linear		= _frameGraph->CreateSampler( SamplerDesc{}.SetAddressMode( EAddressMode::Repeat )
									.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear )).Release();
		
		_sampler.anisotropy	= _frameGraph->CreateSampler( SamplerDesc{}.SetAddressMode( EAddressMode::ClampToEdge )
									.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear )
									.SetAnisotropy( 16.0f )).Release();

		_sampler.linearClamp = _frameGraph->CreateSampler( SamplerDesc{}.SetAddressMode( EAddressMode::ClampToEdge )
									.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear )).Release();

		_sampler.shadow		= _frameGraph->CreateSampler( SamplerDesc{}.SetAddressMode( EAddressMode::ClampToEdge )
									.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear )
									.SetCompareOp( ECompareOp::LEqual )).Release();

		CHECK_ERR( _sampler.linear and _sampler.anisotropy and _sampler.linearClamp and _sampler.shadow );
		return true;
	}

/*
=================================================
	_InitUI
=================================================
*/
	bool  BaseSample::_InitUI ()
	{
	#ifdef FG_ENABLE_IMGUI

		// initialize imgui and renderer
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		CHECK_ERR( _uiRenderer.Initialize( _frameGraph, GImGui ));
	#endif
		return true;
	}
	
/*
=================================================
	_DrawUI
=================================================
*/
	void  BaseSample::_DrawUI (const CommandBuffer &cmdbuf, RawImageID renderTarget, ArrayView<Task> dependencies)
	{
		#ifdef FG_ENABLE_IMGUI
		if ( _settingsWndOpen )
		{
			_UpdateUI( _frameGraph->GetDescription( renderTarget ).dimension.xy() );

			auto&	draw_data = *ImGui::GetDrawData();

			if ( draw_data.TotalVtxCount > 0 )
			{
				LogicalPassID	pass_id = cmdbuf->CreateRenderPass( RenderPassDesc{ int2{float2{ draw_data.DisplaySize.x, draw_data.DisplaySize.y }} }
												.AddViewport(float2{ draw_data.DisplaySize.x, draw_data.DisplaySize.y })
												.AddTarget( RenderTargetID::Color_0, renderTarget, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store ));

				_uiRenderer.Draw( cmdbuf, pass_id, dependencies );
			}
		}
		#endif
	}

/*
=================================================
	_ScaleSurface
=================================================
*/
	uint2  BaseSample::_ScaleSurface (const uint2 &size, int scaleIdx)
	{
		switch ( scaleIdx )
		{
			case -1 :	return size / 2;
			case -2 :	return size / 4;
			case 1 :	return size * 2;
			default :	return size;
		}
	}
	
/*
=================================================
	_SurfaceScaleName
=================================================
*/
	const char*  BaseSample::_SurfaceScaleName (int scaleIdx)
	{
		switch ( scaleIdx )
		{
			case -1 :	return "1/2";
			case -2 :	return "1/4";
			case 1 :	return "x2";
			default :	return "=";
		}
	}
	
/*
=================================================
	_UpdateUI
=================================================
*/
	bool  BaseSample::_UpdateUI (const uint2 &dim)
	{
	#ifdef FG_ENABLE_IMGUI
		ImGuiIO &	io = ImGui::GetIO();
		CHECK_ERR( io.Fonts->IsBuilt() );

		io.DisplaySize	= ImVec2{float(dim.x), float(dim.y)};
		io.DeltaTime	= FrameTime().count();

		CHECK_ERR( _UpdateInput() );

		ImGui::NewFrame();
			
		if ( ImGui::Begin( "Settings", INOUT &_settingsWndOpen, ImGuiWindowFlags_AlwaysAutoResize ))
		{
			OnUpdateUI();

			_uiWindowRect = RectF{ VecCast( ImGui::GetWindowSize() )};
			_uiWindowRect += VecCast( ImGui::GetWindowPos() );
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
	bool  BaseSample::_UpdateInput ()
	{
	#ifdef FG_ENABLE_IMGUI
		ImGuiIO &	io = ImGui::GetIO();

		for (size_t i = 0; i < _mouseJustPressed.size(); ++i)
		{
			io.MouseDown[i] = (_mouseJustPressed[i] != EKeyAction::Up);
		}
		
		io.MousePos = { GetMousePos().x, GetMousePos().y };

		std::memset( io.NavInputs, 0, sizeof(io.NavInputs) );
		return true;

	#else
		return true;
	#endif
	}

/*
=================================================
	OnKey
=================================================
*/
	void  BaseSample::OnKey (StringView key, EKeyAction action)
	{
		BaseSceneApp::OnKey( key, action );

	#ifdef FG_ENABLE_IMGUI
		if ( key == "left mb" )		_mouseJustPressed[0] = action;
		if ( key == "right mb" )	_mouseJustPressed[1] = action;
		if ( key == "middle mb" )	_mouseJustPressed[2] = action;
	#endif
	}

/*
=================================================
	OnMouseMove
=================================================
*/
	void  BaseSample::OnMouseMove (const float2 &pos)
	{
	#ifdef FG_ENABLE_IMGUI
		_mouseOverUI = _settingsWndOpen and _uiWindowRect.Intersects( pos );
		
		_EnableCameraMovement( not _mouseOverUI );
	#endif

		BaseSceneApp::OnMouseMove( pos );
	}

/*
=================================================
	_LoadShader
=================================================
*/
	String  BaseSample::_LoadShader (NtStringView filename)
	{
		FileRStream		file{ filename };
		CHECK_ERR( file.IsOpen() );

		String	str;
		CHECK_ERR( file.Read( size_t(file.Size()), OUT str ));

		return str;
	}

}	// FG
