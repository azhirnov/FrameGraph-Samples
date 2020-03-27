#include "SkyManager.h"

namespace FG
{

    #define SHADOW_CUTOFF 1.6110731557f
    #define SHADOW_STEEPNESS 1.5f
    #define EE 1000.0f
    #define PI 3.14159265f
    #define E 2.718281828459f
    #define SUN_DISTANCE 400000.0f
    #define MIE_CONST glm::vec3( 1.839991851443397f, 2.779802391966052f, 4.079047954386109f)
    #define RAYLEIGH_TOTAL glm::vec3(5.804542996261093E-6, 1.3562911419845635E-5, 3.0265902468824876E-5)

    float clamp(float t, float min, float max) {
        return std::max(min, std::min(max, t));
    }

    void SkyManager::calcSunColor() {

        if (_sun.direction.y < 0.0f) {
            _sun.color = glm::vec4(0.8f, 0.9f, 1.0f, _sun.color.a);
        } else {
            glm::vec3 sunset = glm::vec3(2.f, 0.33922, 0.0431f);
            float t = (_sun.direction.y) * 13.f;
            t = clamp(t, 0.f, 1.f);
            glm::vec3 color = (1.f - t) * sunset + (t)* glm::vec3(1.f);
            _sun.color = glm::vec4(color, _sun.color.a);
        }
    }

    void SkyManager::calcSunIntensity() {
        float zenithAngleCos = clamp(_sun.direction.y, -1.f, 1.f);
        _sun.intensity = EE * std::max(0.f, 1.f - powf(E, -((SHADOW_CUTOFF - acosf(zenithAngleCos)) / SHADOW_STEEPNESS)));
        if (_sun.direction.y < 0.0f) {
            _sun.intensity = 2.0f;
        }
    }

    void SkyManager::calcSkyBetaR() {
        float sunFade = 1.0f - clamp(1.0f - exp(_sun.location.y / 450000.0f), 0.0f, 1.0f);
        _sky.betaR = glm::vec4(RAYLEIGH_TOTAL * (_rayleigh - 1.f + sunFade), 0.0f); // UBO padding
    }

    void SkyManager::calcSkyBetaV() {
        float c = (0.2f * _turbidity) * 10E-18f;
        _sky.betaV = glm::vec4(0.434f * c * MIE_CONST * _mie, 0.0f); // UBO padding
    }

    void SkyManager::calcSunPosition() {
        float theta = 2.0f * PI * (_elevation - 0.5f);
        float phi = 2.0f * PI * (_azimuth - 0.5f);
        //(cos(phi), sin(phi) * sin(theta), sin(phi) * cos(theta)); // double-check this


        glm::vec3 dir = glm::vec3(cos(phi), sin(phi) * sin(theta), sin(phi) * cos(theta));
        //glm::normalize(glm::vec3(std::cos(_elevation), cos(_azimuth) * sin(_elevation), sin(_azimuth) * cos(_elevation))); // not terribly realistic / standard, but...
        _sun.direction = glm::vec4(dir, 0.0f);
        //_sun.direction *= -1.f;
        _sun.location = glm::vec4(SUN_DISTANCE * dir, 1.0f); // assume center is 0 0 0 for sky
        if (_sun.direction.y < 0.0f) {
            _sun.location *= -1.0f;
        }
        _sun.directionBasis = glm::mat4(1);
        glm::vec3 right = (abs(_sun.direction.y) < 0.001f) ? ((_sun.direction.z < 0) ? glm::vec3(0, 1, 0) : glm::vec3(0, -1, 0)) : glm::vec3(0, 0, 1);
        glm::vec3 dirN = glm::normalize(glm::cross(dir, right));
        glm::vec3 dirB = glm::normalize(glm::cross(dir, dirN));
        _sun.directionBasis[1] = _sun.direction;
        _sun.directionBasis[0] = glm::vec4(dirN, 0.0f);
        _sun.directionBasis[2] = glm::vec4(dirB, 0.0f);
        if (_sun.direction.y < 0.0f) {
            _sun.directionBasis *= -1.0f;
        }
    }

    SkyManager::SkyManager()
    {
        _elevation = PI / 4.f; // 0 is sunrise, PI is sunset
        _azimuth = PI / 8.f; // here: tilt about x axis with y up. not realistic, but ok for 'seasonal tilt'
        //_sun = {};
        //_sky = {};
        _sun = {
            glm::vec4(0.f),
            glm::vec4(0.f),
            glm::vec4(0.f),
            glm::mat4(1.f),
            0.0f
        };
        _sky = {
            glm::vec4(0.f),
            glm::vec4(0.f),
            glm::vec4(1, 0.05, 1, 0),
            0.f,
        };
        _sun.color = glm::vec4(1, 1, 1, 0); // TODO. Note, alpha channel has to start at 0 as it serves a much different purpose
        calcSunPosition();
        calcSunColor();
        calcSunIntensity();
        _mie = 0.005f;
        _sky.mie_directional = 0.8f;
        _rayleigh = 2.f;
        calcSkyBetaR();
        calcSkyBetaV();
    }


    SkyManager::~SkyManager()
    {
    }


    void SkyManager::rebuildSkyFromNewSun(float elevation, float azimuth) {
        _elevation = elevation;
        _azimuth = azimuth;
        calcSunPosition();
        calcSunIntensity();
        calcSunColor();
        calcSkyBetaR();
        calcSkyBetaV();
    }

    void SkyManager::rebuildSkyFromScattering(float turbidity, float mie, float mie_directional) {
        _sky.mie_directional = mie_directional;
        _mie = mie;
        calcSkyBetaR();
        calcSkyBetaV();
    }

    void SkyManager::rebuildSky(float elevation, float azimuth, float turbidity, float mie, float mie_directional) {
        _elevation = elevation;
        _azimuth = azimuth;
        calcSunPosition();
        calcSunIntensity();
        calcSunColor();
        _sky.mie_directional = mie_directional;
        _mie = mie;
        calcSkyBetaR();
        calcSkyBetaV();
    }

}   // FG
