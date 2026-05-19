#pragma once

#define RENDER_PIPELINE_FROZEN 1

namespace Graphics {

class PipelineEnforcer {
public:
    static bool IsFrozen() {
    #ifdef RENDER_PIPELINE_FROZEN
        return true;
    #else
        return false;
    #endif
    }
    
    static bool IsDirectBackbufferAllowed() {
        return !IsFrozen();
    }
    
    static bool IsNewRenderPathAllowed() {
        return !IsFrozen();
    }
    
    static void CheckBackbufferAccess(const char* caller) {
        if (IsFrozen() && IsDirectBackbufferAllowed() == false) {
            ReportViolation(caller, "Direct backbuffer access forbidden");
        }
    }
    
    static void CheckRenderPath(const char* caller, const char* pathName) {
        if (IsFrozen() && !IsNewRenderPathAllowed()) {
            ReportViolation(caller, pathName);
        }
    }
    
    static void ReportViolation(const char* context, const char* message) {
    #ifdef _DEBUG
        char buf[256];
        sprintf(buf, "[PIPELINE VIOLATION] %s: %s\n", context, message);
        ::OutputDebugStringA(buf);
    #endif
    }
};

}