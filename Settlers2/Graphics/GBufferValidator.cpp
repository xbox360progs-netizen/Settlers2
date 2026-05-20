#include "stdafx.h"
#include "GBufferValidator.h"
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

GBufferValidator::GBufferValidator()
    : m_enabled(true), m_verbose(false), m_errors(0), m_warnings(0) {
}

bool GBufferValidator::ValidateAll(IDirect3DSurface9* pos, IDirect3DSurface9* normal,
                                   IDirect3DSurface9* albedo, IDirect3DSurface9* spec,
                                   IDirect3DSurface9* depth) {
    bool result = true;

    OutputDebugStringA("[GBufferValidator] Starting full validation...\n");

    if (!ValidateAlbedo(albedo)) result = false;
    if (!ValidateNormals(normal)) result = false;
    if (!ValidateDepth(depth)) result = false;
    if (!ValidateSpecular(spec)) result = false;

    if (result) {
        OutputDebugStringA("[GBufferValidator] All GBuffer surfaces valid\n");
    }

    return result;
}

bool GBufferValidator::ValidateAlbedo(IDirect3DSurface9* albedo) {
    if (!albedo) {
        LogError("Albedo surface is NULL");
        return false;
    }

    if (!ValidateSurfaceBasic(albedo, "Albedo")) return false;
    if (!ValidateAlbedoData(albedo)) return false;

    LogInfo("Albedo validation passed");
    return true;
}

bool GBufferValidator::ValidateNormals(IDirect3DSurface9* normal) {
    if (!normal) {
        LogError("Normal surface is NULL");
        return false;
    }

    if (!ValidateSurfaceBasic(normal, "Normal")) return false;
    if (!ValidateNormalData(normal)) return false;

    LogInfo("Normal validation passed");
    return true;
}

bool GBufferValidator::ValidateDepth(IDirect3DSurface9* depth) {
    if (!depth) {
        LogError("Depth surface is NULL");
        return false;
    }

    if (!ValidateDepthData(depth)) return false;

    LogInfo("Depth validation passed");
    return true;
}

bool GBufferValidator::ValidateSpecular(IDirect3DSurface9* spec) {
    if (!spec) {
        LogError("Specular surface is NULL");
        return false;
    }

    if (!ValidateSurfaceBasic(spec, "Specular")) return false;
    if (!ValidateSpecularData(spec)) return false;

    LogInfo("Specular validation passed");
    return true;
}

bool GBufferValidator::ValidateSurfaceBasic(IDirect3DSurface9* surface, const char* name) {
    D3DSURFACE_DESC desc;
    HRESULT hr = surface->GetDesc(&desc);

    if (FAILED(hr)) {
        char buf[256];
        sprintf(buf, "[GBufferValidator] ERROR: Failed to get desc for %s\n", name);
        LogError(buf);
        return false;
    }

    if (desc.Width == 0 || desc.Height == 0) {
        char buf[256];
        sprintf(buf, "[GBufferValidator] ERROR: %s has invalid dimensions %dx%d\n",
                name, desc.Width, desc.Height);
        LogError(buf);
        return false;
    }

    if (m_verbose) {
        char buf[256];
        sprintf(buf, "[GBufferValidator] %s: %dx%d, Format=%d\n",
                name, desc.Width, desc.Height, desc.Format);
        LogInfo(buf);
    }

    return true;
}

bool GBufferValidator::ValidateAlbedoData(IDirect3DSurface9* surface) {
    D3DSURFACE_DESC desc;
    surface->GetDesc(&desc);

    D3DLOCKED_RECT lockRect;
    HRESULT hr = surface->LockRect(&lockRect, NULL, D3DLOCK_READONLY);

    if (FAILED(hr)) {
        LogError("Failed to lock albedo surface for reading");
        return false;
    }

    int width = desc.Width;
    int height = desc.Height;
    int pitch = lockRect.Pitch;
    DWORD* pixels = (DWORD*)lockRect.pBits;

    int zeroPixels = 0;
    int fullyOpaque = 0;
    int fullyTransparent = 0;

    int sampleStep = 16;
    int samples = 0;
    float avgBrightness = 0;

    for (int y = 0; y < height; y += sampleStep) {
        for (int x = 0; x < width; x += sampleStep) {
            int offset = (y * pitch / 4) + x;
            DWORD pixel = pixels[offset];

            BYTE a = (pixel >> 24) & 0xFF;
            BYTE r = (pixel >> 16) & 0xFF;
            BYTE g = (pixel >> 8) & 0xFF;
            BYTE b = pixel & 0xFF;

            if (a == 0) {
                zeroPixels++;
                fullyTransparent++;
            } else if (a == 255) {
                fullyOpaque++;
            }

            float brightness = (r + g + b) / 3.0f / 255.0f;
            avgBrightness += brightness;
            samples++;
        }
    }

    surface->UnlockRect();

    avgBrightness /= samples;

    char buf[512];
    sprintf(buf, "[GBufferValidator] Albedo stats: zero=%d, transparent=%d, opaque=%d, avgBright=%.2f\n",
            zeroPixels, fullyTransparent, fullyOpaque, avgBrightness);
    OutputDebugStringA(buf);

    if (zeroPixels > (width / sampleStep) * (height / sampleStep) * 0.9f) {
        LogWarning("Albedo: >90% pixels have alpha=0 - GBuffer may not be filled correctly");
        return false;
    }

    if (avgBrightness < 0.01f) {
        LogWarning("Albedo: Very low average brightness - GBuffer may be empty");
    }

    return true;
}

bool GBufferValidator::ValidateNormalData(IDirect3DSurface9* surface) {
    D3DSURFACE_DESC desc;
    surface->GetDesc(&desc);

    D3DLOCKED_RECT lockRect;
    HRESULT hr = surface->LockRect(&lockRect, NULL, D3DLOCK_READONLY);

    if (FAILED(hr)) {
        LogError("Failed to lock normal surface for reading");
        return false;
    }

    int width = desc.Width;
    int height = desc.Height;
    int pitch = lockRect.Pitch;

    int sampleStep = 16;
    int samples = 0;
    int nanCount = 0;
    int zeroNormalCount = 0;

    int formatBytes = (desc.Format == D3DFMT_A16B16G16R16F) ? 8 : 4;
    WORD* pixels = (WORD*)lockRect.pBits;

    for (int y = 0; y < height; y += sampleStep) {
        for (int x = 0; x < width; x += sampleStep) {
            int offset = (y * pitch / formatBytes) + x * 4;

            float nx = ((pixels[offset + 0] / 65535.0f) - 0.5f) * 2.0f;
            float ny = ((pixels[offset + 1] / 65535.0f) - 0.5f) * 2.0f;
            float nz = ((pixels[offset + 2] / 65535.0f) - 0.5f) * 2.0f;

            if (_isnan(nx) || _isnan(ny) || _isnan(nz)) {
                nanCount++;
            }

            float len = sqrtf(nx * nx + ny * ny + nz * nz);
            if (len < 0.01f) {
                zeroNormalCount++;
            }

            samples++;
        }
    }

    surface->UnlockRect();

    char buf[256];
    sprintf(buf, "[GBufferValidator] Normal stats: samples=%d, nan=%d, zeroNormals=%d\n",
            samples, nanCount, zeroNormalCount);
    OutputDebugStringA(buf);

    if (nanCount > samples * 0.1f) {
        LogWarning("Normal: >10% NaN values detected");
    }

    return true;
}

bool GBufferValidator::ValidateDepthData(IDirect3DSurface9* surface) {
    D3DSURFACE_DESC desc;
    surface->GetDesc(&desc);

    D3DLOCKED_RECT lockRect;
    HRESULT hr = surface->LockRect(&lockRect, NULL, D3DLOCK_READONLY);

    if (FAILED(hr)) {
        LogError("Failed to lock depth surface for reading");
        return false;
    }

    int width = desc.Width;
    int height = desc.Height;
    int pitch = lockRect.Pitch;

    int sampleStep = 16;
    int samples = 0;
    float minDepth = 1.0f;
    float maxDepth = 0.0f;
    float avgDepth = 0.0f;
    int nanCount = 0;

    float* depths = (float*)lockRect.pBits;
    int stride = pitch / 4;

    for (int y = 0; y < height; y += sampleStep) {
        for (int x = 0; x < width; x += sampleStep) {
            float depth = depths[y * stride + x];

            if (_isnan(depth) || (!_finite(depth) && !_isnan(depth))) {
                nanCount++;
            } else {
                minDepth = (depth < minDepth) ? depth : minDepth;
                maxDepth = (depth > maxDepth) ? depth : maxDepth;
                avgDepth += depth;
            }
            samples++;
        }
    }

    surface->UnlockRect();

    avgDepth /= samples;

    char buf[512];
    sprintf(buf, "[GBufferValidator] Depth stats: min=%.4f, max=%.4f, avg=%.4f, nan=%d\n",
            minDepth, maxDepth, avgDepth, nanCount);
    OutputDebugStringA(buf);

    if (minDepth == 1.0f && maxDepth == 1.0f && avgDepth == 1.0f) {
        LogWarning("Depth: All values are 1.0 - depth buffer may not be written");
    }

    if (nanCount > samples * 0.05f) {
        LogWarning("Depth: >5% NaN/Infinity values detected");
    }

    return true;
}

bool GBufferValidator::ValidateSpecularData(IDirect3DSurface9* surface) {
    D3DSURFACE_DESC desc;
    surface->GetDesc(&desc);

    D3DLOCKED_RECT lockRect;
    HRESULT hr = surface->LockRect(&lockRect, NULL, D3DLOCK_READONLY);

    if (FAILED(hr)) {
        LogError("Failed to lock specular surface for reading");
        return false;
    }

    int width = desc.Width;
    int height = desc.Height;
    int pitch = lockRect.Pitch;

    int sampleStep = 16;
    int samples = 0;
    float avgGloss = 0;
    int zeroSpec = 0;

    DWORD* pixels = (DWORD*)lockRect.pBits;

    for (int y = 0; y < height; y += sampleStep) {
        for (int x = 0; x < width; x += sampleStep) {
            int offset = (y * pitch / 4) + x;
            DWORD pixel = pixels[offset];

            BYTE a = (pixel >> 24) & 0xFF;
            BYTE r = (pixel >> 16) & 0xFF;
            BYTE g = (pixel >> 8) & 0xFF;
            BYTE b = pixel & 0xFF;

            float specMagnitude = (r + g + b) / 3.0f / 255.0f;
            float gloss = a / 255.0f;

            avgGloss += gloss;
            if (specMagnitude < 0.01f && gloss < 0.01f) {
                zeroSpec++;
            }

            samples++;
        }
    }

    surface->UnlockRect();

    avgGloss /= samples;

    char buf[256];
    sprintf(buf, "[GBufferValidator] Specular stats: avgGloss=%.2f, zeroSpec=%d\n",
            avgGloss, zeroSpec);
    OutputDebugStringA(buf);

    return true;
}

void GBufferValidator::LogError(const char* msg) {
    char buf[512];
    sprintf(buf, "[GBufferValidator] ERROR: %s\n", msg);
    OutputDebugStringA(buf);
    m_errors++;
    m_lastError = msg;
}

void GBufferValidator::LogWarning(const char* msg) {
    char buf[512];
    sprintf(buf, "[GBufferValidator] WARNING: %s\n", msg);
    OutputDebugStringA(buf);
    m_warnings++;
}

void GBufferValidator::LogInfo(const char* msg) {
    if (m_verbose) {
        char buf[512];
        sprintf(buf, "[GBufferValidator] INFO: %s\n", msg);
        OutputDebugStringA(buf);
    }
}

void GBufferValidator::ResetCounters() {
    m_errors = 0;
    m_warnings = 0;
    m_lastError.clear();
}

}