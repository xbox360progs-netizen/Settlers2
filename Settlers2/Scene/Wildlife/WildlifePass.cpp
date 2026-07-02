#include "stdafx.h"
#include "WildlifePass.h"
#include "RenderWildlife.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/TextureRegistry.h"

namespace Scene {

WildlifePass::WildlifePass()
    : m_atlasLoaded(false)
    , m_textureSlot(0)
{
}

void WildlifePass::LoadAtlas()
{
    TextureRegistry& reg = TextureRegistry::instance();
    reg.getTextureOrLoad("Units");
    std::tr1::shared_ptr<SpriteAtlas> unitsAtlas = reg.getAtlas("Units");
    if (!unitsAtlas) return;

    const std::vector<uint32_t>* animalGroup = unitsAtlas->GetGroup("Animals");
    if (!animalGroup || animalGroup->empty()) return;

    // Pre-cache sprite regions from the "Animals" group indices.
    // Maximum 16 (4 types x 4 directions), but we cache whatever is available.
    for (size_t i = 0; i < animalGroup->size() && i < 16; ++i) {
        uint32_t regionIdx = (*animalGroup)[i];
        const SpriteRegion* region = unitsAtlas->GetRegion(regionIdx);
        if (!region) continue;

        m_sprites[i].u0 = region->u0;
        m_sprites[i].v0 = region->v0;
        m_sprites[i].u1 = region->u1;
        m_sprites[i].v1 = region->v1;
        m_sprites[i].w  = static_cast<float>(region->width);
        m_sprites[i].h  = static_cast<float>(region->height);
        m_sprites[i].pivotX = region->pivotX;
        m_sprites[i].pivotY = region->pivotY;
    }

    m_atlasLoaded = true;
}

void WildlifePass::Execute(const RenderFrame& frame, const RenderContext& context, RenderCommandBuffer& buffer)
{
    const std::vector<RenderWildlife>& wildlife = frame.wildlife;
    if (wildlife.empty()) return;

    if (!m_atlasLoaded) {
        LoadAtlas();
        if (!m_atlasLoaded) return;
    }

    for (size_t i = 0; i < wildlife.size(); ++i) {
        const RenderWildlife& w = wildlife[i];
        const WildlifeVisual& vis = w.visual;

        // Compute sprite index: type * 4 directions + direction index.
        int rawIdx = static_cast<int>(vis.type);
        int dirSpriteIdx = rawIdx * 4 + static_cast<int>(vis.dirIndex);

        int spriteIdx;
        if (dirSpriteIdx < 16) {
            spriteIdx = dirSpriteIdx;
        } else if (rawIdx < 16) {
            spriteIdx = rawIdx;
        } else {
            continue;
        }

        const AnimalSprite& s = m_sprites[spriteIdx];
        if (s.w == 0.0f || s.h == 0.0f) continue;

        // Apply pivot offset in screen space.
        int sx = w.transform.screenX - static_cast<int>(s.pivotX);
        int sy = w.transform.screenY - static_cast<int>(s.pivotY);

        uint16_t depth = static_cast<uint16_t>(w.transform.depthLayer);

        buffer.PushSprite(
            sx, sy,
            s.w, s.h,
            s.u0, s.v0, s.u1, s.v1,
            m_textureSlot,
            depth,
            0xFFFFFFFF,
            0xFFFF,
            0xFF,
            0xFF
        );
    }
}

} // namespace Scene
