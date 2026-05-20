#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>

namespace Graphics {

struct DecalDesc {
    D3DXVECTOR3 position;
    D3DXVECTOR3 normal;
    D3DXVECTOR2 size;
    LPDIRECT3DTEXTURE9 texture;
    float lifetime;
    float fadeStart;

    DecalDesc() : position(0,0,0), normal(0,1,0), size(1,1), texture(NULL),
                  lifetime(0), fadeStart(0) {}
};

class DecalSystem {
public:
    DecalSystem();
    ~DecalSystem();

    void Initialize(LPDIRECT3DDEVICE9 pDevice);
    void Shutdown();

    void AddDecal(const DecalDesc& decal);
    void RemoveDecal(int index);
    void Clear();

    void Update(float deltaTime);

    int GetDecalCount() const { return (int)m_decals.size(); }

    void SetMaxDecals(int max) { m_maxDecals = max; }
    int GetMaxDecals() const { return m_maxDecals; }

    void Render(IDirect3DSurface9* target);

    void SetDebugDraw(bool debug) { m_debugDraw = debug; }

private:
    LPDIRECT3DDEVICE9 m_pDevice;
    std::vector<DecalDesc> m_decals;
    int m_maxDecals;
    bool m_debugDraw;
    bool m_enabled;

    LPDIRECT3DVERTEXBUFFER9 m_pQuadVB;
    LPDIRECT3DINDEXBUFFER9 m_pQuadIB;
    LPDIRECT3DVERTEXDECLARATION9 m_pQuadDecl;

    void CreateQuadMesh();
    void RenderDecal(const DecalDesc& decal);
};

struct VolumetricLightDesc {
    D3DXVECTOR3 position;
    D3DXVECTOR3 direction;
    float radius;
    float length;
    float angle;
    D3DXVECTOR3 color;
    float intensity;
    bool isSpot;
    bool enabled;

    VolumetricLightDesc() : position(0,0,0), direction(0,-1,0), radius(1), length(10),
                            angle(0.5f), color(1,1,1), intensity(1), isSpot(false), enabled(true) {}
};

class VolumetricLightSystem {
public:
    VolumetricLightSystem();
    ~VolumetricLightSystem();

    void Initialize(LPDIRECT3DDEVICE9 pDevice);
    void Shutdown();

    void AddVolumetricLight(const VolumetricLightDesc& light);
    void RemoveVolumetricLight(int index);
    void Clear();

    void Update(float deltaTime);

    int GetLightCount() const { return (int)m_lights.size(); }

    void SetEnabled(bool enable) { m_enabled = enable; }
    bool IsEnabled() const { return m_enabled; }

    void Render(IDirect3DTexture9* input, IDirect3DSurface9* output);

private:
    LPDIRECT3DDEVICE9 m_pDevice;
    std::vector<VolumetricLightDesc> m_lights;
    bool m_enabled;
    bool m_debugDraw;

    LPDIRECT3DTEXTURE9 m_pScatterTexture;
    LPDIRECT3DSURFACE9 m_pScatterSurface;

    void CreateRenderTargets();
    void ComputeScattering(const VolumetricLightDesc& light);
};

class WaterReflectionSystem {
public:
    WaterReflectionSystem();
    ~WaterReflectionSystem();

    void Initialize(LPDIRECT3DDEVICE9 pDevice, int width, int height);
    void Shutdown();

    void SetReflectionTexture(LPDIRECT3DTEXTURE9 tex) { m_pReflectionTexture = tex; }
    LPDIRECT3DTEXTURE9 GetReflectionTexture() const { return m_pReflectionTexture; }

    void SetWaterHeight(float height) { m_waterHeight = height; }
    float GetWaterHeight() const { return m_waterHeight; }

    void RenderReflection();
    void ApplyReflection(IDirect3DTexture9* input, IDirect3DSurface9* output);

    void SetReflectionEnabled(bool enable) { m_reflectionEnabled = enable; }
    bool IsReflectionEnabled() const { return m_reflectionEnabled; }

private:
    LPDIRECT3DDEVICE9 m_pDevice;
    float m_waterHeight;
    bool m_reflectionEnabled;
    bool m_debugDraw;

    LPDIRECT3DTEXTURE9 m_pReflectionTexture;
    LPDIRECT3DSURFACE9 m_pReflectionSurface;
    LPDIRECT3DSURFACE9 m_pReflectionDepth;

    LPDIRECT3DVERTEXBUFFER9 m_pQuadVB;
    LPDIRECT3DINDEXBUFFER9 m_pQuadIB;
    LPDIRECT3DVERTEXDECLARATION9 m_pQuadDecl;

    int m_width;
    int m_height;

    void CreateRenderTargets();
    void CreateQuadMesh();
};

}