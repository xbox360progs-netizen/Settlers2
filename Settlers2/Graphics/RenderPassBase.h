#pragma once
#include <d3d9.h>
#include <string>
#include <vector>
#include "RenderFrame.h"

namespace Graphics {

enum PassResourceAccess {
    PASS_READ,
    PASS_WRITE,
    PASS_READ_WRITE,
    PASS_RESOURCE_RESOLVE
};

struct RenderTargetDesc {
    const char* name;
    int width;
    int height;
    D3DFORMAT format;
    bool depth;
    D3DPOOL pool;
    
    RenderTargetDesc() : name(NULL), width(0), height(0), format(D3DFMT_UNKNOWN), depth(false), pool(D3DPOOL_DEFAULT) {}
};

class IRenderPass {
public:
    virtual ~IRenderPass() {}
    
    virtual void BeginPass() = 0;
    virtual void Execute() = 0;
    virtual void EndPass() = 0;
    
    virtual const char* GetName() const = 0;
    virtual RenderPassType GetType() const = 0;
    virtual int GetPriority() const = 0;
    virtual bool IsEnabled() const = 0;
    
    virtual void SetEnabled(bool enabled) = 0;
    virtual void SetResources(int* readRTs, int readCount, int* writeRTs, int writeCount) = 0;
    
    virtual void AddGPUEvent() = 0;
    virtual void ValidateState() = 0;
};

class RenderPassBase : public IRenderPass {
public:
    RenderPassBase(const char* name, RenderPassType type, int priority);
    virtual ~RenderPassBase();
    
    virtual void BeginPass() override;
    virtual void Execute() override = 0;
    virtual void EndPass() override;
    
    virtual const char* GetName() const override { return m_name.c_str(); }
    virtual RenderPassType GetType() const override { return m_type; }
    virtual int GetPriority() const override { return m_priority; }
    virtual bool IsEnabled() const override { return m_enabled; }
    
    virtual void SetEnabled(bool enabled) override { m_enabled = enabled; }
    virtual void SetResources(int* readRTs, int readCount, int* writeRTs, int writeCount) override;
    virtual void AddGPUEvent() override;
    virtual void ValidateState() override;

protected:
    virtual void OnBeginPass() {}
    virtual void OnEndPass() {}
    virtual void OnValidateState() {}
    
    void SetReadRTs(int* rtIndices, int count);
    void SetWriteRTs(int* rtIndices, int count);
    
    std::string m_name;
    RenderPassType m_type;
    int m_priority;
    bool m_enabled;
    bool m_passActive;
    
    std::vector<int> m_readRTs;
    std::vector<int> m_writeRTs;
};

void ResetBlendState(IDirect3DDevice9* device);
void ResetDepthState(IDirect3DDevice9* device);
void ResetRasterizerState(IDirect3DDevice9* device);
void ResetSamplerStates(IDirect3DDevice9* device);
void ResetShaders(IDirect3DDevice9* device);
void ResetAllRenderStates(IDirect3DDevice9* device);

void ValidateRenderTargetBindings(IDirect3DDevice9* device, const char* passName);
void ValidateDepthState(const char* passName);
void ValidateBlendState(const char* passName);
void ValidateRasterizerState(const char* passName);

void SetGlobalValidator(class RenderStateValidator* validator);

}