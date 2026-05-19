#include "stdafx.h"
#include "RenderGraph.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

RenderPass::RenderPass(const RenderPassDesc& desc)
    : m_name(desc.name ? desc.name : "")
    , m_executeFn(desc.executeFn)
    , m_enabled(desc.enabled)
    , m_priority(desc.priority)
{
}

RenderPass::~RenderPass() {
}

void RenderPass::Execute() {
    if (m_enabled && m_executeFn) {
        m_executeFn();
    }
}

RenderGraph::RenderGraph()
    : m_pDevice(NULL)
    , m_initialized(false)
    , m_width(0)
    , m_height(0)
{
}

RenderGraph::~RenderGraph() {
    Shutdown();
}

void RenderGraph::Initialize(IDirect3DDevice9* pDevice) {
    m_pDevice = pDevice;
    m_initialized = true;
    OutputDebugStringA("[RenderGraph] Initialized\n");
}

void RenderGraph::Shutdown() {
    Clear();
    m_pDevice = NULL;
    m_initialized = false;
}

int RenderGraph::AddPass(const RenderPassDesc& desc) {
    RenderPass* pass = new RenderPass(desc);
    m_passes.push_back(pass);
    
    char buf[128];
    sprintf(buf, "[RenderGraph] Added pass: %s\n", desc.name ? desc.name : "unnamed");
    OutputDebugStringA(buf);
    
    return (int)m_passes.size() - 1;
}

void RenderGraph::RemovePass(int id) {
    if (id >= 0 && id < (int)m_passes.size()) {
        delete m_passes[id];
        m_passes.erase(m_passes.begin() + id);
    }
}

void RenderGraph::RemovePass(const char* name) {
    for (size_t i = 0; i < m_passes.size(); i++) {
        if (m_passes[i]->GetName() == name) {
            delete m_passes[i];
            m_passes.erase(m_passes.begin() + i);
            return;
        }
    }
}

void RenderGraph::Execute() {
    if (!m_initialized) return;
    
    OutputDebugStringA("[RenderGraph] Execute()\n");
    
    for (size_t i = 0; i < m_passes.size(); i++) {
        m_passes[i]->Execute();
    }
    
    OutputDebugStringA("[RenderGraph] Execute() done\n");
}

void RenderGraph::SetPassEnabled(int id, bool enabled) {
    if (id >= 0 && id < (int)m_passes.size()) {
        m_passes[id]->SetEnabled(enabled);
    }
}

void RenderGraph::SetPassEnabled(const char* name, bool enabled) {
    for (size_t i = 0; i < m_passes.size(); i++) {
        if (m_passes[i]->GetName() == name) {
            m_passes[i]->SetEnabled(enabled);
            return;
        }
    }
}

void RenderGraph::Clear() {
    for (size_t i = 0; i < m_passes.size(); i++) {
        delete m_passes[i];
    }
    m_passes.clear();
}

RenderPass* RenderGraph::GetPass(int id) {
    if (id >= 0 && id < (int)m_passes.size()) {
        return m_passes[id];
    }
    return NULL;
}

void RenderGraph::OnResize(int width, int height) {
    m_width = width;
    m_height = height;
    OutputDebugStringA("[RenderGraph] Resized\n");
}

}