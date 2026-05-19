#pragma once
#include <d3d9.h>
#include <d3dx9.h>

namespace Graphics {

class SSAOSystem {
public:
    SSAOSystem();
    ~SSAOSystem();

    void Initialize(LPDIRECT3DDEVICE9 pDevice, int screenWidth, int screenHeight);
    void Shutdown();

    void SetEnabled(bool enable) { m_enabled = enable; }
    bool IsEnabled() const { return m_enabled; }

    void SetRadius(float radius) { m_radius = radius; }
    float GetRadius() const { return m_radius; }

    void SetBias(float bias) { m_bias = bias; }
    float GetBias() const { return m_bias; }

    void SetIntensity(float intensity) { m_intensity = intensity; }
    float GetIntensity() const { return m_intensity; }

    void SetSampleCount(int count) { m_sampleCount = count; }
    int GetSampleCount() const { return m_sampleCount; }

    void Render(IDirect3DSurface9* normalRT, IDirect3DSurface9* depthRT);

    LPDIRECT3DTEXTURE9 GetAOMap() { return m_pAOMap; }
    LPDIRECT3DSURFACE9 GetAOSurface() { return m_pAOSurface; }

    void SetDebugView(bool debug) { m_debugView = debug; }
    bool IsDebugViewEnabled() const { return m_debugView; }

private:
    LPDIRECT3DDEVICE9 m_pDevice;
    bool m_enabled;
    bool m_debugView;

    int m_screenWidth;
    int m_screenHeight;
    float m_radius;
    float m_bias;
    float m_intensity;
    int m_sampleCount;

    LPDIRECT3DTEXTURE9 m_pAOMap;
    LPDIRECT3DSURFACE9 m_pAOSurface;
    LPDIRECT3DSURFACE9 m_pTempSurface;

    LPDIRECT3DVERTEXBUFFER9 m_pQuadVB;
    LPDIRECT3DINDEXBUFFER9 m_pQuadIB;
    LPDIRECT3DVERTEXDECLARATION9 m_pQuadDecl;

    void CreateAOMap();
    void DestroyAOMap();
    void CreateQuadMesh();

    void BlurAOMap();
};

}