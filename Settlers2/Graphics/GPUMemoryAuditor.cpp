#include "stdafx.h"
#include "GPUMemoryAuditor.h"
#include <stdio.h>

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

GPUMemoryAuditor::GPUMemoryAuditor()
    : m_screenWidth(1280), m_screenHeight(720), m_totalBytes(0),
      m_gBufferCount(0), m_rtCount(0), m_depthStencilCount(0) {
    Reset();
}

void GPUMemoryAuditor::Reset() {
    memset(m_gBuffers, 0, sizeof(m_gBuffers));
    memset(m_renderTargets, 0, sizeof(m_renderTargets));
    memset(m_depthStencil, 0, sizeof(m_depthStencil));
    m_totalBytes = 0;
    m_gBufferCount = 0;
    m_rtCount = 0;
    m_depthStencilCount = 0;
}

int GPUMemoryAuditor::CalculateBytesPerPixel(D3DFORMAT format) {
    switch (format) {
    case D3DFMT_A32B32G32R32F: return 16;
    case D3DFMT_A16B16G16R16F: return 8;
    case D3DFMT_A8R8G8B8:      return 4;
    case D3DFMT_R8G8B8:        return 3;
    case D3DFMT_R5G6B5:        return 2;
    case D3DFMT_D24S8:         return 4;
    case D3DFMT_D32:           return 4;
    case D3DFMT_D16:           return 2;
    default:                   return 4;
    }
}

int GPUMemoryAuditor::CalculateSurfaceBytes(int width, int height, D3DFORMAT format) {
    int bpp = CalculateBytesPerPixel(format);
    return width * height * bpp;
}

const char* GPUMemoryAuditor::GetFormatName(D3DFORMAT format) {
    switch (format) {
    case D3DFMT_A32B32G32R32F: return "A32B32G32R32F (128bpp)";
    case D3DFMT_A16B16G16R16F: return "A16B16G16R16F (64bpp)";
    case D3DFMT_A8R8G8B8:      return "A8R8G8B8 (32bpp)";
    case D3DFMT_R8G8B8:        return "R8G8B8 (24bpp)";
    case D3DFMT_R5G6B5:        return "R5G6B5 (16bpp)";
    case D3DFMT_D24S8:         return "D24S8 (32bpp)";
    case D3DFMT_D32:           return "D32 (32bpp)";
    case D3DFMT_D16:           return "D16 (16bpp)";
    default:                   return "Unknown";
    }
}

void GPUMemoryAuditor::AddGBuffer(const char* name, int width, int height, D3DFORMAT format) {
    if (m_gBufferCount >= 8) return;

    GPUTextureInfo& info = m_gBuffers[m_gBufferCount++];
    info.name = name;
    info.width = width;
    info.height = height;
    info.format = format;
    info.bytesPerPixel = CalculateBytesPerPixel(format);
    info.totalBytes = CalculateSurfaceBytes(width, height, format);
    info.isRenderTarget = true;
    info.isDepthStencil = false;

    m_totalBytes += info.totalBytes;
}

void GPUMemoryAuditor::AddRenderTarget(const char* name, int width, int height, D3DFORMAT format) {
    if (m_rtCount >= 8) return;

    GPUTextureInfo& info = m_renderTargets[m_rtCount++];
    info.name = name;
    info.width = width;
    info.height = height;
    info.format = format;
    info.bytesPerPixel = CalculateBytesPerPixel(format);
    info.totalBytes = CalculateSurfaceBytes(width, height, format);
    info.isRenderTarget = true;
    info.isDepthStencil = false;

    m_totalBytes += info.totalBytes;
}

void GPUMemoryAuditor::AddDepthStencil(const char* name, int width, int height, D3DFORMAT format) {
    if (m_depthStencilCount >= 4) return;

    GPUTextureInfo& info = m_depthStencil[m_depthStencilCount++];
    info.name = name;
    info.width = width;
    info.height = height;
    info.format = format;
    info.bytesPerPixel = CalculateBytesPerPixel(format);
    info.totalBytes = CalculateSurfaceBytes(width, height, format);
    info.isRenderTarget = false;
    info.isDepthStencil = true;

    m_totalBytes += info.totalBytes;
}

GPUMemoryReport GPUMemoryAuditor::GenerateReport() {
    GPUMemoryReport report;
    memset(&report, 0, sizeof(report));

    report.totalGBufferBytes = 0;
    report.gBufferCount = m_gBufferCount;
    for (int i = 0; i < m_gBufferCount; i++) {
        report.totalGBufferBytes += m_gBuffers[i].totalBytes;
    }

    report.totalRTBytes = 0;
    report.rtCount = m_rtCount;
    for (int i = 0; i < m_rtCount; i++) {
        report.totalRTBytes += m_renderTargets[i].totalBytes;
    }

    report.depthStencilBytes = 0;
    report.depthStencilCount = m_depthStencilCount;
    for (int i = 0; i < m_depthStencilCount; i++) {
        report.depthStencilBytes += m_depthStencil[i].totalBytes;
    }

    int resolution = m_screenWidth * m_screenHeight;

    int framesPerSecond = 60;
    int bytesPerFrame = report.totalGBufferBytes + report.totalRTBytes + report.depthStencilBytes;
    int bandwidthBytesPerSec = bytesPerFrame * framesPerSecond;
    report.estimatedBandwidthMB = bandwidthBytesPerSec / (1024 * 1024);

#ifdef _XBOX
    report.estimatedEDRAMUsage = report.totalGBufferBytes;
    if (report.estimatedEDRAMUsage > 10 * 1024 * 1024) {
        report.warnings = "WARNING: GBuffer exceeds 10MB EDRAM budget on Xbox 360!";
    } else {
        report.warnings = "OK: GBuffer fits in EDRAM budget";
    }
#else
    report.estimatedEDRAMUsage = 0;
    report.warnings = "N/A: EDRAM only applies to Xbox 360";
#endif

    return report;
}

void GPUMemoryAuditor::PrintReport() {
    char buf[1024];
    OutputDebugStringA("\n=== GPU Memory Audit Report ===\n");

    sprintf(buf, "Resolution: %dx%d\n", m_screenWidth, m_screenHeight);
    OutputDebugStringA(buf);

    OutputDebugStringA("\n--- G-Buffers ---\n");
    int totalGBufferKB = 0;
    for (int i = 0; i < m_gBufferCount; i++) {
        GPUTextureInfo& info = m_gBuffers[i];
        sprintf(buf, "  %s: %dx%d %s (%d KB)\n",
                info.name, info.width, info.height,
                GetFormatName(info.format), info.totalBytes / 1024);
        OutputDebugStringA(buf);
        totalGBufferKB += info.totalBytes / 1024;
    }
    sprintf(buf, "  TOTAL: %d KB (%d buffers)\n\n", totalGBufferKB, m_gBufferCount);
    OutputDebugStringA(buf);

    OutputDebugStringA("\n--- Render Targets ---\n");
    for (int i = 0; i < m_rtCount; i++) {
        GPUTextureInfo& info = m_renderTargets[i];
        sprintf(buf, "  %s: %dx%d %s (%d KB)\n",
                info.name, info.width, info.height,
                GetFormatName(info.format), info.totalBytes / 1024);
        OutputDebugStringA(buf);
    }

    OutputDebugStringA("\n--- Depth Stencil ---\n");
    for (int i = 0; i < m_depthStencilCount; i++) {
        GPUTextureInfo& info = m_depthStencil[i];
        sprintf(buf, "  %s: %dx%d %s (%d KB)\n",
                info.name, info.width, info.height,
                GetFormatName(info.format), info.totalBytes / 1024);
        OutputDebugStringA(buf);
    }

    GPUMemoryReport report = GenerateReport();

    OutputDebugStringA("\n--- Summary ---\n");
    sprintf(buf, "  Total GBuffer: %d KB\n", report.totalGBufferBytes / 1024);
    OutputDebugStringA(buf);
    sprintf(buf, "  Total RT: %d KB\n", report.totalRTBytes / 1024);
    OutputDebugStringA(buf);
    sprintf(buf, "  Total Depth: %d KB\n", report.depthStencilBytes / 1024);
    OutputDebugStringA(buf);
    sprintf(buf, "  GRAND TOTAL: %d KB (%.2f MB)\n", m_totalBytes / 1024, m_totalBytes / (1024.0f * 1024.0f));
    OutputDebugStringA(buf);

    sprintf(buf, "\n  Estimated bandwidth: %d MB/s @ 60fps\n", report.estimatedBandwidthMB);
    OutputDebugStringA(buf);

#ifdef _XBOX
    sprintf(buf, "  EDRAM usage: %d KB\n", report.estimatedEDRAMUsage / 1024);
    OutputDebugStringA(buf);
    sprintf(buf, "  EDRAM status: %s\n", report.warnings);
    OutputDebugStringA(buf);
#endif

    OutputDebugStringA("\n==============================\n\n");
}

}