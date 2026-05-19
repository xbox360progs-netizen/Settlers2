#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include <string>

namespace Graphics {

enum LightType {
    LIGHT_DIRECTIONAL = 0,
    LIGHT_POINT = 1,
    LIGHT_SPOT = 2
};

struct LightDesc {
    D3DXVECTOR3 position;
    D3DXVECTOR3 direction;
    D3DXVECTOR3 color;
    float intensity;
    float radius;
    float innerAngle;
    float outerAngle;
    LightType type;
    bool enabled;

    LightDesc()
        : position(0, 0, 0), direction(0, -1, 0), color(1, 1, 1),
          intensity(1.0f), radius(10.0f), 
          innerAngle(0.3f), outerAngle(0.8f),
          type(LIGHT_DIRECTIONAL), enabled(true) {}

    LightDesc(LightType t, const D3DXVECTOR3& pos, const D3DXVECTOR3& col, float intensity_)
        : position(pos), direction(0, -1, 0), color(col),
          intensity(intensity_), radius(10.0f),
          innerAngle(0.3f), outerAngle(0.8f),
          type(t), enabled(true) {}
};

class Light {
public:
    Light();
    Light(const LightDesc& desc);
    ~Light();

    void SetPosition(const D3DXVECTOR3& pos) { m_position = pos; }
    D3DXVECTOR3 GetPosition() const { return m_position; }

    void SetDirection(const D3DXVECTOR3& dir) { m_direction = dir; }
    D3DXVECTOR3 GetDirection() const { return m_direction; }

    void SetColor(const D3DXVECTOR3& col) { m_color = col; }
    D3DXVECTOR3 GetColor() const { return m_color; }

    void SetIntensity(float intensity) { m_intensity = intensity; }
    float GetIntensity() const { return m_intensity; }

    void SetRadius(float radius) { m_radius = radius; }
    float GetRadius() const { return m_radius; }

    void SetType(LightType type) { m_type = type; }
    LightType GetType() const { return m_type; }

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void SetSpotAngles(float inner, float outer) {
        m_innerAngle = inner;
        m_outerAngle = outer;
    }
    float GetInnerAngle() const { return m_innerAngle; }
    float GetOuterAngle() const { return m_outerAngle; }

    void SetName(const char* name) { m_name = name ? name : ""; }
    const char* GetName() const { return m_name.c_str(); }

    bool IsInRange(const D3DXVECTOR3& point) const;
    float GetAttenuation(const D3DXVECTOR3& point) const;
    float GetSpotFactor(const D3DXVECTOR3& point) const;

    void SetShadowCaster(bool caster) { m_shadowCaster = caster; }
    bool IsShadowCaster() const { return m_shadowCaster; }

    void SetShadowMapIndex(int index) { m_shadowMapIndex = index; }
    int GetShadowMapIndex() const { return m_shadowMapIndex; }

private:
    std::string m_name;
    D3DXVECTOR3 m_position;
    D3DXVECTOR3 m_direction;
    D3DXVECTOR3 m_color;
    float m_intensity;
    float m_radius;
    float m_innerAngle;
    float m_outerAngle;
    LightType m_type;
    bool m_enabled;
    bool m_shadowCaster;
    int m_shadowMapIndex;
};

class LightManager {
public:
    LightManager();
    ~LightManager();

    Light* CreateLight(LightType type, const char* name = NULL);
    Light* CreateDirectionalLight(const D3DXVECTOR3& direction, const D3DXVECTOR3& color, float intensity);
    Light* CreatePointLight(const D3DXVECTOR3& position, const D3DXVECTOR3& color, float intensity, float radius);
    Light* CreateSpotLight(const D3DXVECTOR3& position, const D3DXVECTOR3& direction,
                          const D3DXVECTOR3& color, float intensity, float radius, float innerAngle, float outerAngle);

    void RemoveLight(Light* pLight);
    void RemoveLight(int index);
    void RemoveAll();

    Light* GetLight(int index);
    int GetLightCount() const;
    int GetEnabledLightCount() const;

    void Update(float deltaTime);

    void SetAmbientLight(const D3DXVECTOR3& color) { m_ambientLight = color; }
    D3DXVECTOR3 GetAmbientLight() const { return m_ambientLight; }

    Light* GetDirectionalLight();
    void GetPointLights(std::vector<Light*>& outLights);
    void GetSpotLights(std::vector<Light*>& outLights);

    void SetMaxPointLights(int max) { m_maxPointLights = max; }
    int GetMaxPointLights() const { return m_maxPointLights; }

    void SetMaxSpotLights(int max) { m_maxSpotLights = max; }
    int GetMaxSpotLights() const { return m_maxSpotLights; }

private:
    std::vector<Light*> m_lights;
    Light* m_pDirectionalLight;
    D3DXVECTOR3 m_ambientLight;
    int m_maxPointLights;
    int m_maxSpotLights;

    Light* CreateLightInternal(LightType type, const char* name);
};

}