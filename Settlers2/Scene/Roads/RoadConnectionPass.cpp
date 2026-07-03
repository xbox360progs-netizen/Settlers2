#include "stdafx.h"
#include "RoadConnectionPass.h"
#include "RenderRoadConnection.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/TextureRegistry.h"
#include "../../Graphics/RenderLayers.h"

namespace Scene {

RoadConnectionPass::RoadConnectionPass()
    : m_atlasLoaded(false)
    , m_textureSlot(0)
{
}

void RoadConnectionPass::LoadAtlas()
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

    m_sprite.u0 = region->u0;
    m_sprite.v0 = region->v0;
    m_sprite.u1 = region->u1;
    m_sprite.v1 = region->v1;
    m_sprite.w  = static_cast<float>(region->width);
    m_sprite.h  = static_cast<float>(region->height);
    m_sprite.pivotX = region->pivotX;
    m_sprite.pivotY = region->pivotY;

    m_atlasLoaded = true;
}

void RoadConnectionPass::Execute(const RenderFrame& frame, const RenderContext& context,
                                  RenderCommandBuffer& buffer)
{
    const std::vector<RenderRoadConnection>& connections = frame.roadConnections;
    if (connections.empty()) return;

    if (!m_atlasLoaded) {
        LoadAtlas();
        if (!m_atlasLoaded) return;
    }

    for (size_t i = 0; i < connections.size(); ++i) {
        const RenderRoadConnection& seg = connections[i];

        int sx = (std::min)(seg.screenX0, seg.screenX1);
        int sy = (seg.screenY0 + seg.screenY1) / 2 - 3;
        float w = static_cast<float>(abs(seg.screenX1 - seg.screenX0));
        float h = 6.0f;

        buffer.PushSprite(
            sx, sy, w, h,
            m_sprite.u0, m_sprite.v0,
            m_sprite.u1, m_sprite.v1,
            m_textureSlot, seg.depthLayer, 0xFFFFFFFF,
            0xFFFF, 0xFF, LAYER_EFFECTS
        );
    }
}

} // namespace Scene
