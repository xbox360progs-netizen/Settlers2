#include "stdafx.h"
#include "AdvancedEffects.h"
#include <math.h>

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

// === DecalSystem ===

DecalSystem::DecalSystem()
    : m_pDevice(NULL), m_maxDecals(64), m_debugDraw(false),
      m_pQuadVB(NULL), m_pQuadIB(NULL), m_pQuadDecl(NULL) {
}

DecalSystem::~DecalSystem() {
    Shutdown();
}

void DecalSystem::Initialize(LPDIRECT3DDEVICE9 pDevice) {
    m_pDevice = pDevice;
    CreateQuadMesh();
    OutputDebugStringA("[DecalSystem] Initialized\n");
}

void DecalSystem::Shutdown() {
    Clear();

    if (m_pQuadVB) { m_pQuadVB->Release(); m_pQuadVB = NULL; }
    if (m_pQuadIB) { m_pQuadIB->Release(); m_pQuadIB = NULL; }
    if (m_pQuadDecl) { m_pQuadDecl->Release(); m_pQuadDecl = NULL; }

    OutputDebugStringA("[DecalSystem] Shutdown complete\n");
}

void DecalSystem::CreateQuadMesh() {
    if (!m_pDevice) return;

    struct Vertex {
        float x, y, z;
        float u, v;
    };

    Vertex verts[4] = {
        { -1, 0, -1, 0, 1 },
        {  1, 0, -1, 1, 1 },
        {  1, 0,  1, 1, 0 },
        { -1, 0,  1, 0, 0 }
    };

    WORD indices[6] = { 0, 1, 2, 0, 2, 3 };

    m_pDevice->CreateVertexBuffer(4 * sizeof(Vertex), 0, 0, D3DPOOL_DEFAULT, &m_pQuadVB, NULL);
    m_pDevice->CreateIndexBuffer(6 * sizeof(WORD), 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_pQuadIB, NULL);

    void* pData;
    m_pQuadVB->Lock(0, 0, &pData, 0);
    memcpy(pData, verts, sizeof(verts));
    m_pQuadVB->Unlock();

    m_pQuadIB->Lock(0, 0, &pData, 0);
    memcpy(pData, indices, sizeof(indices));
    m_pQuadIB->Unlock();

    D3DVERTEXELEMENT9 decl[] = {
        { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };

    m_pDevice->CreateVertexDeclaration(decl, &m_pQuadDecl);
}

void DecalSystem::AddDecal(const DecalDesc& decal) {
    if ((int)m_decals.size() >= m_maxDecals) {
        OutputDebugStringA("[DecalSystem] WARNING: Max decals reached\n");
        return;
    }
    m_decals.push_back(decal);
}

void DecalSystem::RemoveDecal(int index) {
    if (index >= 0 && index < (int)m_decals.size()) {
        m_decals.erase(m_decals.begin() + index);
    }
}

void DecalSystem::Clear() {
    m_decals.clear();
}

void DecalSystem::Update(float deltaTime) {
    for (int i = (int)m_decals.size() - 1; i >= 0; i--) {
        DecalDesc& decal = m_decals[i];

        if (decal.lifetime > 0) {
            decal.lifetime -= deltaTime;

            if (decal.lifetime <= decal.fadeStart) {
                float alpha = decal.lifetime / decal.fadeStart;
                if (alpha < 0) alpha = 0;
            }

            if (decal.lifetime <= 0) {
                RemoveDecal(i);
            }
        }
    }
}

void DecalSystem::RenderDecal(const DecalDesc& decal) {
    if (!m_pDevice || !decal.texture || !m_pQuadVB || !m_pQuadIB) return;

    m_pDevice->SetTexture(0, decal.texture);
    m_pDevice->SetStreamSource(0, m_pQuadVB, 0, sizeof(float) * 5);
    m_pDevice->SetIndices(m_pQuadIB);

    if (m_pQuadDecl) {
        m_pDevice->SetVertexDeclaration(m_pQuadDecl);
    }

    m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2);
}

void DecalSystem::Render(IDirect3DSurface9* target) {
    if (!m_enabled || m_decals.empty()) return;

    for (size_t i = 0; i < m_decals.size(); i++) {
        RenderDecal(m_decals[i]);
    }
}

// === VolumetricLightSystem ===

VolumetricLightSystem::VolumetricLightSystem()
    : m_pDevice(NULL), m_enabled(false), m_debugDraw(false),
      m_pScatterTexture(NULL), m_pScatterSurface(NULL) {
}

VolumetricLightSystem::~VolumetricLightSystem() {
    Shutdown();
}

void VolumetricLightSystem::Initialize(LPDIRECT3DDEVICE9 pDevice) {
    m_pDevice = pDevice;
    CreateRenderTargets();
    OutputDebugStringA("[VolumetricLight] Initialized\n");
}

void VolumetricLightSystem::Shutdown() {
    Clear();

    if (m_pScatterSurface) { m_pScatterSurface->Release(); m_pScatterSurface = NULL; }
    if (m_pScatterTexture) { m_pScatterTexture->Release(); m_pScatterTexture = NULL; }

    OutputDebugStringA("[VolumetricLight] Shutdown complete\n");
}

void VolumetricLightSystem::CreateRenderTargets() {
    if (!m_pDevice) return;

    HRESULT hr = m_pDevice->CreateTexture(
        256, 256, 1, D3DUSAGE_RENDERTARGET,
        D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
        &m_pScatterTexture, NULL
    );

    if (SUCCEEDED(hr) && m_pScatterTexture) {
        m_pScatterTexture->GetSurfaceLevel(0, &m_pScatterSurface);
    }
}

void VolumetricLightSystem::AddVolumetricLight(const VolumetricLightDesc& light) {
    m_lights.push_back(light);
}

void VolumetricLightSystem::RemoveVolumetricLight(int index) {
    if (index >= 0 && index < (int)m_lights.size()) {
        m_lights.erase(m_lights.begin() + index);
    }
}

void VolumetricLightSystem::Clear() {
    m_lights.clear();
}

void VolumetricLightSystem::Update(float deltaTime) {
    for (size_t i = 0; i < m_lights.size(); i++) {
        if (!m_lights[i].enabled) continue;
    }
}

void VolumetricLightSystem::ComputeScattering(const VolumetricLightDesc& light) {
}

void VolumetricLightSystem::Render(IDirect3DTexture9* input, IDirect3DSurface9* output) {
    if (!m_enabled || m_lights.empty()) return;

    OutputDebugStringA("[VolumetricLight] Rendering volumetric light\n");
}

// === WaterReflectionSystem ===

WaterReflectionSystem::WaterReflectionSystem()
    : m_pDevice(NULL), m_waterHeight(0.0f), m_reflectionEnabled(false),
      m_debugDraw(false), m_pReflectionTexture(NULL), m_pReflectionSurface(NULL),
      m_pReflectionDepth(NULL), m_pQuadVB(NULL), m_pQuadIB(NULL),
      m_pQuadDecl(NULL), m_width(512), m_height(512) {
}

WaterReflectionSystem::~WaterReflectionSystem() {
    Shutdown();
}

void WaterReflectionSystem::Initialize(LPDIRECT3DDEVICE9 pDevice, int width, int height) {
    m_pDevice = pDevice;
    m_width = width;
    m_height = height;

    CreateRenderTargets();
    CreateQuadMesh();

    OutputDebugStringA("[WaterReflection] Initialized\n");
}

void WaterReflectionSystem::Shutdown() {
    if (m_pReflectionDepth) { m_pReflectionDepth->Release(); m_pReflectionDepth = NULL; }
    if (m_pReflectionSurface) { m_pReflectionSurface->Release(); m_pReflectionSurface = NULL; }
    if (m_pReflectionTexture) { m_pReflectionTexture->Release(); m_pReflectionTexture = NULL; }

    if (m_pQuadVB) { m_pQuadVB->Release(); m_pQuadVB = NULL; }
    if (m_pQuadIB) { m_pQuadIB->Release(); m_pQuadIB = NULL; }
    if (m_pQuadDecl) { m_pQuadDecl->Release(); m_pQuadDecl = NULL; }

    OutputDebugStringA("[WaterReflection] Shutdown complete\n");
}

void WaterReflectionSystem::CreateRenderTargets() {
    if (!m_pDevice) return;

    HRESULT hr = m_pDevice->CreateTexture(
        m_width, m_height, 1, D3DUSAGE_RENDERTARGET,
        D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
        &m_pReflectionTexture, NULL
    );

    if (SUCCEEDED(hr) && m_pReflectionTexture) {
        m_pReflectionTexture->GetSurfaceLevel(0, &m_pReflectionSurface);
    }

    hr = m_pDevice->CreateDepthStencilSurface(
        m_width, m_height, D3DFMT_D16,
        D3DMULTISAMPLE_NONE, 0, FALSE,
        &m_pReflectionDepth, NULL
    );
}

void WaterReflectionSystem::CreateQuadMesh() {
    if (!m_pDevice) return;

    struct Vertex {
        float x, y, z;
        float u, v;
    };

    Vertex verts[4] = {
        { -1, -1, 0, 0, 1 },
        {  1, -1, 0, 1, 1 },
        {  1,  1, 0, 1, 0 },
        { -1,  1, 0, 0, 0 }
    };

    WORD indices[6] = { 0, 1, 2, 0, 2, 3 };

    m_pDevice->CreateVertexBuffer(4 * sizeof(Vertex), 0, 0, D3DPOOL_DEFAULT, &m_pQuadVB, NULL);
    m_pDevice->CreateIndexBuffer(6 * sizeof(WORD), 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_pQuadIB, NULL);

    void* pData;
    m_pQuadVB->Lock(0, 0, &pData, 0);
    memcpy(pData, verts, sizeof(verts));
    m_pQuadVB->Unlock();

    m_pQuadIB->Lock(0, 0, &pData, 0);
    memcpy(pData, indices, sizeof(indices));
    m_pQuadIB->Unlock();

    D3DVERTEXELEMENT9 decl[] = {
        { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };

    m_pDevice->CreateVertexDeclaration(decl, &m_pQuadDecl);
}

void WaterReflectionSystem::RenderReflection() {
    if (!m_reflectionEnabled || !m_pDevice) return;

    if (m_pReflectionSurface && m_pReflectionDepth) {
        m_pDevice->SetRenderTarget(0, m_pReflectionSurface);
        m_pDevice->SetDepthStencilSurface(m_pReflectionDepth);
        m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 100, 150), 1.0f, 0);
    }

    OutputDebugStringA("[WaterReflection] Rendering reflection\n");
}

void WaterReflectionSystem::ApplyReflection(IDirect3DTexture9* input, IDirect3DSurface9* output) {
    if (!m_reflectionEnabled || !m_pDevice || !input) return;

    if (output) {
        m_pDevice->StretchRect(input, NULL, output, NULL, D3DTEXF_LINEAR);
    }

    OutputDebugStringA("[WaterReflection] Applied\n");
}

}