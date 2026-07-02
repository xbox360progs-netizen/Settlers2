#include "stdafx.h"
#include "RoadPreviewPass.h"
#include "RenderRoadPreview.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/TextureRegistry.h"
#include "../../Graphics/RenderLayers.h"

namespace Scene {

RoadPreviewPass::RoadPreviewPass()
    : m_atlasLoaded(false)
    , m_textureSlot(0)
    , m_alignOffsetX(0.0f)
{
}

void RoadPreviewPass::LoadAtlas()
{
    TextureRegistry& reg = TextureRegistry::instance();
    reg.getTextureOrLoad("streets");
    std::tr1::shared_ptr<SpriteAtlas> streetsAtlas = reg.getAtlas("streets");
    if (!streetsAtlas) return;

    const std::vector<uint32_t>* group = streetsAtlas->GetGroup("street_1");
    if (!group || group->empty()) return;

    uint32_t regionIdx = (*group)[0];
    const SpriteRegion* region = streetsAtlas->GetRegion(regionIdx);
    if (!region) return;

    m_tileSprite.u0 = region->u0;
    m_tileSprite.v0 = region->v0;
    m_tileSprite.u1 = region->u1;
    m_tileSprite.v1 = region->v1;
    m_tileSprite.w  = static_cast<float>(region->width);
    m_tileSprite.h  = static_cast<float>(region->height);
    m_tileSprite.pivotX = region->pivotX;
    m_tileSprite.pivotY = region->pivotY;

    // Connection quads use the same texture region (stretched horizontally).
    m_connectionSprite = m_tileSprite;

    // Pre-compute flag alignment offset: aligns street sprite with flag pivot.
    {
        std::tr1::shared_ptr<SpriteAtlas> ba = reg.getAtlas("Buildings");
        if (ba) {
            uint32_t fi = ba->GetIndex("flag");
            const SpriteRegion* fr = ba->GetRegion(fi);
            if (fr) {
                m_alignOffsetX = (fr->width * 0.5f - fr->pivotX)
                    - (region->width * 0.5f - region->pivotX);
            }
        }
    }

    m_atlasLoaded = true;
}

void RoadPreviewPass::Execute(const RenderFrame& frame, const RenderContext& context, RenderCommandBuffer& buffer)
{
    const std::vector<RenderRoadSegment>& segments = frame.roadPreview;
    if (segments.empty()) return;

    if (!m_atlasLoaded) {
        LoadAtlas();
        if (!m_atlasLoaded) return;
    }

    // Shared depth for all road preview elements.
    static const uint16_t kDepth = static_cast<uint16_t>(0.98f * 65535.0f);

    for (size_t i = 0; i < segments.size(); ++i) {
        const RenderRoadSegment& seg = segments[i];
        uint32_t color = seg.valid
            ? 0xA0FFFFFF   // semi-transparent white (path)
            : 0x78FF6464;  // semi-transparent red (neighbor hint)

        bool isConnection = (seg.screenX0 != seg.screenX1 || seg.screenY0 != seg.screenY1);

        if (isConnection) {
            // Horizontal connection quad between two tiles.
            int sx = (std::min)(seg.screenX0, seg.screenX1) + static_cast<int>(m_alignOffsetX);
            int sy = (seg.screenY0 + seg.screenY1) / 2 - 3;
            float w = static_cast<float>(abs(seg.screenX1 - seg.screenX0));
            float h = 6.0f;

            buffer.PushSprite(
                sx, sy, w, h,
                m_connectionSprite.u0, m_connectionSprite.v0,
                m_connectionSprite.u1, m_connectionSprite.v1,
                m_textureSlot, kDepth, color,
                0xFFFF, 0xFF, LAYER_FOREGROUND
            );
        } else {
            // Single tile sprite.
            int sx = seg.screenX0 - static_cast<int>(m_tileSprite.pivotX)
                + static_cast<int>(m_alignOffsetX);
            int sy = seg.screenY0 - static_cast<int>(m_tileSprite.pivotY);

            buffer.PushSprite(
                sx, sy,
                m_tileSprite.w, m_tileSprite.h,
                m_tileSprite.u0, m_tileSprite.v0,
                m_tileSprite.u1, m_tileSprite.v1,
                m_textureSlot, kDepth, color,
                0xFFFF, 0xFF, LAYER_FOREGROUND
            );
        }
    }
}

} // namespace Scene
