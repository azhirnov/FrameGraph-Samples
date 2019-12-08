// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ShaderView.h"

namespace FG
{

	struct Shaders
	{
		using ShaderDescr	= ShaderView::ShaderDescr;

		struct Shadertoy
		{
			static void Glowballs (Ptr<ShaderView> sv);
			static void SirenianDawn (Ptr<ShaderView> sv);
			static void NovaMarble (Ptr<ShaderView> sv)				{ sv->AddShader("st_shaders/NovaMarble.glsl"); }
			static void GlowCity (Ptr<ShaderView> sv)				{ sv->AddShader("st_shaders/GlowCity.glsl"); }
		};
		
		struct ShadertoyVR
		{
			static void Skyline (Ptr<ShaderView> sv)				{ sv->AddShader("st_shaders/Skyline.glsl"); }
			static void SkylineFreeCam (Ptr<ShaderView> sv)			{ sv->AddShader("st_shaders/Skyline2.glsl"); }
			static void NightMist (Ptr<ShaderView> sv)				{ sv->AddShader("st_shaders/NightMist.glsl"); }
			static void Apollonian (Ptr<ShaderView> sv)				{ sv->AddShader("st_shaders/Apollonian.glsl"); }
			static void ProteanClouds (Ptr<ShaderView> sv)			{ sv->AddShader("st_shaders/ProteanClouds.glsl"); }
			static void Catacombs (Ptr<ShaderView> sv);
			static void DesertCanyon (Ptr<ShaderView> sv);
			static void FrozenWasteland (Ptr<ShaderView> sv);
		};
		
		struct My
		{
			static void ConvexShape2D (Ptr<ShaderView> sv)			{ sv->AddShader("my_shaders/ConvexShape2D.glsl"); }
			static void OptimizedSDF (Ptr<ShaderView> sv);
		};

		struct MyVR
		{
			static void ConvexShape3D (Ptr<ShaderView> sv)			{ sv->AddShader("my_shaders/ConvexShape3D.glsl"); }
			static void Building_1 (Ptr<ShaderView> sv)				{ sv->AddShader("my_shaders/Building_1.glsl"); }
			static void Building_2 (Ptr<ShaderView> sv)				{ sv->AddShader("my_shaders/Building_2.glsl"); } 
		};
	};

}	// FG
