#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include <string>

namespace Graphics {

class IDirect3DDevice9;
class GPUTimer;

enum class PostFXType {
    BLOOM,
    FOG,
    TONEMAP,
    SSAO,
    COLOR_GRADE,
    VIGNETTE,
    DEPTH_OF_FIELD
};

struct PostFXParams {
    PostFXType type;
    float intensity;
    float params[4];
    bool enabled;

    PostFXParams()
        : type(PostFXType::BLOOM), intensity(1.0f), enabled(true) {
        params[0] = params[1] = params[2] = params[3] = 0.0f;
    }

    PostFXParams(PostFXType t, float i = 1.0f)
        : type(t), intensity(i), enabled(true) {
        params[0] = params[1] = params[2] = params[3] = 0.0f;
    }
};

class IPostFXPass {
public:
    virtual ~IPostFXPass() {}

    virtual const char* GetName() const = 0;
    virtual PostFXType GetType() const = 0;
    virtual bool IsEnabled() const = 0;
    virtual void SetEnabled(bool enabled) = 0;

    virtual void Initialize(IDirect3DDevice9* device) = 0;
    virtual void Shutdown() = 0;

    virtual void SetParameters(const PostFXParams& params) = 0;
    virtual const PostFXParams& GetParameters() const = 0;

    virtual void Execute() = 0;

protected:
    IPostFXPass(PostFXType type) : m_type(type), m_enabled(true) {}

    PostFXType m_type;
    bool m_enabled;
};

class PostFXChain
{
public:
    PostFXChain();
    ~PostFXChain();

    void Initialize(IDirect3DDevice9* device, GPUTimer* timer);
    void Shutdown();

    IPostFXPass* AddPass(PostFXType type, const PostFXParams& params = PostFXParams());
    void RemovePass(PostFXType type);
    void RemovePass(const char* name);
    IPostFXPass* GetPass(PostFXType type);
    IPostFXPass* GetPass(const char* name);

    void SetEnabled(PostFXType type, bool enabled);
    void SetParameters(PostFXType type, const PostFXParams& params);

    void Execute();

    int GetPassCount() const { return (int)m_passes.size(); }
    bool IsEmpty() const { return m_passes.empty(); }

    void SetInputTexture(IDirect3DTexture9* texture);
    IDirect3DTexture9* GetInputTexture() const { return m_inputTexture; }

    IDirect3DTexture9* GetOutputTexture() const { return m_outputTexture; }

    void SetDebugOutput(bool enable) { m_debugOutput = enable; }
    bool IsDebugOutputEnabled() const { return m_debugOutput; }

private:
    void RenderFullscreenQuad();

    IDirect3DDevice9* m_device;
    GPUTimer* m_gpuTimer;

    std::vector<IPostFXPass*> m_passes;
    std::vector<PostFXParams> m_params;

    IDirect3DTexture9* m_inputTexture;
    IDirect3DTexture9* m_outputTexture;
    IDirect3DSurface9* m_outputSurface;

    bool m_debugOutput;
    int m_gpuTimerIndex;
};

}