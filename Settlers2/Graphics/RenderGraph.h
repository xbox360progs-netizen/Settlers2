#pragma once
#include <d3d9.h>
#include <vector>
#include <string>
#include <functional>

namespace Graphics {

class RenderPass;

typedef std::function<void()> PassExecuteFn;

struct RenderPassDesc {
    const char* name;
    PassExecuteFn executeFn;
    bool enabled;
    int priority;
    
    RenderPassDesc() : name(NULL), enabled(true), priority(0) {}
    RenderPassDesc(const char* n, PassExecuteFn fn, int p = 0) 
        : name(n), executeFn(fn), enabled(true), priority(p) {}
};

class RenderPass {
public:
    RenderPass(const RenderPassDesc& desc);
    ~RenderPass();

    const char* GetName() const { return m_name.c_str(); }
    void Execute();
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }
    void SetPriority(int priority) { m_priority = priority; }
    int GetPriority() const { return m_priority; }

private:
    std::string m_name;
    PassExecuteFn m_executeFn;
    bool m_enabled;
    int m_priority;
};

class RenderGraph {
public:
    RenderGraph();
    ~RenderGraph();

    void Initialize(IDirect3DDevice9* pDevice);
    void Shutdown();

    int AddPass(const RenderPassDesc& desc);
    void RemovePass(int id);
    void RemovePass(const char* name);

    void Execute();

    void SetPassEnabled(int id, bool enabled);
    void SetPassEnabled(const char* name, bool enabled);

    void Clear();

    int GetPassCount() const { return (int)m_passes.size(); }
    RenderPass* GetPass(int id);

    void OnResize(int width, int height);

private:
    IDirect3DDevice9* m_pDevice;
    std::vector<RenderPass*> m_passes;
    bool m_initialized;
    int m_width;
    int m_height;
};

}