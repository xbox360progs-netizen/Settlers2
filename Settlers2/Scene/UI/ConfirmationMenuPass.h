#pragma once
#include "../Rendering/RenderPass.h"
#include <stdint.h>

class TextManager;

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

class ConfirmationMenuPass : public RenderPass {
public:
    explicit ConfirmationMenuPass(TextManager* textManager);
    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

    void SetBgSlot(uint16_t slot) { m_bgSlot = slot; }
    void SetIconSlot(uint16_t slot) { m_iconSlot = slot; }

private:
    struct SpriteSlot { float u0, v0, u1, v1; float w, h; };

    bool        m_atlasLoaded;
    uint16_t    m_bgSlot;
    uint16_t    m_iconSlot;
    TextManager* m_textManager;
    SpriteSlot  m_bgPanel;       // menu panel background
    SpriteSlot  m_iconMountain;
    SpriteSlot  m_iconGeologist;
    SpriteSlot  m_ornament;

    void LoadAtlas();

    ConfirmationMenuPass(const ConfirmationMenuPass&);
    ConfirmationMenuPass& operator=(const ConfirmationMenuPass&);
};

} // namespace Scene
