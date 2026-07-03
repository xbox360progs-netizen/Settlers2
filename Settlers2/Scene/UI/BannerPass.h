#pragma once
#include "../Rendering/RenderPass.h"
#include <stdint.h>

class TextManager;

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

// Renders notification banner sprite + status text.
// Reads RenderBanner DTO with pre-computed slide position and UV coords.
class BannerPass : public RenderPass {
public:
    explicit BannerPass(TextManager* textManager);
    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

    void SetTextureSlot(uint16_t slot) { m_textureSlot = slot; }

private:
    TextManager* m_textManager;
    uint16_t     m_textureSlot;
};

} // namespace Scene
