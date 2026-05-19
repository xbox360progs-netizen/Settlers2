#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>

namespace Graphics {

class Light;

struct ShadowMapInfo {
    int index;
    int width;
    int height;
    D3DFORMAT format;
    bool isDirectional;
    D3DXMATRIX viewMatrix;
    D3DXMATRIX projectionMatrix;
    D3DXVECTOR3 lightPosition;
    D3DXVECTOR3 lightDirection;
    float shadowBias;
    float shadowRadius;
};

class ShadowSystem {
public:
    ShadowSystem();
    ~ShadowSystem();

    void Initialize(LPDIRECT3DDEVICE9 pDevice, int shadowMapSize = 1024);
    void Shutdown();

    void CreateDirectionalShadowMap(int size = 1024);
    void CreatePointLightShadowMap(Light* pLight, int size = 512);
    void CreateSpotLightShadowMap(Light* pLight, int size = 512);

    void SetShadowMapSize(int size) { m_shadowMapSize = size; }
    int GetShadowMapSize() const { return m_shadowMapSize; }

    void SetShadowBias(float bias) { m_shadowBias = bias; }
    float GetShadowBias() const { return m_shadowBias; }

    void BeginShadowPass(Light* pLight);
    void EndShadowPass();

    void BeginDirectionalShadowPass();
    void EndDirectionalShadowPass();

    LPDIRECT3DTEXTURE9 GetShadowMapTexture(int index);
    LPDIRECT3DSURFACE9 GetShadowMapSurface(int index);
    LPDIRECT3DSURFACE9 GetShadowMapDepthSurface(int index);

    const D3DXMATRIX& GetShadowMatrix(int index) const;
    const D3DXMATRIX& GetShadowViewMatrix(int index) const;
    const D3DXMATRIX& GetShadowProjectionMatrix(int index) const;

    int GetShadowMapCount() const { return (int)m_shadowMaps.size(); }

    void SetCascadedShadowCount(int count) { m_cascadeCount = count; }
    int GetCascadedShadowCount() const { return m_cascadeCount; }

    void SetDirectionalLight(const D3DXVECTOR3& direction, const D3DXVECTOR3& lightPos);

    void SetDebugDraw(bool debug) { m_debugDraw = debug; }
    bool IsDebugDrawEnabled() const { return m_debugDraw; }

    void RenderDebugInfo();

    float GetShadowAttenuation(const D3DXVECTOR3& worldPos, int shadowMapIndex);

private:
    LPDIRECT3DDEVICE9 m_pDevice;
    int m_shadowMapSize;
    float m_shadowBias;
    bool m_debugDraw;
    int m_cascadeCount;

    struct ShadowMapData {
        LPDIRECT3DTEXTURE9 pTexture;
        LPDIRECT3DSURFACE9 pSurface;
        LPDIRECT3DSURFACE9 pDepthSurface;
        D3DXMATRIX viewMatrix;
        D3DXMATRIX projectionMatrix;
        D3DXMATRIX viewProjectionMatrix;
        bool isDirectional;
        D3DXVECTOR3 position;
        D3DXVECTOR3 direction;
        float radius;
    };

    std::vector<ShadowMapData> m_shadowMaps;

    void CreateShadowMapInternal(int width, int height, ShadowMapData& outData);
    void DestroyShadowMap(ShadowMapData& data);

    void UpdateDirectionalShadowMatrices(int index, const D3DXVECTOR3& lightDir);
    void UpdatePointShadowMatrices(int index, const D3DXVECTOR3& lightPos);
    void UpdateSpotShadowMatrices(int index, const D3DXVECTOR3& lightPos, const D3DXVECTOR3& lightDir);
};

}