#pragma once
#include "../Rendering/RenderPass.h"
#include <vector>
#include <stdint.h>

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

// Renders resource icon overlays on flags.
// Reads RenderFrame.flagResources (screen coords pre-computed by ProjectionSystem)
// and pushes screen-space sprites to CommandBuffer.
// Caches icon sprite regions from the Icon atlas on first execution.
class FlagResourcePass : public RenderPass {
public:
    FlagResourcePass();
    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

    // Set the icon texture slot (called by GameRenderer each frame).
    void SetTextureSlot(uint16_t slot) { m_textureSlot = slot; }

private:
    struct IconSprite {
        float u0, v0, u1, v1;
        float w, h;
        float pivotX, pivotY;
        bool  valid;
    };

    bool                 m_atlasLoaded;
    uint16_t             m_textureSlot;
    std::vector<IconSprite> m_iconCache;  // indexed by ResourceType enum

    void LoadAtlas();
};

} // namespace Scene
