#include "stdafx.h"
#include "WorkSitePass.h"
#include "RenderWorkSite.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/TextureRegistry.h"
#include "../../Graphics/SpriteAtlas.h"
#include "../../Graphics/RenderLayers.h"

namespace Scene {

WorkSitePass::WorkSitePass()
    : m_atlasLoaded(false)
    , m_textureSlot(0)
{
}

void WorkSitePass::LoadAtlas()
{
    std::tr1::shared_ptr<SpriteAtlas> atlas =
        TextureRegistry::instance().getAtlas("Buildings");
    m_atlasLoaded = (atlas.get() != NULL);
}

void WorkSitePass::Execute(const RenderFrame& frame, const RenderContext& context,
                            RenderCommandBuffer& buffer)
{
    const std::vector<RenderWorkSite>& sites = frame.workSites;
    if (sites.empty()) return;

    if (!m_atlasLoaded) {
        LoadAtlas();
        if (!m_atlasLoaded) return;
    }

    std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas =
        TextureRegistry::instance().getAtlas("Buildings");
    if (!buildingsAtlas) return;

    for (size_t i = 0; i < sites.size(); ++i) {
        const RenderWorkSite& ws = sites[i];
        if (ws.spriteIdx < 0) continue;

        const SpriteRegion* r = buildingsAtlas->GetRegion(
            static_cast<uint32_t>(ws.spriteIdx));
        if (!r) continue;

        int sx = ws.transform.screenX;
        int sy = ws.transform.screenY;

        buffer.PushSprite(
            sx - static_cast<int>(r->pivotX + 0.5f),
            sy - static_cast<int>(r->pivotY + 0.5f),
            static_cast<float>(r->width),
            static_cast<float>(r->height),
            r->u0, r->v0, r->u1, r->v1,
            m_textureSlot,
            static_cast<uint16_t>(ws.transform.depthLayer),
            0xFFFFFFFF,
            0xFFFF, 0xFF, LAYER_EFFECTS
        );
    }
}

} // namespace Scene
