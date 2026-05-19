#include "stdafx.h"
#include "Xbox360EDRAM.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

static int GetFormatBytesPerPixel(D3DFORMAT format) {
    switch (format) {
        case D3DFMT_A32B32G32R32F: return 16;
        case D3DFMT_A16B16G16R16F: return 8;
        case D3DFMT_A8R8G8B8: return 4;
        case D3DFMT_X8R8G8B8: return 4;
        case D3DFMT_R32F: return 4;
        case D3DFMT_D24S8: return 4;
        case D3DFMT_D32F: return 4;
        case D3DFMT_D16: return 2;
        default: return 4;
    }
}

Xbox360EDRAMBudget::Xbox360EDRAMBudget()
    : m_pDevice(NULL)
    , m_screenWidth(0)
    , m_screenHeight(0)
    , m_totalUsedBytes(0)
    , m_maxTiles(32)
    , m_state(BUDGET_OK)
    , m_resolveCount(0)
    , m_mrtColor0(D3DFMT_UNKNOWN)
    , m_mrtColor1(D3DFMT_UNKNOWN)
    , m_mrtColor2(D3DFMT_UNKNOWN)
    , m_mrtColor3(D3DFMT_UNKNOWN)
    , m_depthFormat(D3DFMT_UNKNOWN)
{
}

Xbox360EDRAMBudget::~Xbox360EDRAMBudget() {
    Shutdown();
}

void Xbox360EDRAMBudget::Initialize(IDirect3DDevice9* pDevice, int screenWidth, int screenHeight) {
    m_pDevice = pDevice;
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    m_totalUsedBytes = 0;

    m_tiles.resize(m_maxTiles);
    for (int i = 0; i < m_maxTiles; i++) {
        m_tiles[i].allocated = false;
    }

    OutputDebugStringA("[Xbox360EDRAMBudget] Initialized - 10MB EDRAM budget\n");
}

void Xbox360EDRAMBudget::Shutdown() {
    m_tiles.clear();
    m_tileStats.clear();
    m_pDevice = NULL;
}

void Xbox360EDRAMBudget::BeginFrame() {
    m_totalUsedBytes = 0;
    m_resolveCount = 0;
    m_state = BUDGET_OK;
    m_tileStats.clear();

    for (int i = 0; i < m_maxTiles; i++) {
        if (m_tiles[i].allocated) {
            m_tiles[i].bytesUsed = 0;
        }
    }
}

void Xbox360EDRAMBudget::EndFrame() {
    EDRAMTileStats stats;
    for (int i = 0; i < m_maxTiles; i++) {
        if (m_tiles[i].allocated) {
            stats.tileIndex = i;
            stats.bytesUsed = m_tiles[i].bytesUsed;
            stats.bytesTotal = m_tiles[i].width * m_tiles[i].height * m_tiles[i].bytesPerPixel;
            stats.surfaceName = m_tiles[i].name;
            m_tileStats.push_back(stats);
        }
    }
}

int Xbox360EDRAMBudget::AllocateTile(const char* name, int width, int height, D3DFORMAT format) {
    int bytes = CalculateTileBytes(width, height, format);
    if (bytes <= 0) return -1;

    for (int i = 0; i < m_maxTiles; i++) {
        if (!m_tiles[i].allocated) {
            m_tiles[i].name = name;
            m_tiles[i].width = width;
            m_tiles[i].height = height;
            m_tiles[i].format = format;
            m_tiles[i].bytesPerPixel = GetFormatBytesPerPixel(format);
            m_tiles[i].bytesUsed = bytes;
            m_tiles[i].allocated = true;
            m_totalUsedBytes += bytes;

            char buf[128];
            sprintf(buf, "[EDRAM] Allocated tile %d: %s (%dx%d) = %d bytes\n", 
                i, name, width, height, bytes);
            OutputDebugStringA(buf);

            if (m_totalUsedBytes > 10 * 1024 * 1024) {
                m_state = BUDGET_OVERFLOW;
                OutputDebugStringA("[EDRAM] WARNING: EDRAM overflow!\n");
            }

            return i;
        }
    }

    OutputDebugStringA("[EDRAM] ERROR: No free tiles!\n");
    return -1;
}

void Xbox360EDRAMBudget::ReleaseTile(int tileIndex) {
    if (tileIndex >= 0 && tileIndex < m_maxTiles && m_tiles[tileIndex].allocated) {
        m_totalUsedBytes -= m_tiles[tileIndex].bytesUsed;
        m_tiles[tileIndex].allocated = false;
    }
}

void Xbox360EDRAMBudget::SetMRTFormats(D3DFORMAT color0, D3DFORMAT color1, D3DFORMAT color2, D3DFORMAT color3) {
    m_mrtColor0 = color0;
    m_mrtColor1 = color1;
    m_mrtColor2 = color2;
    m_mrtColor3 = color3;
}

void Xbox360EDRAMBudget::SetDepthFormat(D3DFORMAT format) {
    m_depthFormat = format;
}

float Xbox360EDRAMBudget::GetUtilization() const {
    int budget = 10 * 1024 * 1024;
    return (float)m_totalUsedBytes / (float)budget * 100.0f;
}

int Xbox360EDRAMBudget::CalculateTileBytes(int width, int height, D3DFORMAT format) {
    int bpp = GetFormatBytesPerPixel(format);
    return width * height * bpp;
}

ResolveOptimizer::ResolveOptimizer()
    : m_pDevice(NULL)
    , m_resolveCount(0)
    , m_skippedResolves(0)
    , m_currentFrame(0)
    , m_dirtyThreshold(2)
{
}

ResolveOptimizer::~ResolveOptimizer() {
    Shutdown();
}

void ResolveOptimizer::Initialize(IDirect3DDevice9* pDevice) {
    m_pDevice = pDevice;
    OutputDebugStringA("[ResolveOptimizer] Initialized\n");
}

void ResolveOptimizer::Shutdown() {
    m_surfaceCache.clear();
    m_pDevice = NULL;
}

bool ResolveOptimizer::ShouldResolve(IDirect3DSurface9* surface) {
    for (size_t i = 0; i < m_surfaceCache.size(); i++) {
        if (m_surfaceCache[i].surface == surface) {
            if (!m_surfaceCache[i].dirty || 
                (m_currentFrame - m_surfaceCache[i].frameDirty) < m_dirtyThreshold) {
                m_skippedResolves++;
                return false;
            }
            return true;
        }
    }
    return true;
}

void ResolveOptimizer::MarkDirty(IDirect3DSurface9* surface) {
    for (size_t i = 0; i < m_surfaceCache.size(); i++) {
        if (m_surfaceCache[i].surface == surface) {
            m_surfaceCache[i].dirty = true;
            m_surfaceCache[i].frameDirty = m_currentFrame;
            return;
        }
    }

    SurfaceInfo info;
    info.surface = surface;
    info.dirty = true;
    info.frameDirty = m_currentFrame;
    m_surfaceCache.push_back(info);
}

void ResolveOptimizer::MarkClean(IDirect3DSurface9* surface) {
    for (size_t i = 0; i < m_surfaceCache.size(); i++) {
        if (m_surfaceCache[i].surface == surface) {
            m_surfaceCache[i].dirty = false;
            return;
        }
    }
}

void ResolveOptimizer::ClearDirtyFlags() {
    for (size_t i = 0; i < m_surfaceCache.size(); i++) {
        m_surfaceCache[i].dirty = false;
    }
}

void ResolveOptimizer::BeginFrame() {
    m_currentFrame++;
    m_resolveCount = 0;
    m_skippedResolves = 0;
}

void ResolveOptimizer::EndFrame() {
}

MSAAOptimizer::MSAAOptimizer()
    : m_pDevice(NULL)
    , m_currentMSAAType(D3DMULTISAMPLE_NONE)
{
    for (int i = 0; i < 4; i++) {
        m_msaaDisabled[i] = false;
    }
}

MSAAOptimizer::~MSAAOptimizer() {
    Shutdown();
}

void MSAAOptimizer::Initialize(IDirect3DDevice9* pDevice) {
    m_pDevice = pDevice;
    OutputDebugStringA("[MSAAOptimizer] Initialized\n");
}

void MSAAOptimizer::Shutdown() {
    m_pDevice = NULL;
}

bool MSAAOptimizer::ShouldUseMSAA(int renderTargetIndex) {
    if (renderTargetIndex < 0 || renderTargetIndex >= 4) return false;
    return !m_msaaDisabled[renderTargetIndex];
}

void MSAAOptimizer::SetMSAADisabledForTarget(int renderTargetIndex, bool disabled) {
    if (renderTargetIndex >= 0 && renderTargetIndex < 4) {
        m_msaaDisabled[renderTargetIndex] = disabled;
    }
}

}