/*
	Lighting models
*/

#include "Math.glsl"


//-----------------------------------------------------------------------------
// Attenuation

float  LinearAttenuation (const float dist, const float radius)
{
	return Saturate( 1.0 - (dist / radius) );
}

float  QuadraticAttenuation (const float dist, const float radius)
{
	float	f = dist / radius;
	return Saturate( 1.0 - f*f );
}

float  Attenuation (const float3 attenFactor, const float dist)
{
	return 1.0 / ( attenFactor.x + attenFactor.y * dist + attenFactor.z * dist * dist );
}



//-----------------------------------------------------------------------------
// PBR

float3 SpecularBRDF (const float3 albedo, const float3 lightColor, const float3 lightDir, const float3 viewDir,
					 const float3 surfNorm, const float metallic, const float roughness)
{
	float3 H = Normalize( viewDir + lightDir );
	float dotNV = Saturate( Dot( surfNorm, viewDir ));
	float dotNL = Saturate( Dot( surfNorm, lightDir ));
	float dotLH = Saturate( Dot( lightDir, H ));
	float dotNH = Saturate( Dot( surfNorm, H ));

	float3 color = float3(0.0);

	if ( dotNL > 0.0 )
	{
		float  rough = Max( 0.05, roughness );

		float  D;
		{
			float alpha = rough * rough;
			float alpha2 = alpha * alpha;
			float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
			D = (alpha2) / (Pi() * denom*denom); 
		}

		float  G;
		{
			float r = (rough + 1.0);
			float k = (r*r) / 8.0;
			float GL = dotNL / (dotNL * (1.0 - k) + k);
			float GV = dotNV / (dotNV * (1.0 - k) + k);
			G = GL * GV;
		}

		float3 F;
		{
			float3 F0 = Lerp( float3(0.04), albedo, metallic );
			F = F0 + (1.0 - F0) * Pow(1.0 - dotNV, 5.0); 
		}

		float3 spec = D * F * G / (4.0 * dotNL * dotNV);

		color += spec * dotNL * lightColor;
	}

	return color;
}
