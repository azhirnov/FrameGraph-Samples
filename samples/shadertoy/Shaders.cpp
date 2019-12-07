// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Shaders.h"
#include "stl/Algorithms/StringUtils.h"

namespace FG
{
	
	void Shaders::Shadertoy::Glowballs (Ptr<ShaderView> sv)
	{
		ShaderDescr sh_main;
		sh_main.Pipeline( "st_shaders/Glowballs.glsl" );
		sh_main.InChannel( "main", 0 );
		sv->AddShader( "main", std::move(sh_main) );
	}

}	// FG
