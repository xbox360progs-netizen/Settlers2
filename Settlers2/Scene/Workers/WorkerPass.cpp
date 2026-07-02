#include "stdafx.h"
#include "WorkerPass.h"
#include "RenderWorker.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/SpriteAtlas.h"
#include "../../Graphics/TextureRegistry.h"
#include "../../World/ResourceNode.h"
#include "../../World/Components/Building.h"
#include "../TextureSlots.h"

namespace Scene {

WorkerPass::WorkerPass()
    : m_unitSlot(SLOT_UNITS)
    , m_iconSlot(SLOT_UI_MENU_ICON)
{
}

int WorkerPass::ResolveSpriteIndex(const RenderWorker& w) const
{
    switch (w.type) {
        case 0: // SettlerType_Carrier
            if (w.carrying) {
                if (w.dy < 0) return (w.dx >= 0) ? 9 : 11;
                if (w.dy > 0) return (w.dx >= 0) ? 8 : 10;
                return (w.dx >= 0) ? 8 : 11;
            } else {
                if (w.dy < 0) return (w.dx < 0) ? 2 : 0;
                if (w.dy > 0) return (w.dx < 0) ? 3 : 1;
                return (w.dx >= 0) ? 1 : 3;
            }

        case 1: // SettlerType_Builder
        case 2: // SettlerType_Worker
            return (w.dx >= 0) ? 4 : 5;

        case 3: // SettlerType_BuildingWorker
            switch (w.buildingType) {
                case World::Woodcutter:
                    if (w.dy < 0) return (w.dx < 0) ? 19 : 18;
                    if (w.dy > 0) return (w.dx >= 0) ? 20 : 21;
                    return (w.dx >= 0) ? 20 : 19;
                case World::Forester:
                    if (w.dy < 0) return (w.dx < 0) ? 37 : 36;
                    if (w.dy > 0) return (w.dx >= 0) ? 34 : 35;
                    return (w.dx >= 0) ? 34 : 37;
                case World::Fisher:
                    return (w.dx >= 0) ? 6 : 7;
                case World::Hunter:
                    return (w.dx >= 0) ? 12 : 13;
                default:
                    return (w.dx >= 0) ? 4 : 5;
            }

        default:
            return 4;
    }
}

void WorkerPass::Execute(const RenderFrame& frame, const RenderContext& context, RenderCommandBuffer& buffer)
{
    const std::vector<RenderWorker>& workers = frame.workers;
    if (workers.empty()) return;

    // Resolve atlas via TextureRegistry (same as SettlerRenderer).
    TextureRegistry& reg = TextureRegistry::instance();
    std::tr1::shared_ptr<SpriteAtlas> unitsAtlas = reg.getAtlas("Units");
    if (!unitsAtlas) return;

    std::tr1::shared_ptr<SpriteAtlas> iconAtlas;
    // Check if any worker carries cargo — only then load Icon atlas.
    bool needIcons = false;
    for (size_t i = 0; i < workers.size() && !needIcons; ++i) {
        if (workers[i].carrying && workers[i].cargoType != 0) needIcons = true;
    }
    if (needIcons) {
        iconAtlas = reg.getAtlas("Icon");
    }

    for (size_t i = 0; i < workers.size(); ++i) {
        const RenderWorker& w = workers[i];
        const RenderTransform& t = w.transform;

        int spriteIdx = ResolveSpriteIndex(w);
        const SpriteRegion* r = unitsAtlas->GetRegion(spriteIdx);
        if (!r) continue;

        WORD depth = static_cast<WORD>(t.depthLayer);

        buffer.PushSprite(
            t.screenX - static_cast<int>(r->pivotX + 0.5f),
            t.screenY - static_cast<int>(r->pivotY + 0.5f),
            static_cast<float>(r->width), static_cast<float>(r->height),
            r->u0, r->v0, r->u1, r->v1,
            m_unitSlot, depth);

        // Cargo icon for carriers
        if (w.carrying && w.cargoType != 0 && iconAtlas) {
            World::ResourceType cargoType = static_cast<World::ResourceType>(w.cargoType);
            const char* iconName = World::ResourceTypeToIconName(cargoType);
            if (iconName && iconName[0]) {
                uint32_t cargoIdx = iconAtlas->GetIndex(iconName);
                if (cargoIdx != 0xFFFFFFFF) {
                    const SpriteRegion* cargoR = iconAtlas->GetRegion(cargoIdx);
                    if (cargoR) {
                        float cargoSize = 16.0f;
                        int cargoSX = t.screenX - static_cast<int>(cargoSize * 0.5f);
                        int cargoSY = t.screenY - static_cast<int>(r->pivotY + cargoSize + 0.5f);
                        buffer.PushSprite(
                            cargoSX, cargoSY,
                            cargoSize, cargoSize,
                            cargoR->u0, cargoR->v0, cargoR->u1, cargoR->v1,
                            m_iconSlot,
                            static_cast<WORD>(t.depthLayer + 10));
                    }
                }
            }
        }
    }
}

} // namespace Scene
