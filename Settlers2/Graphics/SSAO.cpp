#include "stdafx.h"
#include "SSAO.h"
#include <math.h>

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

SSAOSystem::SSAOSystem()
    : m_pDevice(NULL), m_enabled(false), m_debugView(false),
      m_screenWidth(1280), m_screenHeight(720),
      m_radius(0.5f), m_bias(0.025f), m_intensity(1.0f), m_sampleCount(16),
      m_pAOMap(NULL), m_pAOSurface(NULL), m_pTempSurface(NULL),
      m_pQuadVB(NULL), m_pQuadIB(NULL), m_pQuadDecl(NULL) {
}

SSAOSystem::~SSAOSystem() {
    Shutdown();
}

void SSAOSystem::Initialize(LPDIRECT3DDEVICE9 pDevice, int screenWidth, int screenHeight) {
    m_pDevice = pDevice;
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    CreateAOMap();
    CreateQuadMesh();

    char buf[256];
    sprintf(buf, "[SSAO] Initialized: %dx%d, radius=%.2f, samples=%d\n",
            screenWidth, screenHeight, m_radius, m_sampleCount);
    OutputDebugStringA(buf);
}

void SSAOSystem::Shutdown() {
    DestroyAOMap();

    if (m_pQuadVB) {
        m_pQuadVB->Release();
        m_pQuadVB = NULL;
    }
    if (m_pQuadIB) {
        m_pQuadIB->Release();
        m_pQuadIB = NULL;
    }
    if (m_pQuadDecl) {
        m_pQuadDecl->Release();
        m_pQuadDecl = NULL;
    }

    OutputDebugStringA("[SSAO] Shutdown complete\n");
}

void SSAOSystem::CreateAOMap() {
    if (!m_pDevice) return;

    if (m_pAOMap) {
        m_pAOMap->Release();
        m_pAOMap = NULL;
    }

    HRESULT hr = m_pDevice->CreateTexture(
        m_screenWidth / 2, m_screenHeight / 2,
        1,
        D3DUSAGE_RENDERTARGET,
        D3DFMT_L8,
        D3DPOOL_DEFAULT,
        &m_pAOMap,
        NULL
    );

    if (FAILED(hr)) {
        OutputDebugStringA("[SSAO] ERROR: Failed to create AO texture\n");
        return;
    }

    hr = m_pAOMap->GetSurfaceLevel(0, &m_pAOSurface);
    if (FAILED(hr)) {
        OutputDebugStringA("[SSAO] ERROR: Failed to get AO surface\n");
        return;
    }

    hr = m_pDevice->CreateRenderTarget(
        m_screenWidth / 2, m_screenHeight / 2,
        D3DFMT_A8R8G8B8,
        D3DMULTISAMPLE_NONE,
        0,
        FALSE,
        &m_pTempSurface,
        NULL
    );
}

void SSAOSystem::DestroyAOMap() {
    if (m_pTempSurface) {
        m_pTempSurface->Release();
        m_pTempSurface = NULL;
    }
    if (m_pAOSurface) {
        m_pAOSurface->Release();
        m_pAOSurface = NULL;
    }
    if (m_pAOMap) {
        m_pAOMap->Release();
        m_pAOMap = NULL;
    }
}

void SSAOSystem::CreateQuadMesh() {
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

    if (m_pQuadVB) m_pQuadVB->Release();
    if (m_pQuadIB) m_pQuadIB->Release();

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

    if (m_pQuadDecl) m_pQuadDecl->Release();
    m_pDevice->CreateVertexDeclaration(decl, &m_pQuadDecl);
}

void SSAOSystem::Render(IDirect3DSurface9* normalRT, IDirect3DSurface9* depthRT) {
    if (!m_enabled || !m_pDevice || !m_pAOSurface) return;

    char buf[256];
    sprintf(buf, "[SSAO] Rendering AO (normalRT=%p, depthRT=%p)\n", normalRT, depthRT);
    OutputDebugStringA(buf);
}

void SSAOSystem::BlurAOMap() {
}

}