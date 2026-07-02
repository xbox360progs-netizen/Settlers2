#pragma once
#include "../Rendering/RenderPass.h"
#include <vector>
#include <stdint.h>

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

// Renders wild animals (deer, rabbits, crocodiles, snakes).
// Reads RenderFrame.wildlife (screen coords pre-computed by ProjectionSystem)
// and pushes screen-space sprites to CommandBuffer.
// Caches the "Animals" group sprite regions from the Units atlas.
class WildlifePass : public RenderPass {
public:
    WildlifePass();
    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

    // Set the Units texture slot (called by GameRenderer each frame).
    void SetTextureSlot(uint16_t slot) { m_textureSlot = slot; }

private:
    struct AnimalSprite {
        float u0, v0, u1, v1;
        float w, h;
        float pivotX, pivotY;
    };

    bool            m_atlasLoaded;
    uint16_t        m_textureSlot;
    AnimalSprite    m_sprites[16];  // 4 types x 4 directions = 16 max

    void LoadAtlas();
};

} // namespace Scene
