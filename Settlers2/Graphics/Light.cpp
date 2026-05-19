#include "stdafx.h"
#include "Light.h"
#include <math.h>

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

Light::Light() 
    : m_position(0, 0, 0), m_direction(0, -1, 0), m_color(1, 1, 1),
      m_intensity(1.0f), m_radius(10.0f), 
      m_innerAngle(0.3f), m_outerAngle(0.8f),
      m_type(LIGHT_DIRECTIONAL), m_enabled(true),
      m_shadowCaster(false), m_shadowMapIndex(-1) {
}

Light::Light(const LightDesc& desc) 
    : m_position(desc.position), m_direction(desc.direction), m_color(desc.color),
      m_intensity(desc.intensity), m_radius(desc.radius),
      m_innerAngle(desc.innerAngle), m_outerAngle(desc.outerAngle),
      m_type(desc.type), m_enabled(desc.enabled),
      m_shadowCaster(false), m_shadowMapIndex(-1) {
}

Light::~Light() {
}

bool Light::IsInRange(const D3DXVECTOR3& point) const {
    if (m_type == LIGHT_DIRECTIONAL) {
        return true;
    }

    D3DXVECTOR3 diff = point - m_position;
    float distSq = D3DXVec3LengthSq(&diff);
    return distSq <= m_radius * m_radius;
}

float Light::GetAttenuation(const D3DXVECTOR3& point) const {
    if (m_type == LIGHT_DIRECTIONAL) {
        return 1.0f;
    }

    D3DXVECTOR3 diff = point - m_position;
    float dist = D3DXVec3Length(&diff);

    float atten = 1.0f - (dist / m_radius);
    atten = max(0.0f, atten);
    atten = atten * atten;

    return atten;
}

float Light::GetSpotFactor(const D3DXVECTOR3& point) const {
    if (m_type != LIGHT_SPOT) {
        return 1.0f;
    }

    D3DXVECTOR3 toLight = m_position - point;
    D3DXVec3Normalize(&toLight, &toLight);

    float cosAngle = D3DXVec3Dot(&toLight, &m_direction);

    float innerCos = cosf(m_innerAngle);
    float outerCos = cosf(m_outerAngle);

    if (cosAngle < outerCos) {
        return 0.0f;
    }

    if (cosAngle > innerCos) {
        return 1.0f;
    }

    float t = (cosAngle - outerCos) / (innerCos - outerCos);
    return t * t;
}

LightManager::LightManager()
    : m_pDirectionalLight(NULL), m_maxPointLights(16), m_maxSpotLights(8) {
    m_ambientLight = D3DXVECTOR3(0.1f, 0.1f, 0.15f);
}

LightManager::~LightManager() {
    RemoveAll();
}

Light* LightManager::CreateLightInternal(LightType type, const char* name) {
    Light* pLight = new Light();
    pLight->SetType(type);
    if (name) {
        pLight->SetName(name);
    }
    return pLight;
}

Light* LightManager::CreateLight(LightType type, const char* name) {
    Light* pLight = CreateLightInternal(type, name);
    m_lights.push_back(pLight);

    if (type == LIGHT_DIRECTIONAL && !m_pDirectionalLight) {
        m_pDirectionalLight = pLight;
    }

    return pLight;
}

Light* LightManager::CreateDirectionalLight(const D3DXVECTOR3& direction, const D3DXVECTOR3& color, float intensity) {
    Light* pLight = CreateLight(LIGHT_DIRECTIONAL, "Directional");
    if (pLight) {
        D3DXVECTOR3 dir = direction;
        D3DXVec3Normalize(&dir, &dir);
        pLight->SetDirection(dir);
        pLight->SetColor(color);
        pLight->SetIntensity(intensity);
    }
    return pLight;
}

Light* LightManager::CreatePointLight(const D3DXVECTOR3& position, const D3DXVECTOR3& color, float intensity, float radius) {
    if (GetLightCount() >= m_maxPointLights) {
        char buf[128];
        sprintf(buf, "[LightManager] WARNING: Max point lights (%d) reached!\n", m_maxPointLights);
        OutputDebugStringA(buf);
        return NULL;
    }

    Light* pLight = CreateLight(LIGHT_POINT, NULL);
    if (pLight) {
        pLight->SetPosition(position);
        pLight->SetColor(color);
        pLight->SetIntensity(intensity);
        pLight->SetRadius(radius);
    }
    return pLight;
}

Light* LightManager::CreateSpotLight(const D3DXVECTOR3& position, const D3DXVECTOR3& direction,
                                    const D3DXVECTOR3& color, float intensity, float radius,
                                    float innerAngle, float outerAngle) {
    if (GetLightCount() >= m_maxSpotLights) {
        char buf[128];
        sprintf(buf, "[LightManager] WARNING: Max spot lights (%d) reached!\n", m_maxSpotLights);
        OutputDebugStringA(buf);
        return NULL;
    }

    Light* pLight = CreateLight(LIGHT_SPOT, NULL);
    if (pLight) {
        pLight->SetPosition(position);
        D3DXVECTOR3 dir = direction;
        D3DXVec3Normalize(&dir, &dir);
        pLight->SetDirection(dir);
        pLight->SetColor(color);
        pLight->SetIntensity(intensity);
        pLight->SetRadius(radius);
        pLight->SetSpotAngles(innerAngle, outerAngle);
    }
    return pLight;
}

void LightManager::RemoveLight(Light* pLight) {
    for (size_t i = 0; i < m_lights.size(); i++) {
        if (m_lights[i] == pLight) {
            if (pLight == m_pDirectionalLight) {
                m_pDirectionalLight = NULL;
            }
            delete pLight;
            m_lights.erase(m_lights.begin() + i);
            return;
        }
    }
}

void LightManager::RemoveLight(int index) {
    if (index >= 0 && index < (int)m_lights.size()) {
        if (m_lights[index] == m_pDirectionalLight) {
            m_pDirectionalLight = NULL;
        }
        delete m_lights[index];
        m_lights.erase(m_lights.begin() + index);
    }
}

void LightManager::RemoveAll() {
    for (size_t i = 0; i < m_lights.size(); i++) {
        delete m_lights[i];
    }
    m_lights.clear();
    m_pDirectionalLight = NULL;
}

Light* LightManager::GetLight(int index) {
    if (index >= 0 && index < (int)m_lights.size()) {
        return m_lights[index];
    }
    return NULL;
}

int LightManager::GetLightCount() const {
    return (int)m_lights.size();
}

int LightManager::GetEnabledLightCount() const {
    int count = 0;
    for (size_t i = 0; i < m_lights.size(); i++) {
        if (m_lights[i]->IsEnabled()) {
            count++;
        }
    }
    return count;
}

void LightManager::Update(float deltaTime) {
    for (size_t i = 0; i < m_lights.size(); i++) {
    }
}

Light* LightManager::GetDirectionalLight() {
    return m_pDirectionalLight;
}

void LightManager::GetPointLights(std::vector<Light*>& outLights) {
    outLights.clear();
    for (size_t i = 0; i < m_lights.size(); i++) {
        if (m_lights[i]->GetType() == LIGHT_POINT && m_lights[i]->IsEnabled()) {
            outLights.push_back(m_lights[i]);
        }
    }
}

void LightManager::GetSpotLights(std::vector<Light*>& outLights) {
    outLights.clear();
    for (size_t i = 0; i < m_lights.size(); i++) {
        if (m_lights[i]->GetType() == LIGHT_SPOT && m_lights[i]->IsEnabled()) {
            outLights.push_back(m_lights[i]);
        }
    }
}

}