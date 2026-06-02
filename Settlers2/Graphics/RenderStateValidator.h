#pragma once
#include <d3d9.h>
#include <string>

IDirect3DDevice9* GetDirect3DDevice9();

namespace Graphics {

class RenderStateValidator {
public:
    RenderStateValidator();

    void Validate(const char* passName);
    void ValidateGBufferBindings();
    void ValidateLightingPass();

    void CheckDepthState(const char* pass);
    void CheckBlendState(const char* pass);
    void CheckRasterizerState(const char* pass);
    void CheckViewport(const char* pass);
    void CheckRenderTargets(const char* pass);
    void CheckShaders(const char* pass);
    void CheckSamplers(const char* pass);

    void EnableValidation(bool enable) { m_validationEnabled = enable; }
    bool IsValidationEnabled() const { return m_validationEnabled; }

    int GetErrorCount() const { return m_errorCount; }
    int GetWarningCount() const { return m_warningCount; }
    void ResetCounters();

    void LogState(const char* context);

private:
    bool m_validationEnabled;
    int m_errorCount;
    int m_warningCount;

    struct StateSnapshot {
        DWORD zEnable;
        DWORD zWriteEnable;
        DWORD alphaBlendEnable;
        DWORD srcBlend;
        DWORD destBlend;
        DWORD cullMode;
        int rtCount;
    };

    StateSnapshot m_lastSnapshot;
    std::string m_lastPass;

    void CaptureSnapshot(StateSnapshot& out);
    bool ValidateSnapshot(const StateSnapshot& snap, const char* pass);
    void LogMismatch(const char* state, const char* expected, const char* actual, const char* pass);
};

}
