#pragma once
#include <d3d9.h>
#include <vector>

namespace Graphics {

enum EDRAMBudgetState {
    BUDGET_OK,
    BUDGET_WARNING,
    BUDGET_OVERFLOW
};

struct MRTFormatConfig {
    D3DFORMAT format;
    int bytesPerPixel;
    bool requiresResolve;
};

struct EDRAMTileStats {
    int tileIndex;
    int bytesUsed;
    int bytesTotal;
    const char* surfaceName;
};

class Xbox360EDRAMBudget {
public:
    Xbox360EDRAMBudget();
    ~Xbox360EDRAMBudget();

    void Initialize(IDirect3DDevice9* pDevice, int screenWidth, int screenHeight);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    int AllocateTile(const char* name, int width, int height, D3DFORMAT format);
    void ReleaseTile(int tileIndex);

    void SetMRTFormats(D3DFORMAT color0, D3DFORMAT color1, D3DFORMAT color2, D3DFORMAT color3);
    void SetDepthFormat(D3DFORMAT format);

    int GetTotalUsedBytes() const { return m_totalUsedBytes; }
    int GetTotalBudgetBytes() const { return 10 * 1024 * 1024; }
    float GetUtilization() const;

    EDRAMBudgetState GetState() const { return m_state; }
    int GetResolveCount() const { return m_resolveCount; }

    void RecordResolve() { m_resolveCount++; }
    void ResetResolveCount() { m_resolveCount = 0; }

    const std::vector<EDRAMTileStats>& GetTileStats() const { return m_tileStats; }

private:
    IDirect3DDevice9* m_pDevice;
    int m_screenWidth;
    int m_screenHeight;
    int m_totalUsedBytes;
    int m_maxTiles;
    EDRAMBudgetState m_state;
    int m_resolveCount;

    struct EDRAMTile {
        const char* name;
        int width;
        int height;
        D3DFORMAT format;
        int bytesPerPixel;
        int bytesUsed;
        bool allocated;
    };

    std::vector<EDRAMTile> m_tiles;
    std::vector<EDRAMTileStats> m_tileStats;

    D3DFORMAT m_mrtColor0;
    D3DFORMAT m_mrtColor1;
    D3DFORMAT m_mrtColor2;
    D3DFORMAT m_mrtColor3;
    D3DFORMAT m_depthFormat;

    int CalculateTileBytes(int width, int height, D3DFORMAT format);
};

class ResolveOptimizer {
public:
    ResolveOptimizer();
    ~ResolveOptimizer();

    void Initialize(IDirect3DDevice9* pDevice);
    void Shutdown();

    bool ShouldResolve(IDirect3DSurface9* surface);
    void MarkDirty(IDirect3DSurface9* surface);
    void MarkClean(IDirect3DSurface9* surface);
    void ClearDirtyFlags();

    void BeginFrame();
    void EndFrame();

    int GetResolveCount() const { return m_resolveCount; }
    int GetSkippedResolves() const { return m_skippedResolves; }

private:
    IDirect3DDevice9* m_pDevice;
    int m_resolveCount;
    int m_skippedResolves;

    struct SurfaceInfo {
        IDirect3DSurface9* surface;
        bool dirty;
        int frameDirty;
    };

    std::vector<SurfaceInfo> m_surfaceCache;
    int m_currentFrame;
    int m_dirtyThreshold;
};

class MSAAOptimizer {
public:
    MSAAOptimizer();
    ~MSAAOptimizer();

    void Initialize(IDirect3DDevice9* pDevice);
    void Shutdown();

    bool ShouldUseMSAA(int renderTargetIndex);
    void SetMSAADisabledForTarget(int renderTargetIndex, bool disabled);

    D3DMULTISAMPLE_TYPE GetCurrentMSAAType() const { return m_currentMSAAType; }
    void SetMSAAType(D3DMULTISAMPLE_TYPE type) { m_currentMSAAType = type; }

private:
    IDirect3DDevice9* m_pDevice;
    D3DMULTISAMPLE_TYPE m_currentMSAAType;
    bool m_msaaDisabled[4];
};

}
