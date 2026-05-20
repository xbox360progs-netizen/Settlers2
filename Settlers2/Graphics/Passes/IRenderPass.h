#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>

namespace Graphics {

enum RenderPassType {
    PASS_GEOMETRY,
    PASS_LIGHTING,
    PASS_TRANSPARENT,
    PASS_UI,
    PASS_POSTFX,
    PASS_COUNT
};

class RenderContext;

class IRenderPass {
public:
    virtual ~IRenderPass() {}

    virtual const char* GetName() const = 0;
    virtual RenderPassType GetType() const = 0;
    virtual int GetPriority() const = 0;
    virtual bool IsEnabled() const = 0;
    virtual void SetEnabled(bool enabled) = 0;

    virtual void BeginPass() = 0;
    virtual void Execute() = 0;
    virtual void EndPass() = 0;

    virtual void SetContext(RenderContext* ctx) { m_context = ctx; }
    virtual RenderContext* GetContext() const { return m_context; }

protected:
    IRenderPass(const char* name, RenderPassType type, int priority)
        : m_name(name), m_type(type), m_priority(priority), m_enabled(true), m_context(NULL) {}

    std::string m_name;
    RenderPassType m_type;
    int m_priority;
    bool m_enabled;
    RenderContext* m_context;
};

}