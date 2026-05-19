#pragma once
#include <d3d9.h>

namespace Graphics {

struct GPUTextureInfo {
    const char* name;
    int width;
    int height;
    D3DFORMAT format;
    int bytesPerPixel;
    int totalBytes;
    bool isRenderTarget;
    bool isDepthStencil;
};

struct GPUMemoryReport {
    int totalGBufferBytes;
    int totalRTBytes;
    int depthStencilBytes;

    int gBufferCount;
    int rtCount;
    int depthStencilCount;

    int estimatedBandwidthMB;
    int estimatedEDRAMUsage; // Xbox 360 specific

    const char* warnings;
};

class GPUMemoryAuditor {
public:
    GPUMemoryAuditor();

    GPUMemoryReport GenerateReport();

    void AddGBuffer(const char* name, int width, int height, D3DFORMAT format);
    void AddRenderTarget(const char* name, int width, int height, D3DFORMAT format);
    void AddDepthStencil(const char* name, int width, int height, D3DFORMAT format);

    int CalculateBytesPerPixel(D3DFORMAT format);
    int CalculateSurfaceBytes(int width, int height, D3DFORMAT format);

    void PrintReport();

    void SetResolution(int width, int height) {
        m_screenWidth = width;
        m_screenHeight = height;
    }

    int GetTotalMemoryBytes() const { return m_totalBytes; }

    const char* GetFormatName(D3DFORMAT format);

private:
    int m_screenWidth;
    int m_screenHeight;
    int m_totalBytes;

    GPUTextureInfo m_gBuffers[8];
    GPUTextureInfo m_renderTargets[8];
    GPUTextureInfo m_depthStencil[4];

    int m_gBufferCount;
    int m_rtCount;
    int m_depthStencilCount;

    void Reset();
};

}