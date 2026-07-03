#include "stdafx.h"
#include "GroundResourcePass.h"
#include "RenderGroundResource.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/TextureRegistry.h"
#include "../../Graphics/TextManager.h"
#include "../TextureSlots.h"

namespace Scene {

GroundResourcePass::GroundResourcePass(TextManager* textManager)
    : m_textManager(textManager)
    , m_atlasLoaded(false)
    , m_textureSlot(0)
{
    m_woodIcon.valid = false;
}

void GroundResourcePass::LoadAtlas()
{
    TextureRegistry& reg = TextureRegistry::instance();
    std::tr1::shared_ptr<SpriteAtlas> iconAtlas = reg.getAtlas("Icon");
    if (!iconAtlas) return;

    uint32_t idx = iconAtlas->GetIndex("r_exotic_wood");
    if (idx != 0xFFFFFFFF) {
        const SpriteRegion* region = iconAtlas->GetRegion(idx);
        if (region) {
            m_woodIcon.u0 = region->u0;
            m_woodIcon.v0 = region->v0;
            m_woodIcon.u1 = region->u1;
            m_woodIcon.v1 = region->v1;
            m_woodIcon.w  = static_cast<float>(region->width);
            m_woodIcon.h  = static_cast<float>(region->height);
            m_woodIcon.pivotX = region->pivotX;
            m_woodIcon.pivotY = region->pivotY;
            m_woodIcon.valid = true;
        }
    }

    m_atlasLoaded = true;
}

void GroundResourcePass::Execute(const RenderFrame& frame, const RenderContext& context, RenderCommandBuffer& buffer)
{
    const std::vector<RenderGroundResource>& resources = frame.groundResources;
    if (resources.empty()) return;

    if (!m_atlasLoaded) {
        LoadAtlas();
        if (!m_atlasLoaded) return;
    }

    if (!m_woodIcon.valid) return;

    for (size_t i = 0; i < resources.size(); ++i) {
        const RenderGroundResource& r = resources[i];
        if (r.visualOnly) continue;

        const int sx = r.transform.screenX;
        const int sy = r.transform.screenY;

        float iconSize = 24.0f;

        buffer.PushSprite(
            sx - static_cast<int>(iconSize * 0.5f),
            sy - static_cast<int>(iconSize) - 8,
            iconSize, iconSize,
            m_woodIcon.u0, m_woodIcon.v0, m_woodIcon.u1, m_woodIcon.v1,
            m_textureSlot,
            static_cast<uint16_t>(0.99f * 65535.0f),
            0xDCFFFFFF,                  // color: 220 alpha
            0xFFFF,                      // shaderId: default
            0xFF,                        // blendMode: default
            0xFF                         // layer: default
        );

        // Amount text overlay (pre-projected to textScreenX/Y)
        if (m_textManager && r.amount > 0) {
            char buf[16];
            _snprintf(buf, sizeof(buf), "%d", r.amount);
            m_textManager->DrawString(buf,
                static_cast<float>(r.textScreenX),
                static_cast<float>(r.textScreenY),
                D3DCOLOR_ARGB(255, 255, 255, 0), 0.08f);
        }
    }
}

} // namespace Scene
