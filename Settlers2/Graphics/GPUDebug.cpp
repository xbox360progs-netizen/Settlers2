#include "stdafx.h"
#include "GPUDebug.h"
#include <float.h>
#include <math.h>

#ifdef _DEBUG
static bool g_nanDetectionEnabled = true;
#else
static bool g_nanDetectionEnabled = false;
#endif

static IDirect3DDevice9* g_device = NULL;

IDirect3DDevice9* GetGlobalDevice() {
    return g_device;
}

void SetGlobalDevice(IDirect3DDevice9* device) {
    g_device = device;
}

bool IsNaN(float f) {
    return _isnan(f) != 0;
}

bool IsINF(float f) {
    return _finite(f) == 0;
}

bool IsValid(float f) {
    return _finite(f) != 0 && _isnan(f) == 0;
}

void NaNCheckFloat(const char* name, float value) {
#ifdef _DEBUG
    if (!g_nanDetectionEnabled) return;
    
    if (IsNaN(value)) {
        char buf[256];
        sprintf(buf, "[NaN] %s = NaN\n", name);
        ::OutputDebugStringA(buf);
    }
    else if (IsINF(value)) {
        char buf[256];
        sprintf(buf, "[NaN] %s = INF\n", name);
        ::OutputDebugStringA(buf);
    }
#endif
}

void NaNCheckVector3(const char* name, const float* v) {
#ifdef _DEBUG
    if (!g_nanDetectionEnabled || !v) return;
    
    if (IsNaN(v[0]) || IsNaN(v[1]) || IsNaN(v[2])) {
        char buf[256];
        sprintf(buf, "[NaN] %s = (%.3f, %.3f, %.3f) has NaN\n", name, v[0], v[1], v[2]);
        ::OutputDebugStringA(buf);
    }
    else if (IsINF(v[0]) || IsINF(v[1]) || IsINF(v[2])) {
        char buf[256];
        sprintf(buf, "[NaN] %s = (%.3f, %.3f, %.3f) has INF\n", name, v[0], v[1], v[2]);
        ::OutputDebugStringA(buf);
    }
#endif
}

void NaNCheckVector4(const char* name, const float* v) {
#ifdef _DEBUG
    if (!g_nanDetectionEnabled || !v) return;
    
    if (IsNaN(v[0]) || IsNaN(v[1]) || IsNaN(v[2]) || IsNaN(v[3])) {
        char buf[256];
        sprintf(buf, "[NaN] %s = (%.3f, %.3f, %.3f, %.3f) has NaN\n", name, v[0], v[1], v[2], v[3]);
        ::OutputDebugStringA(buf);
    }
    else if (IsINF(v[0]) || IsINF(v[1]) || IsINF(v[2]) || IsINF(v[3])) {
        char buf[256];
        sprintf(buf, "[NaN] %s = (%.3f, %.3f, %.3f, %.3f) has INF\n", name, v[0], v[1], v[2], v[3]);
        ::OutputDebugStringA(buf);
    }
#endif
}

void EnableNaNDetection(bool enable) {
    g_nanDetectionEnabled = enable;
}

bool IsNaNDetectionEnabled() {
    return g_nanDetectionEnabled;
}