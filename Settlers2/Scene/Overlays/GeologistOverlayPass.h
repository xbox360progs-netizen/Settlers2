#pragma once
#include "../Rendering/RenderPass.h"
#include <stdint.h>
#include <vector>
#include <string>

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

// Renders geologist overlays: mountain highlight, surveyed deposit icons,
// and working indicator. Reads RenderFrame.overlays with screen coords
// pre-computed by ProjectionSystem.
class GeologistOverlayPass : public RenderPass {
public:
    GeologistOverlayPass();
    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

    void SetTextureSlot(uint16_t slot) { m_textureSlot = slot; }

private:
    struct SpriteInfo {
        float u0, v0, u1, v1;
        float w, h;
    };

    bool        m_atlasLoaded;
    uint16_t    m_textureSlot;
    SpriteInfo  m_mountainSprite;    // generic yellow highlight quad
    SpriteInfo  m_workingSprite;     // icon_geologist_work

    struct DepositSprite {
        uint8_t resourceType;
        SpriteInfo sprite;
    };
    std::vector<DepositSprite> m_depositCache;

    void LoadAtlas();
    const SpriteInfo* GetDepositSprite(uint8_t resourceType);

    // Prevent copy
    GeologistOverlayPass(const GeologistOverlayPass&);
    GeologistOverlayPass& operator=(const GeologistOverlayPass&);
};

} // namespace Scene
