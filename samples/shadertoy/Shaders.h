// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ShaderView.h"

namespace FG
{

	struct Shaders
	{
		using ShaderDescr	= ShaderView::ShaderDescr;

		struct Shadertoy
		{
			static void AlienBeacon (Ptr<ShaderView> sv);
			static void Auroras (Ptr<ShaderView> sv)				{ sv->AddShader("st_shaders/Auroras.glsl"); }
			static void CloudFlight (Ptr<ShaderView> sv);
			static void CloudyTerrain (Ptr<ShaderView> sv);
			static void Canyon (Ptr<ShaderView> sv);
			static void CanyonPass (Ptr<ShaderView> sv);
			static void Dwarf (Ptr<ShaderView> sv)					{ sv->AddShader("st_shaders/Dwarf.glsl"); }
			static void DesertPassage (Ptr<ShaderView> sv);
			static void DesertSand (Ptr<ShaderView> sv);
			static void Glowballs (Ptr<ShaderView> sv);
			static void GlowCity (Ptr<ShaderView> sv)				{ sv->AddShader("st_shaders/GlowCity.glsl"); }
			static void Generators (Ptr<ShaderView> sv)				{ sv->AddShader("st_shaders/Generators.glsl"); }
			static void Insect (Ptr<ShaderView> sv);
			static void Luminescence (Ptr<ShaderView> sv);
			static void Mesas (Ptr<ShaderView> sv);
			static void NovaMarble (Ptr<ShaderView> sv)				{ sv->AddShader("st_shaders/NovaMarble.glsl"); }
			static void Organix (Ptr<ShaderView> sv);
			static void PlasmaGlobe (Ptr<ShaderView> sv);
			static void SpaceEgg (Ptr<ShaderView> sv)				{ sv->AddShader("st_shaders/SpaceEgg.glsl"); }
			static void SculptureIII (Ptr<ShaderView> sv);
			static void StructuredVolSampling (Ptr<ShaderView> sv);
			static void ServerRoom (Ptr<ShaderView> sv);
			static void Volcanic (Ptr<ShaderView> sv);
		};
		
		struct ShadertoyVR
		{
			static void AncientMars (Ptr<ShaderView> sv);
			static void Apollonian (Ptr<ShaderView> sv)				{ sv->AddShader("st_shaders/Apollonian.glsl"); }
			static void AtTheMountains (Ptr<ShaderView> sv);
			static void Catacombs (Ptr<ShaderView> sv);
			static void CavePillars (Ptr<ShaderView> sv);
			static void DesertCanyon (Ptr<ShaderView> sv);
			static void FrozenWasteland (Ptr<ShaderView> sv);
			static void FractalExplorer (Ptr<ShaderView> sv);
			static void IveSeen (Ptr<ShaderView> sv);
			static void NebulousTunnel (Ptr<ShaderView> sv);
			static void NightMist (Ptr<ShaderView> sv)				{ sv->AddShader("st_shaders/NightMist.glsl"); }
			static void OpticalDeconstruction (Ptr<ShaderView> sv);
			static void ProteanClouds (Ptr<ShaderView> sv);
			static void PeacefulPostApocalyptic (Ptr<ShaderView> sv);
			static void SirenianDawn (Ptr<ShaderView> sv);
			static void SphereFBM (Ptr<ShaderView> sv)				{ sv->AddShader("st_shaders/SphereFBM.glsl"); }
			static void Skyline (Ptr<ShaderView> sv);
			static void Xyptonjtroz (Ptr<ShaderView> sv)			{ sv->AddShader("st_shaders/Xyptonjtroz.glsl"); }
			static void ThenAndBefore (Ptr<ShaderView> sv);
		};
		
		struct My
		{
			static void ConvexShape2D (Ptr<ShaderView> sv)			{ sv->AddShader("my_shaders/ConvexShape2D.glsl"); }
			static void VoronoiRecursion (Ptr<ShaderView> sv)		{ sv->AddShader("my_shaders/VoronoiRecursion.glsl"); }
			static void OptimizedSDF (Ptr<ShaderView> sv);
			static void PrecalculatedRays (Ptr<ShaderView> sv);
			static void ThousandsOfStars (Ptr<ShaderView> sv);
		};

		struct MyVR
		{
			static void ConvexShape3D (Ptr<ShaderView> sv)			{ sv->AddShader("my_shaders/ConvexShape3D.glsl"); }
			static void Building_1 (Ptr<ShaderView> sv)				{ sv->AddShader("my_shaders/Building_1.glsl"); }
			static void Building_2 (Ptr<ShaderView> sv)				{ sv->AddShader("my_shaders/Building_2.glsl"); } 
		};
	};

}	// FG
