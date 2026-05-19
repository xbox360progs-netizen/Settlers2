#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>

namespace Graphics {

struct SpriteShadowData {
    D3DXVECTOR3 worldPosition;
    float heightOffset;
    D3DXVECTOR2 scale;
    float intensity;
    float blurRadius;
    bool receiveShadows;
};

class SpriteShadowSystem {
public:
    SpriteShadowSystem();
    ~SpriteShadowSystem();

    void Initialize(IDirect3DDevice9* pDevice, int shadowMapSize = 512);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void AddSpriteShadow(const SpriteShadowData& shadow);
    void ClearShadowList();

    void Render(IDirect3DDevice9* pDevice, IDirect3DTexture9* shadowMap, 
                const D3DXMATRIX& viewProj);

    void SetLightDirection(const D3DXVECTOR3& dir) { m_lightDirection = dir; }
    const D3DXVECTOR3& GetLightDirection() const { return m_lightDirection; }

    void SetLightHeight(float height) { m_lightHeight = height; }
    float GetLightHeight() const { return m_lightHeight; }

    void SetShadowIntensity(float intensity) { m_shadowIntensity = intensity; }
    float GetShadowIntensity() const { return m_shadowIntensity; }

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void SetBlurEnabled(bool enabled) { m_blurEnabled = enabled; }
    bool IsBlurEnabled() const { return m_blurEnabled; }

private:
    IDirect3DDevice9* m_pDevice;
    int m_shadowMapSize;
    bool m_initialized;
    bool m_enabled;
    bool m_blurEnabled;
    D3DXVECTOR3 m_lightDirection;
    float m_lightHeight;
    float m_shadowIntensity;

    std::vector<SpriteShadowData> m_spriteShadows;

    IDirect3DTexture9* m_pShadowBlurTexture;
    IDirect3DSurface9* m_pShadowBlurSurface;

    void CreateBlurResources();
    void ReleaseBlurResources();
    void RenderProjectedShadows(IDirect3DDevice9* pDevice);
    void RenderBlurPass(IDirect3DDevice9* pDevice);
};

class ProjectedShadowGenerator {
public:
    ProjectedShadowGenerator();
    ~ProjectedShadowGenerator();

    void Initialize(IDirect3DDevice9* pDevice);
    void Shutdown();

    void SetProjectorMatrix(const D3DXMATRIX& matrix);
    void SetShadowTexture(IDirect3DTexture9* texture);
    void SetColor(D3DXVECTOR4 color);
    void SetSoftness(float softness);

    void Render(IDirect3DDevice9* pDevice);

private:
    IDirect3DDevice9* m_pDevice;
    bool m_initialized;
    D3DXMATRIX m_projectorMatrix;
    IDirect3DTexture9* m_pShadowTexture;
    D3DXVECTOR4 m_color;
    float m_softness;
};

class CascadedShadowMap {
public:
    CascadedShadowMap();
    ~CascadedShadowMap();

    void Initialize(IDirect3DDevice9* pDevice, int mapSize, int cascadeCount);
    void Shutdown();

    void UpdateCascades(const D3DXVECTOR3& lightDir, 
                       const D3DXMATRIX& viewProj,
                       float farPlaneDistance);

    IDirect3DTexture9* GetCascadeTexture(int index) const { return m_cascadeTextures[index]; }
    const D3DXMATRIX& GetCascadeMatrix(int index) const { return m_cascadeMatrices[index]; }
    float GetCascadeSplit(int index) const { return m_cascadeSplits[index]; }

    int GetCascadeCount() const { return m_cascadeCount; }

private:
    IDirect3DDevice9* m_pDevice;
    int m_mapSize;
    int m_cascadeCount;
    bool m_initialized;

    std::vector<IDirect3DTexture9*> m_cascadeTextures;
    std::vector<IDirect3DSURFACE9*> m_cascadeSurfaces;
    std::vector<D3DXMATRIX> m_cascadeMatrices;
    std::vector<float> m_cascadeSplits;

    void CreateCascadeResources();
    void ReleaseCascadeResources();
};

}