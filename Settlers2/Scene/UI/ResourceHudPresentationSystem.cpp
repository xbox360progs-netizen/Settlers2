#include "stdafx.h"
#include "ResourceHudPresentationSystem.h"
#include "../../Graphics/TextureRegistry.h"
#include "../../Graphics/SpriteAtlas.h"
#include "../../Logic/EconomyManager.h"
#include "../FrameContext.h"

namespace Scene {

void ResourceHudPresentationSystem::SetEconomyManager(Logic::EconomyManager* economy)
{
    m_economy = economy;
}

void ResourceHudPresentationSystem::BuildRenderFrame(
    const FrameContext& frame, RenderResourceHud& out)
{
    if (!frame.overlay.resourceHudLoaded || !m_economy) {
        out.loaded = false;
        out.count = 0;
        return;
    }

    std::tr1::shared_ptr<SpriteAtlas> iconAtlas =
        TextureRegistry::instance().getAtlas("Icon");

    out.loaded = true;
    out.count = 0;
    for (int i = 0; i < OverlayFrameState::RESOURCE_HUD_COUNT; ++i) {
        int iconIdx = frame.overlay.resourceHud[i].iconIdx;
        if (iconIdx < 0) continue;

        RenderResourceHudItem& item = out.items[out.count];
        item.iconIdx = iconIdx;
        item.stockCount = m_economy->GetTotalStock(
            frame.overlay.resourceHud[i].type);

        if (iconAtlas) {
            const SpriteRegion* r = iconAtlas->GetRegion(iconIdx);
            if (r) {
                item.u0 = r->u0; item.v0 = r->v0;
                item.u1 = r->u1; item.v1 = r->v1;
            }
        }

        out.count++;
    }
}

} // namespace Scene
