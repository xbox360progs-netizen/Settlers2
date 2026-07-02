#include "stdafx.h"
#include "FlagResourcePass.h"
#include "RenderFlagResource.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/TextureRegistry.h"

namespace Scene {

FlagResourcePass::FlagResourcePass()
    : m_atlasLoaded(false)
    , m_textureSlot(0)
{
}

void FlagResourcePass::LoadAtlas()
{
    TextureRegistry& reg = TextureRegistry::instance();
    std::tr1::shared_ptr<SpriteAtlas> iconAtlas = reg.getAtlas("Icon");
    if (!iconAtlas) return;

    // Pre-cache all known resource icon regions.
    // Indexed by ResourceType enum value (0 = ResourceType_None = skipped).
    static const int MAX_RESOURCE_TYPES = 50;
    m_iconCache.resize(MAX_RESOURCE_TYPES);

    // Map of resource type → icon name (matching GameRenderer legacy switch).
    // Using inlined values to avoid dependency on World::ResourceType in this render-only class.
    static const struct { uint8_t type; const char* name; } kIconMap[] = {
        { 1,  "r_wood" },       // ResourceType_Wood
        { 2,  "r_planks" },     // ResourceType_Planks
        { 3,  "r_fish" },       // ResourceType_Fish
        { 4,  "r_coal" },       // ResourceType_Coal
        { 5,  "r_ironore" },    // ResourceType_IronOre
        { 6,  "r_goldore" },    // ResourceType_GoldOre
        { 7,  "r_ironbar" },    // ResourceType_IronBar
        { 8,  "r_goldbar" },    // ResourceType_GoldBar
        { 9,  "r_stone" },      // ResourceType_Stone
        { 10, "r_meat" },       // ResourceType_Meat
        { 11, "r_wheat" },      // ResourceType_Wheat
        { 12, "r_flour" },      // ResourceType_Flour
        { 13, "r_bread" },      // ResourceType_Bread
        { 14, "r_water" },      // ResourceType_Water
        { 15, "r_tools" },      // ResourceType_Tools
        { 28, "r_bronzebar" },  // ResourceType_BronzeBar
    };

    for (size_t i = 0; i < sizeof(kIconMap) / sizeof(kIconMap[0]); ++i) {
        uint8_t type = kIconMap[i].type;
        if (type >= m_iconCache.size()) continue;

        uint32_t idx = iconAtlas->GetIndex(kIconMap[i].name);
        if (idx == 0xFFFFFFFF) continue;

        const SpriteRegion* region = iconAtlas->GetRegion(idx);
        if (!region) continue;

        IconSprite& sprite = m_iconCache[type];
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

    m_atlasLoaded = true;
}

void FlagResourcePass::Execute(const RenderFrame& frame, const RenderContext& context, RenderCommandBuffer& buffer)
{
    const std::vector<RenderFlagResource>& resources = frame.flagResources;
    if (resources.empty()) return;

    if (!m_atlasLoaded) {
        LoadAtlas();
        if (!m_atlasLoaded) return;
    }

    for (size_t i = 0; i < resources.size(); ++i) {
        const RenderFlagResource& r = resources[i];

        if (r.resourceType >= m_iconCache.size()) continue;
        const IconSprite& sprite = m_iconCache[r.resourceType];
        if (!sprite.valid) continue;

        // Apply pivot offset at 0.5x scale and per-slot stacking offset.
        int sx = r.screenX - static_cast<int>(sprite.pivotX * 0.5f);
        int sy = r.screenY - static_cast<int>(sprite.pivotY * 0.5f) - 30 + r.stackOrder * 16;

        // Depth: 30011 + flagTileY * 400 - stackOrder (matching legacy +iconY where iconY = -stackOrder).
        uint16_t depth = static_cast<uint16_t>(30011 + r.tileY * 400 - r.stackOrder);

        buffer.PushSprite(
            sx, sy,
            sprite.w * 0.5f, sprite.h * 0.5f,
            sprite.u0, sprite.v0, sprite.u1, sprite.v1,
            m_textureSlot,
            depth,
            0xFFFFFFFF,                  // color
            0xFFFF,                      // shaderId: default (SHADER_WORLD_SCREEN)
            0xFF,                        // blendMode: default (alpha)
            0xFF                         // layer: default (LAYER_WORLD)
        );
    }
}

} // namespace Scene
