#pragma once
#include <d3d9.h>
#include <map>
#include <vector>
#include <string>

namespace Graphics {

struct RenderTargetInfo {
    int id;
    const char* name;
    IDirect3DSurface9* surface;
    int width;
    int height;
    D3DFORMAT format;
    bool isDepth;
    bool isTransient;
    int frameAcquired;
    
    RenderTargetInfo() 
        : id(-1), name(NULL), surface(NULL), width(0), height(0), 
          format(D3DFMT_UNKNOWN), isDepth(false), isTransient(true), frameAcquired(-1) {}
};

class RenderTargetManager {
public:
    RenderTargetManager();
    ~RenderTargetManager();
    
    void Initialize(IDirect3DDevice9* device, int width, int height);
    void Shutdown();
    
    int CreateRenderTarget(const char* name, int width, int height, D3DFORMAT format, bool isDepth, bool isTransient = false);
    int GetRenderTarget(const char* name);
    
    void BeginFrame();
    void EndFrame();
    
    IDirect3DSurface9* AcquireRead(int rtId);
    IDirect3DSurface9* AcquireWrite(int rtId);
    void Release(int rtId);
    
    void ValidateAllReleased();
    
    const RenderTargetInfo* GetInfo(int rtId) const;
    int GetCount() const { return (int)m_renderTargets.size(); }
    
    void SetDebugValidation(bool enable) { m_debugValidation = enable; }
    
private:
    IDirect3DDevice9* m_device;
    int m_width;
    int m_height;
    int m_nextId;
    int m_currentFrame;
    bool m_debugValidation;
    
    std::map<std::string, int> m_nameToId;
    std::vector<RenderTargetInfo> m_renderTargets;
    
    struct PendingRelease {
        int rtId;
        int frameToRelease;
    };
    std::vector<PendingRelease> m_pendingReleases;
    
    int FindFreeSlot();
    void ReportLeak(int rtId);
};

RenderTargetManager* GetGlobalRTManager();
void SetGlobalRTManager(RenderTargetManager* mgr);

}
