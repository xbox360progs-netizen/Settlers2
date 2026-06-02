#include "stdafx.h"
#include "PostProcessing.h"
#include <math.h>

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

PostProcessingSystem::PostProcessingSystem()
    : m_pDevice(NULL), m_enabled(true), m_debugView(false),
      m_screenWidth(1280), m_screenHeight(720),
      m_bloomEnabled(true), m_toneMappingEnabled(true), m_colorGradingEnabled(false),
      m_pBloomExtractTexture(NULL), m_pBloomExtractSurface(NULL),
      m_pBloomBlurTexture1(NULL), m_pBloomBlurSurface1(NULL),
      m_pBloomBlurTexture2(NULL), m_pBloomBlurSurface2(NULL),
      m_pTempTexture(NULL), m_pTempSurface(NULL),
      m_pQuadVB(NULL), m_pQuadIB(NULL), m_pQuadDecl(NULL) {
}

PostProcessingSystem::~PostProcessingSystem() {
    Shutdown();
}

void PostProcessingSystem::Initialize(LPDIRECT3DDEVICE9 pDevice, int screenWidth, int screenHeight) {
    m_pDevice = pDevice;
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    CreateRenderTargets();
    CreateQuadMesh();

    OutputDebugStringA("[PostProcessing] Initialized\n");
}

void PostProcessingSystem::Shutdown() {
    DestroyRenderTargets();

    if (m_pQuadVB) { m_pQuadVB->Release(); m_pQuadVB = NULL; }
    if (m_pQuadIB) { m_pQuadIB->Release(); m_pQuadIB = NULL; }
    if (m_pQuadDecl) { m_pQuadDecl->Release(); m_pQuadDecl = NULL; }

    OutputDebugStringA("[PostProcessing] Shutdown complete\n");
}

void PostProcessingSystem::CreateRenderTargets() {
    if (!m_pDevice) return;

    int halfW = m_screenWidth / 2;
    int halfH = m_screenHeight / 2;

    auto CreateRT = [&](LPDIRECT3DTEXTURE9* ppTex, LPDIRECT3DSURFACE9* ppSurf, int w, int h, D3DFORMAT fmt) {
        if (*ppTex) (*ppTex)->Release();
        if (*ppSurf) (*ppSurf)->Release();

        HRESULT hr = m_pDevice->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, fmt, D3DPOOL_DEFAULT, ppTex, NULL);
        if (SUCCEEDED(hr) && *ppTex) {
            (*ppTex)->GetSurfaceLevel(0, ppSurf);
        }
    };

    CreateRT(&m_pBloomExtractTexture, &m_pBloomExtractSurface, halfW, halfH, D3DFMT_A8R8G8B8);
    CreateRT(&m_pBloomBlurTexture1, &m_pBloomBlurSurface1, halfW, halfH, D3DFMT_A8R8G8B8);
    CreateRT(&m_pBloomBlurTexture2, &m_pBloomBlurSurface2, halfW, halfH, D3DFMT_A8R8G8B8);
    CreateRT(&m_pTempTexture, &m_pTempSurface, m_screenWidth, m_screenHeight, D3DFMT_A8R8G8B8);

    OutputDebugStringA("[PostProcessing] Render targets created\n");
}

void PostProcessingSystem::DestroyRenderTargets() {
    if (m_pBloomExtractSurface) { m_pBloomExtractSurface->Release(); m_pBloomExtractSurface = NULL; }
    if (m_pBloomExtractTexture) { m_pBloomExtractTexture->Release(); m_pBloomExtractTexture = NULL; }

    if (m_pBloomBlurSurface1) { m_pBloomBlurSurface1->Release(); m_pBloomBlurSurface1 = NULL; }
    if (m_pBloomBlurTexture1) { m_pBloomBlurTexture1->Release(); m_pBloomBlurTexture1 = NULL; }

    if (m_pBloomBlurSurface2) { m_pBloomBlurSurface2->Release(); m_pBloomBlurSurface2 = NULL; }
    if (m_pBloomBlurTexture2) { m_pBloomBlurTexture2->Release(); m_pBloomBlurTexture2 = NULL; }

    if (m_pTempSurface) { m_pTempSurface->Release(); m_pTempSurface = NULL; }
    if (m_pTempTexture) { m_pTempTexture->Release(); m_pTempTexture = NULL; }
}

void PostProcessingSystem::CreateQuadMesh() {
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

void PostProcessingSystem::Render(IDirect3DTexture9* inputTexture, IDirect3DSurface9* outputSurface) {
    if (!m_enabled || !m_pDevice || !inputTexture) return;

    OutputDebugStringA("[PostProcessing] Render starting\n");

    IDirect3DSurface9* pOrigRT = NULL;
    m_pDevice->GetRenderTarget(0, &pOrigRT);

    if (m_bloomEnabled) {
        ExtractBrightPixels(inputTexture, m_pBloomExtractTexture);
        GaussianBlur(m_pBloomExtractTexture, m_pBloomBlurTexture1, m_bloomSettings.radius);
        GaussianBlur(m_pBloomBlurTexture1, m_pBloomBlurTexture2, m_bloomSettings.radius);
    }

    if (m_toneMappingEnabled) {
        ApplyToneMapping(inputTexture, m_pTempTexture);
    }

    if (m_colorGradingEnabled) {
        ApplyColorGrading(m_pTempTexture, m_pTempTexture);
    }

    if (m_fogSettings.enabled) {
        ApplyFog(m_pTempTexture, m_pTempTexture);
    }

    // Xbox 360: StretchRect not available, use quad render instead
    if (outputSurface) {
        // TODO: Implement quad-based texture copy for Xbox 360
        // For now, just set the render target
        m_pDevice->SetRenderTarget(0, outputSurface);
    }

    if (pOrigRT) { pOrigRT->Release(); }

    OutputDebugStringA("[PostProcessing] Render complete\n");
}

void PostProcessingSystem::SetBloomEnabled(bool enable) {
    m_bloomEnabled = enable;
}

void PostProcessingSystem::SetToneMappingEnabled(bool enable) {
    m_toneMappingEnabled = enable;
}

void PostProcessingSystem::SetColorGradingEnabled(bool enable) {
    m_colorGradingEnabled = enable;
}

void PostProcessingSystem::ExtractBrightPixels(IDirect3DTexture9* input, IDirect3DTexture9* output) {
    if (!m_pDevice || !input || !output) return;

    LPDIRECT3DSURFACE9 pSurf = NULL;
    output->GetSurfaceLevel(0, &pSurf);
    if (pSurf) {
        m_pDevice->SetRenderTarget(0, pSurf);
        m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
        pSurf->Release();
    }
}

void PostProcessingSystem::GaussianBlur(IDirect3DTexture9* input, IDirect3DTexture9* output, float radius) {
    if (!m_pDevice || !input || !output) return;

    LPDIRECT3DSURFACE9 pSurf = NULL;
    output->GetSurfaceLevel(0, &pSurf);
    if (pSurf) {
        m_pDevice->SetRenderTarget(0, pSurf);
        pSurf->Release();
    }
}

void PostProcessingSystem::ApplyToneMapping(IDirect3DTexture9* input, IDirect3DTexture9* output) {
    if (!m_pDevice) return;

    if (output) {
        LPDIRECT3DSURFACE9 pSurf = NULL;
        output->GetSurfaceLevel(0, &pSurf);
        if (pSurf) {
            // Xbox 360: StretchRect not available, use quad render instead
            // TODO: Implement quad-based texture copy for Xbox 360
            m_pDevice->SetRenderTarget(0, pSurf);
            pSurf->Release();
        }
    }
}

void PostProcessingSystem::ApplyColorGrading(IDirect3DTexture9* input, IDirect3DTexture9* output) {
    if (!m_pDevice || !input) return;

    if (output) {
        LPDIRECT3DSURFACE9 pSurf = NULL;
        output->GetSurfaceLevel(0, &pSurf);
        if (pSurf) {
            // Xbox 360: StretchRect not available, use quad render instead
            // TODO: Implement quad-based texture copy for Xbox 360
            m_pDevice->SetRenderTarget(0, pSurf);
            pSurf->Release();
        }
    }
}

void PostProcessingSystem::ApplyFog(IDirect3DTexture9* input, IDirect3DTexture9* output) {
    if (!m_pDevice || !input) return;

    if (output) {
        LPDIRECT3DSURFACE9 pSurf = NULL;
        output->GetSurfaceLevel(0, &pSurf);
        if (pSurf) {
            // Xbox 360: StretchRect not available, use quad render instead
            // TODO: Implement quad-based texture copy for Xbox 360
            m_pDevice->SetRenderTarget(0, pSurf);
            pSurf->Release();
        }
    }
}

}
