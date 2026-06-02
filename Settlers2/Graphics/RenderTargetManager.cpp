#include "stdafx.h"
#include "RenderTargetManager.h"
#include <algorithm>

#ifdef _DEBUG
#define RT_LOG(msg, ...) \
    do { \
        char _buf[256]; \
        sprintf(_buf, "[RTManager] " msg "\n", __VA_ARGS__); \
        ::OutputDebugStringA(_buf); \
    } while(0)
#else
#define RT_LOG(...) ((void)0)
#endif

namespace Graphics {

static RenderTargetManager* g_globalRTManager = NULL;

RenderTargetManager::RenderTargetManager()
    : m_device(NULL)
    , m_width(0)
    , m_height(0)
    , m_nextId(0)
    , m_currentFrame(0)
    , m_debugValidation(true)
{
}

RenderTargetManager::~RenderTargetManager() {
    Shutdown();
}

void RenderTargetManager::Initialize(IDirect3DDevice9* device, int width, int height) {
    m_device = device;
    m_width = width;
    m_height = height;
    m_nextId = 0;
    
    RT_LOG("Initialized with %dx%d", width, height);
}

void RenderTargetManager::Shutdown() {
    for (size_t i = 0; i < m_renderTargets.size(); i++) {
        if (m_renderTargets[i].surface) {
            m_renderTargets[i].surface->Release();
            m_renderTargets[i].surface = NULL;
        }
    }
    
    m_renderTargets.clear();
    m_nameToId.clear();
    m_pendingReleases.clear();
    
    m_device = NULL;
    RT_LOG("Shutdown complete");
}

int RenderTargetManager::CreateRenderTarget(const char* name, int width, int height, D3DFORMAT format, bool isDepth, bool isTransient) {
    if (!m_device) {
        RT_LOG("ERROR: No device");
        return -1;
    }
    
    if (m_nameToId.find(name) != m_nameToId.end()) {
        RT_LOG("ERROR: RT %s already exists", name);
        return m_nameToId[name];
    }
    
    IDirect3DSurface9* surface = NULL;
    HRESULT hr = S_OK;
    
    if (isDepth) {
        hr = m_device->CreateDepthStencilSurface(width, height, format, D3DMULTISAMPLE_NONE, 0, FALSE, &surface, NULL);
    } else {
        hr = m_device->CreateRenderTarget(width, height, format, D3DMULTISAMPLE_NONE, 0, FALSE, &surface, NULL);
    }
    
    if (FAILED(hr) || !surface) {
        RT_LOG("ERROR: Failed to create RT %s", name);
        return -1;
    }
    
    RenderTargetInfo info;
    info.id = m_nextId++;
    info.name = name;
    info.surface = surface;
    info.width = width;
    info.height = height;
    info.format = format;
    info.isDepth = isDepth;
    info.isTransient = isTransient;
    info.frameAcquired = -1;
    
    m_nameToId[name] = info.id;
    m_renderTargets.push_back(info);
    
    RT_LOG("Created RT %s (id=%d, %dx%d, %s)", name, info.id, width, height, isDepth ? "depth" : "color");
    
    return info.id;
}

int RenderTargetManager::GetRenderTarget(const char* name) {
    auto it = m_nameToId.find(name);
    if (it != m_nameToId.end()) {
        return it->second;
    }
    return -1;
}

void RenderTargetManager::BeginFrame() {
    m_currentFrame++;
    
    for (size_t i = 0; i < m_pendingReleases.size(); i++) {
        if (m_pendingReleases[i].frameToRelease == m_currentFrame) {
            int rtId = m_pendingReleases[i].rtId;
            if (rtId >= 0 && rtId < (int)m_renderTargets.size()) {
                m_renderTargets[rtId].frameAcquired = -1;
            }
        }
    }
    
    m_pendingReleases.clear();
}

void RenderTargetManager::EndFrame() {
    if (!m_debugValidation) return;
    
    for (size_t i = 0; i < m_renderTargets.size(); i++) {
        if (m_renderTargets[i].isTransient && m_renderTargets[i].frameAcquired > 0) {
            ReportLeak(m_renderTargets[i].id);
        }
    }
}

IDirect3DSurface9* RenderTargetManager::AcquireRead(int rtId) {
    if (rtId < 0 || rtId >= (int)m_renderTargets.size()) {
        RT_LOG("ERROR: Invalid RT id %d", rtId);
        return NULL;
    }
    
    RenderTargetInfo& info = m_renderTargets[rtId];
    
    if (info.isTransient) {
        info.frameAcquired = m_currentFrame;
    }
    
    if (info.surface) {
        info.surface->AddRef();
    }
    
    return info.surface;
}

IDirect3DSurface9* RenderTargetManager::AcquireWrite(int rtId) {
    if (rtId < 0 || rtId >= (int)m_renderTargets.size()) {
        RT_LOG("ERROR: Invalid RT id %d", rtId);
        return NULL;
    }
    
    RenderTargetInfo& info = m_renderTargets[rtId];
    
    if (info.isTransient) {
        info.frameAcquired = m_currentFrame;
    }
    
    if (info.surface) {
        info.surface->AddRef();
    }
    
    return info.surface;
}

void RenderTargetManager::Release(int rtId) {
    if (rtId < 0 || rtId >= (int)m_renderTargets.size()) {
        return;
    }
    
    RenderTargetInfo& info = m_renderTargets[rtId];
    info.frameAcquired = -1;
    
    if (info.surface) {
        info.surface->Release();
    }
}

void RenderTargetManager::ValidateAllReleased() {
#ifdef _DEBUG
    if (!m_debugValidation) return;
    
    for (size_t i = 0; i < m_renderTargets.size(); i++) {
        if (m_renderTargets[i].isTransient && m_renderTargets[i].frameAcquired > 0) {
            ReportLeak(m_renderTargets[i].id);
        }
    }
#endif
}

const RenderTargetInfo* RenderTargetManager::GetInfo(int rtId) const {
    if (rtId >= 0 && rtId < (int)m_renderTargets.size()) {
        return &m_renderTargets[rtId];
    }
    return NULL;
}

int RenderTargetManager::FindFreeSlot() {
    for (int i = 0; i < (int)m_renderTargets.size(); i++) {
        if (m_renderTargets[i].surface == NULL) {
            return i;
        }
    }
    return -1;
}

void RenderTargetManager::ReportLeak(int rtId) {
#ifdef _DEBUG
    if (rtId >= 0 && rtId < (int)m_renderTargets.size()) {
        char msg[256];
        sprintf(msg, "[LEAK] RT %s (id=%d) not released at end of frame!\n", 
                m_renderTargets[rtId].name ? m_renderTargets[rtId].name : "unknown", rtId);
        ::OutputDebugStringA(msg);
    }
#endif
}

RenderTargetManager* GetGlobalRTManager() {
    return g_globalRTManager;
}

void SetGlobalRTManager(RenderTargetManager* mgr) {
    g_globalRTManager = mgr;
}

}
