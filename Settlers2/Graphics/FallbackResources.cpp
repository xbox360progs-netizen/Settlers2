#include "stdafx.h"
#include "FallbackResources.h"

#ifdef _DEBUG
#define FALLBACK_LOG(msg, ...) \
    do { \
        char _buf[256]; \
        sprintf(_buf, "[Fallback] " msg "\n", __VA_ARGS__); \
        ::OutputDebugStringA(_buf); \
    } while(0)
#else
#define FALLBACK_LOG(...) ((void)0)
#endif

namespace Graphics {

static FallbackResources* g_fallbackResources = NULL;

FallbackResources::FallbackResources()
    : m_device(NULL)
    , m_initialized(false)
    , m_whiteTexture(NULL)
    , m_flatNormalTexture(NULL)
    , m_blackAOTexture(NULL)
    , m_errorRedTexture(NULL)
    , m_errorPinkTexture(NULL)
    , m_errorPixelShader(NULL)
    , m_errorVertexShader(NULL)
{
}

FallbackResources::~FallbackResources() {
    Shutdown();
}

void FallbackResources::Initialize(IDirect3DDevice9* device) {
    m_device = device;
    if (!m_device) return;
    
    CreateSolidTexture(&m_whiteTexture, D3DCOLOR_ARGB(255, 255, 255, 255));
    CreateFlatNormalTexture(&m_flatNormalTexture);
    CreateSolidTexture(&m_blackAOTexture, D3DCOLOR_ARGB(255, 0, 0, 0));
    CreateSolidTexture(&m_errorRedTexture, D3DCOLOR_ARGB(255, 255, 0, 0));
    CreateSolidTexture(&m_errorPinkTexture, D3DCOLOR_ARGB(255, 255, 0, 255));
    
    m_initialized = true;
    g_fallbackResources = this;
    
    FALLBACK_LOG("Initialized");
}

void FallbackResources::Shutdown() {
    if (m_whiteTexture) { m_whiteTexture->Release(); m_whiteTexture = NULL; }
    if (m_flatNormalTexture) { m_flatNormalTexture->Release(); m_flatNormalTexture = NULL; }
    if (m_blackAOTexture) { m_blackAOTexture->Release(); m_blackAOTexture = NULL; }
    if (m_errorRedTexture) { m_errorRedTexture->Release(); m_errorRedTexture = NULL; }
    if (m_errorPinkTexture) { m_errorPinkTexture->Release(); m_errorPinkTexture = NULL; }
    
    if (m_errorPixelShader) { m_errorPixelShader->Release(); m_errorPixelShader = NULL; }
    if (m_errorVertexShader) { m_errorVertexShader->Release(); m_errorVertexShader = NULL; }
    
    m_device = NULL;
    m_initialized = false;
    g_fallbackResources = NULL;
    
    FALLBACK_LOG("Shutdown complete");
}

void FallbackResources::CreateSolidTexture(IDirect3DTexture9** outTex, D3DCOLOR color) {
    if (!m_device || !outTex) return;
    
    HRESULT hr = m_device->CreateTexture(1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, outTex, NULL);
    if (FAILED(hr) || !*outTex) {
        FALLBACK_LOG("ERROR: Failed to create solid texture");
        return;
    }
    
    D3DLOCKED_RECT lockRect;
    hr = (*outTex)->LockRect(0, &lockRect, NULL, 0);
    if (SUCCEEDED(hr)) {
        *(D3DCOLOR*)lockRect.pBits = color;
        (*outTex)->UnlockRect(0);
    }
}

void FallbackResources::CreateFlatNormalTexture(IDirect3DTexture9** outTex) {
    if (!m_device || !outTex) return;
    
    HRESULT hr = m_device->CreateTexture(1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, outTex, NULL);
    if (FAILED(hr) || !*outTex) {
        FALLBACK_LOG("ERROR: Failed to create flat normal texture");
        return;
    }
    
    D3DLOCKED_RECT lockRect;
    hr = (*outTex)->LockRect(0, &lockRect, NULL, 0);
    if (SUCCEEDED(hr)) {
        D3DCOLOR normalColor = D3DCOLOR_ARGB(255, 128, 128, 255);
        *(D3DCOLOR*)lockRect.pBits = normalColor;
        (*outTex)->UnlockRect(0);
    }
}

Texture* FallbackResources::GetFallbackTexture(FallbackType type) {
    return NULL;
}

IDirect3DTexture9* FallbackResources::GetFallbackD3DTexture(FallbackType type) {
    switch (type) {
        case FALLBACK_WHITE: return m_whiteTexture;
        case FALLBACK_FLAT_NORMAL: return m_flatNormalTexture;
        case FALLBACK_BLACK_AO: return m_blackAOTexture;
        case FALLBACK_ERROR_RED: return m_errorRedTexture;
        case FALLBACK_ERROR_PINK: return m_errorPinkTexture;
    }
    return m_errorPinkTexture;
}

FallbackResources* GetGlobalFallbackResources() {
    return g_fallbackResources;
}

void SetGlobalFallbackResources(FallbackResources* res) {
    g_fallbackResources = res;
}

}
