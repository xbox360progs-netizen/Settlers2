#include "stdafx.h"
#include "BuildingHighlightPass.h"
#include "../Shared/RenderFrame.h"
#include "../Shared/RenderBuildingHighlight.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/TextureRegistry.h"
#include "../../Graphics/SpriteAtlas.h"
#include "../../Graphics/RenderLayers.h"
#include "../BuildingPlacement.h"

namespace Scene {

BuildingHighlightPass::BuildingHighlightPass()
    : m_atlasLoaded(false)
    , m_textureSlot(0)
{
}

void BuildingHighlightPass::LoadAtlas()
{
    TextureRegistry& reg = TextureRegistry::instance();
    reg.getTextureOrLoad("Buildings");
    std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
    if (!buildingsAtlas) return;

    // Pre-cache sprite info for all building types.
    for (int bt = 1; bt < 64; ++bt) {
        World::BuildingType type = static_cast<World::BuildingType>(bt);
        const char* spriteName = BuildingPlacementManager::GetBuildingSpriteName(type);
        if (!spriteName || !*spriteName) continue;

        uint32_t idx = buildingsAtlas->GetIndex(spriteName);
        if (idx == 0xFFFFFFFF) {
            // Try lowercase fallback.
            std::string lower = spriteName;
            for (size_t ci = 0; ci < lower.size(); ++ci)
                if (lower[ci] >= 'A' && lower[ci] <= 'Z')
                    lower[ci] = lower[ci] - 'A' + 'a';
            idx = buildingsAtlas->GetIndex(lower.c_str());
        }
        if (idx == 0xFFFFFFFF) continue;

        const SpriteRegion* r = buildingsAtlas->GetRegion(idx);
        if (!r) continue;

        SpriteInfo si;
        si.u0 = r->u0; si.v0 = r->v0;
        si.u1 = r->u1; si.v1 = r->v1;
        si.w  = static_cast<float>(r->width);
        si.h  = static_cast<float>(r->height);
        si.pivotX = r->pivotX;
        si.pivotY = r->pivotY;
        m_spriteCache[bt] = si;
    }

    m_atlasLoaded = true;
}

const BuildingHighlightPass::SpriteInfo* BuildingHighlightPass::GetSpriteInfo(
    int buildingType, bool depleted)
{
    // Depleted handling: for now, find by type (depleted sprites use same lookup).
    std::map<int, SpriteInfo>::iterator it = m_spriteCache.find(buildingType);
    if (it != m_spriteCache.end())
        return &it->second;
    return NULL;
}

void BuildingHighlightPass::Execute(
    const RenderFrame& frame, const RenderContext& context,
    RenderCommandBuffer& buffer)
{
    if (frame.highlights.empty()) return;

    if (!m_atlasLoaded) {
        LoadAtlas();
        if (!m_atlasLoaded) return;
    }

    for (size_t i = 0; i < frame.highlights.size(); ++i) {
        const RenderBuildingHighlight& h = frame.highlights[i];
        const SpriteInfo* si = GetSpriteInfo(
            static_cast<int>(h.buildingType), h.isDepleted);
        if (!si) continue;

        int sx = h.transform.screenX;
        int sy = h.transform.screenY;
        uint16_t depth = static_cast<uint16_t>(h.transform.depthLayer);

        buffer.PushSprite(
            sx - si->pivotX, sy - si->pivotY,
            si->w, si->h,
            si->u0, si->v0, si->u1, si->v1,
            m_textureSlot, depth,
            D3DCOLOR_ARGB(80, 255, 255, 255),
            0xFFFF, 0xFF, LAYER_FOREGROUND);
    }
}

} // namespace Scene
