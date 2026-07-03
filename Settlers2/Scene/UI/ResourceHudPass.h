#pragma once
#include "../Rendering/RenderPass.h"
#include <stdint.h>

class TextManager;

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

// Renders the top resource HUD bar (icons + stock counts).
// Receives RenderResourceHud data with pre-resolved icon UVs and stock counts.
class ResourceHudPass : public RenderPass {
public:
    explicit ResourceHudPass(TextManager* textManager);
    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

    void SetTextureSlot(uint16_t slot) { m_textureSlot = slot; }

private:
    TextManager* m_textManager;
    uint16_t     m_textureSlot;
};

} // namespace Scene
