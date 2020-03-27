#pragma once

#include "scene/Math/GLM.h"

namespace FG
{

	struct UniformSunObject {
	
		glm::vec4 location; // only really used for the procedural scattering. regard this as a directional light
		glm::vec4 direction; 
		glm::vec4 color;
		glm::mat4 directionBasis; // Equivalent to TBN, for transforming cone samples in ray marcher
		float intensity;
	};

	struct UniformSkyObject{
	
		// many other constants are stored in the sky manager, minimizing the amount of stuff transferred to shader

		// precalculated, depends on sun
		glm::vec4 betaR;
		glm::vec4 betaV;
		glm::vec4 wind;
		float mie_directional;
	};

	class SkyManager
	{
	private:
		float _elevation, _azimuth;
		float _turbidity;
		float _rayleigh;
		float _mie;
		UniformSkyObject _sky;
		UniformSunObject _sun;
		void calcSunPosition();
		void calcSunIntensity();
		void calcSunColor();

		void calcSkyBetaR();
		void calcSkyBetaV();

	public:
		SkyManager();
		~SkyManager();
		void rebuildSkyFromNewSun(float elevation, float azimuth);
		void rebuildSkyFromScattering(float turbidity, float mie, float mie_directional);
		void rebuildSky(float elevation, float azimuth, float turbidity, float mie, float mie_directional);
		void setWindDirection(const glm::vec3 dir) { _sky.wind = glm::vec4(dir, _sky.wind.w); }
		void setTime(float t) { _sky.wind.w = t; }
		UniformSunObject& getSun() { return _sun; }
		UniformSkyObject getSky() { return _sky; }
	};

}   // FG
