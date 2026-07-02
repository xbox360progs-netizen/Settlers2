#pragma once

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

// Abstract base for a single render pass.
// Each pass reads from RenderFrame (+ RenderContext) and pushes commands
// to the CommandBuffer.  Passes are registered in RenderGraph and executed
// in registration order.
// Invariant: Execute() is a pure function of its inputs — no globals,
// no simulation manager access, no side effects outside buffer.
class RenderPass {
public:
    virtual ~RenderPass() {}
    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer) = 0;
};

}
