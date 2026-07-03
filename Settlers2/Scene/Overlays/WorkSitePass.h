#pragma once
#include "../Rendering/RenderPass.h"
#include <stdint.h>

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

// Renders work-site sprites (e.g., mine framework at resource node).
// Reads RenderFrame.highlights (shared slot with building highlights).
// Caches the Buildings atlas for sprite resolution.
class WorkSitePass : public RenderPass {
public:
    WorkSitePass();
    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

    void SetTextureSlot(uint16_t slot) { m_textureSlot = slot; }

private:
    bool     m_atlasLoaded;
    uint16_t m_textureSlot;

    void LoadAtlas();
};

} // namespace Scene
