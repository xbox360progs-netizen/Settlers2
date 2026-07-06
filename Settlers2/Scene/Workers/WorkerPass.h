#pragma once
#include "../Rendering/RenderPass.h"
#include <stdint.h>

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

// Renders all worker types (carriers, builders, building workers).
// Reads RenderFrame.workers — projected screen coords + visual state.
// Resolves sprite indices from worker type/state/direction/profession.
// Data comes from BuildingWorkerPresentation + CarrierPresentation.
class WorkerPass : public RenderPass {
public:
    WorkerPass();
    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

    void SetUnitSlot(uint16_t slot) { m_unitSlot = slot; }
    void SetIconSlot(uint16_t slot) { m_iconSlot = slot; }

private:
    int ResolveSpriteIndex(const struct RenderWorker& w) const;

    uint16_t m_unitSlot;
    uint16_t m_iconSlot;
};

} // namespace Scene
