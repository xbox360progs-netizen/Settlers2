#include "stdafx.h"
#include "SettlerRenderer.h"
#include "RenderSettler.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/SpriteAtlas.h"
#include "../../World/ResourceNode.h"
#include "../TextureSlots.h"
#include <algorithm>

namespace Scene {

SettlerRenderer::SettlerRenderer()
    : m_unitsAtlas(NULL)
    , m_iconAtlas(NULL)
    , m_unitSlot(SLOT_UNITS)
{
}

void SettlerRenderer::SetAtlases(
    Graphics::SpriteAtlas* unitsAtlas,
    Graphics::SpriteAtlas* iconAtlas,
    int unitTextureSlot)
{
    m_unitsAtlas = unitsAtlas;
    m_iconAtlas = iconAtlas;
    m_unitSlot = unitTextureSlot;
}

int SettlerRenderer::ResolveSpriteIndex(const RenderSettler& s) const
{
    const SettlerVisual& v = s.visual;
    switch (static_cast<SettlerType>(v.type)) {
        case SettlerType_Carrier:
            if (v.carrying) {
                if (v.dy < 0) return (v.dx >= 0) ? 9 : 11;
                if (v.dy > 0) return (v.dx >= 0) ? 8 : 10;
                return (v.dx >= 0) ? 8 : 11;
            } else {
                if (v.dy < 0) return (v.dx < 0) ? 2 : 0;
                if (v.dy > 0) return (v.dx < 0) ? 3 : 1;
                return (v.dx >= 0) ? 1 : 3;
            }

        case SettlerType_Builder:
        case SettlerType_Worker:
            return (v.dx >= 0) ? 4 : 5;

        case SettlerType_BuildingWorker:
            switch (v.buildingType) {
                case World::Building_Woodcutter:
                    if (v.dy < 0) return (v.dx < 0) ? 19 : 18;
                    if (v.dy > 0) return (v.dx >= 0) ? 20 : 21;
                    return (v.dx >= 0) ? 20 : 19;
                case World::Building_Forester:
                    if (v.dy < 0) return (v.dx < 0) ? 37 : 36;
                    if (v.dy > 0) return (v.dx >= 0) ? 34 : 35;
                    return (v.dx >= 0) ? 34 : 37;
                case World::Building_Fisher:
                    return (v.dx >= 0) ? 6 : 7;
                case World::Building_Hunter:
                    return (v.dx >= 0) ? 12 : 13;
                default:
                    return (v.dx >= 0) ? 4 : 5;
            }

        default:
            return 4;
    }
}

void SettlerRenderer::Render(
    RenderCommandBuffer& buffer,
    const RenderFrame& frame)
{
    if (!m_unitsAtlas || frame.settlers.empty()) return;

    const std::vector<RenderSettler>& settlers = frame.settlers;

    for (size_t i = 0; i < settlers.size(); ++i) {
        const RenderSettler& s = settlers[i];
        const RenderTransform& t = s.transform;
        const SettlerVisual& v = s.visual;

        int spriteIdx = ResolveSpriteIndex(s);
        const SpriteRegion* r = m_unitsAtlas->GetRegion(spriteIdx);
        if (!r) continue;

        WORD depth = static_cast<WORD>(t.depthLayer);

        buffer.PushSprite(
            t.screenX - (int)(r->pivotX + 0.5f), t.screenY - (int)(r->pivotY + 0.5f),
            (float)r->width, (float)r->height,
            r->u0, r->v0, r->u1, r->v1,
            m_unitSlot, depth);

        // Cargo icon for carriers
        if (v.carrying && v.cargoType != 0 && m_iconAtlas) {
            World::ResourceType cargoType = static_cast<World::ResourceType>(v.cargoType);
            const char* iconName = World::ResourceTypeToIconName(cargoType);
            if (iconName && iconName[0]) {
                uint32_t cargoIdx = m_iconAtlas->GetIndex(iconName);
                if (cargoIdx != 0xFFFFFFFF) {
                    const SpriteRegion* cargoR = m_iconAtlas->GetRegion(cargoIdx);
                    if (cargoR) {
                        float cargoSize = 16.0f;
                        int cargoSX = t.screenX - (int)(cargoSize * 0.5f);
                        int cargoSY = t.screenY - (int)(r->pivotY + cargoSize + 0.5f);
                        buffer.PushSprite(cargoSX, cargoSY,
                            cargoSize, cargoSize,
                            cargoR->u0, cargoR->v0, cargoR->u1, cargoR->v1,
                            SLOT_UI_MENU_ICON,
                            static_cast<WORD>(t.depthLayer + 10));
                    }
                }
            }
        }
    }
}

} // namespace Scene
