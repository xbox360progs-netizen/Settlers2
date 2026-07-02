#pragma once
#include <vector>
#include "RenderPass.h"

namespace Scene {

struct RenderContext;

// Ordered container of render passes. GameRenderer registers passes
// during initialization and calls Execute once per frame. The graph
// owns neither the passes nor the CommandBuffer — the caller manages
// both lifetimes.
class RenderGraph {
public:
    RenderGraph();
    ~RenderGraph();

    // Register a pass. Passes execute in registration order.
    // Pass pointer must remain valid until Execute() completes.
    void AddPass(RenderPass* pass);

    // Execute all registered passes in order.
    void Execute(const RenderFrame& frame, const RenderContext& context,
                 RenderCommandBuffer& buffer);

private:
    std::vector<RenderPass*> m_passes;
};

}
