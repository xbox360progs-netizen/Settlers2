#pragma once
#include <d3d9.h>

namespace Graphics {

struct GBufferInfo {
    int width;
    int height;
    D3DFORMAT format;
    const char* name;
    float minValue;
    float maxValue;
    float avgValue;
    int zeroPixels;
    int nanPixels;
    int infPixels;
};

class GBufferValidator {
public:
    GBufferValidator();

    bool ValidateAll(IDirect3DSurface9* pos, IDirect3DSurface9* normal,
                    IDirect3DSurface9* albedo, IDirect3DSurface9* spec,
                    IDirect3DSurface9* depth);

    bool ValidateAlbedo(IDirect3DSurface9* albedo);
    bool ValidateNormals(IDirect3DSurface9* normal);
    bool ValidateDepth(IDirect3DSurface9* depth);
    bool ValidateSpecular(IDirect3DSurface9* spec);

    void EnableValidation(bool enable) { m_enabled = enable; }
    bool IsEnabled() const { return m_enabled; }

    void SetVerbose(bool verbose) { m_verbose = verbose; }

    int GetErrorCount() const { return m_errors; }
    int GetWarningCount() const { return m_warnings; }
    void ResetCounters();

    const char* GetLastError() const { return m_lastError.c_str(); }

private:
    bool m_enabled;
    bool m_verbose;
    int m_errors;
    int m_warnings;
    std::string m_lastError;

    bool ValidateSurfaceBasic(IDirect3DSurface9* surface, const char* name);
    bool ValidateAlbedoData(IDirect3DSurface9* surface);
    bool ValidateNormalData(IDirect3DSurface9* surface);
    bool ValidateDepthData(IDirect3DSurface9* surface);
    bool ValidateSpecularData(IDirect3DSurface9* surface);

    void LogError(const char* msg);
    void LogWarning(const char* msg);
    void LogInfo(const char* msg);

    GBufferInfo m_info;
};

}