#include "stdafx.h"
#include "WildlifePresentationSystem.h"
#include "../../World/WildlifeSystem.h"
#include "../../World/Animal.h"
#include "../../World/AnimalTypes.h"
#include "../../Logic/CoordinateSystem.h"

namespace Scene {

void WildlifePresentationSystem::SetWildlifeSystem(World::WildlifeSystem* wildlife)
{
    m_wildlife = wildlife;
}

void WildlifePresentationSystem::BuildRenderFrame(std::vector<RenderWildlife>& out)
{
    if (!m_wildlife) return;

    const std::vector<World::Animal>& animals = m_wildlife->GetAllAnimals();
    if (animals.empty()) return;

    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    for (size_t i = 0; i < animals.size(); ++i) {
        const World::Animal& a = animals[i];
        if (a.state != World::AnimalState_Alive) continue;
        if (a.type < 0 || a.type >= World::AnimalType_Count) continue;

        RenderWildlife rw;
        rw.visual.type     = static_cast<uint8_t>(a.type);
        rw.visual.state    = static_cast<uint8_t>(a.state);
        rw.visual.dirIndex = static_cast<uint8_t>(World::VelocityToDirIndex(a.vx, a.vy));

        float wx, wy;
        coords.NodeTileToWorld(a.x, a.y, wx, wy);
        rw.transform.worldX = wx;
        rw.transform.worldY = wy;
        rw.transform.depthLayer = 30005 + static_cast<int>(a.y + 0.5f) * 400;

        out.push_back(rw);
    }
}

} // namespace Scene
