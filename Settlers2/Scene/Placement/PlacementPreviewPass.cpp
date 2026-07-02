#include "stdafx.h"
#include "PlacementPreviewPass.h"
#include "RenderPlacementPreview.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/TextureRegistry.h"
#include "../../Graphics/RenderLayers.h"
#include "../../World/Components/Building.h"

namespace Scene {

PlacementPreviewPass::PlacementPreviewPass()
    : m_atlasLoaded(false)
    , m_textureSlot(0)
{
    for (int i = 0; i < 32; ++i) {
        m_spriteLoaded[i] = false;
        m_spriteCache[i].valid = false;
    }
}

const char* PlacementPreviewPass::SpriteNameForType(uint8_t buildingType) const
{
    switch (static_cast<World::BuildingType>(buildingType)) {
        case World::Woodcutter:   return "b_woodcutter";
        case World::Forester:     return "b_forester";
        case World::Sawmill:      return "b_sawmill";
        case World::Stonemason:   return "b_mason";
        case World::CoalMine:
        case World::BronzeMine:   return "b_mine";
        case World::IronMine:     return "b_mine";
        case World::GoldMine:     return "b_mine";
        case World::IronSmelter:  return "b_ironsmelter";
        case World::GoldSmelter:  return "b_goldsmelter";
        case World::BronzeSmelter: return "b_bronzesmelter";
        case World::Farm:         return "b_farm";
        case World::Mill:         return "b_mill";
        case World::Bakery:       return "b_bakery";
        case World::Fisher:       return "b_fisher";
        case World::Hunter:       return "b_hunter";
        case World::ToolWorkshop: return "b_toolworkshop";
        case World::Storehouse:   return "b_warehouse";
        case World::Well:         return "b_well";
        case World::Barracks:     return "b_barracks";
        default:                  return "";
    }
}

void PlacementPreviewPass::CacheSprite(uint8_t buildingType)
{
    if (buildingType >= 32) return;

    BuildingSprite& sprite = m_spriteCache[buildingType];
    sprite.valid = false;
    m_spriteLoaded[buildingType] = true;

    const char* name = SpriteNameForType(buildingType);
    if (!name || !*name) return;

    TextureRegistry& reg = TextureRegistry::instance();
    std::tr1::shared_ptr<SpriteAtlas> atlas = reg.getAtlas("Buildings");
    if (!atlas) return;

    uint32_t idx = atlas->GetIndex(name);
    if (idx == 0xFFFFFFFF) return;

    const SpriteRegion* region = atlas->GetRegion(idx);
    if (!region) return;

    sprite.u0 = region->u0;
    sprite.v0 = region->v0;
    sprite.u1 = region->u1;
    sprite.v1 = region->v1;
    sprite.w  = static_cast<float>(region->width);
    sprite.h  = static_cast<float>(region->height);
    sprite.pivotX = region->pivotX;
    sprite.pivotY = region->pivotY;
    sprite.valid = true;
}

void PlacementPreviewPass::Execute(const RenderFrame& frame, const RenderContext& context, RenderCommandBuffer& buffer)
{
    const std::vector<RenderPlacementPreview>& previews = frame.preview;
    if (previews.empty()) return;

    if (!m_atlasLoaded) {
        // Ensure Buildings atlas is available.
        TextureRegistry& reg = TextureRegistry::instance();
        reg.getTextureOrLoad("Buildings");
        m_atlasLoaded = true;
    }

    for (size_t i = 0; i < previews.size(); ++i) {
        const RenderPlacementPreview& p = previews[i];
        uint8_t buildingType = p.visual.type;

        if (buildingType == 0 || buildingType >= 32) continue;

        // Lazy-load sprite cache for this building type.
        if (!m_spriteLoaded[buildingType]) {
            CacheSprite(buildingType);
        }

        const BuildingSprite& sprite = m_spriteCache[buildingType];
        if (!sprite.valid) continue;

        // Apply pivot offset in screen space.
        int sx = p.transform.screenX - static_cast<int>(sprite.pivotX);
        int sy = p.transform.screenY - static_cast<int>(sprite.pivotY);

        uint16_t depth = static_cast<uint16_t>(p.transform.depthLayer);
        // Green tint for valid placement, red tint for invalid.
        uint32_t color = p.visual.allowed ? 0xAAFFFFFF : 0x44FF4444;

        buffer.PushSprite(
            sx, sy,
            sprite.w, sprite.h,
            sprite.u0, sprite.v0, sprite.u1, sprite.v1,
            m_textureSlot,
            depth,
            color,
            0xFFFF,
            0xFF,
            LAYER_FOREGROUND
        );
    }
}

} // namespace Scene
