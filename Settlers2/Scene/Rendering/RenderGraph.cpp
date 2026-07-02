#include "stdafx.h"
#include "RenderGraph.h"
#include "RenderContext.h"
#include "../Shared/RenderFrame.h"

namespace Scene {

RenderGraph::RenderGraph()
{
}

RenderGraph::~RenderGraph()
{
    m_passes.clear();
}

void RenderGraph::AddPass(RenderPass* pass)
{
    if (pass) {
        m_passes.push_back(pass);
    }
}

void RenderGraph::Execute(const RenderFrame& frame, const RenderContext& context,
                          RenderCommandBuffer& buffer)
{
    for (size_t i = 0; i < m_passes.size(); ++i) {
        m_passes[i]->Execute(frame, context, buffer);
    }
}

}
