#pragma once
#include "../Rendering/RenderPass.h"
#include <stdint.h>

class TextManager;

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

class HuntingSpotPass : public RenderPass {
public:
    explicit HuntingSpotPass(TextManager* textManager);
    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

    void SetTextureSlot(uint16_t slot) { m_textureSlot = slot; }

private:
    bool        m_atlasLoaded;
    uint16_t    m_textureSlot;
    TextManager* m_textManager;
    struct SpriteInfo { float u0, v0, u1, v1; float w, h; };
    SpriteInfo  m_deerSprite;

    void LoadAtlas();

    HuntingSpotPass(const HuntingSpotPass&);
    HuntingSpotPass& operator=(const HuntingSpotPass&);
};

} // namespace Scene
