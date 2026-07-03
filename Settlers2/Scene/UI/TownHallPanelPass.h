#pragma once
#include "../Rendering/RenderPass.h"
#include <stdint.h>

class TextManager;

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

class TownHallPanelPass : public RenderPass {
public:
    explicit TownHallPanelPass(TextManager* textManager);
    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

    void SetTextureSlot(uint16_t slot) { m_textureSlot = slot; }

private:
    uint16_t    m_textureSlot;
    TextManager* m_textManager;

    static const char* kStockNames[6];
    static const int   kStockTypes[6];

    TownHallPanelPass(const TownHallPanelPass&);
    TownHallPanelPass& operator=(const TownHallPanelPass&);
};

} // namespace Scene
