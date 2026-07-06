#include "stdafx.h"
#include "FlagResourcePresentationSystem.h"
#include "../../Logic/CoordinateSystem.h"

namespace Scene {

void FlagResourcePresentationSystem::BuildRenderFrame(std::vector<RenderFlagResource>& outResources)
{
    outResources.clear();
    if (!m_flagSource) return;

    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    uint32_t count = m_flagSource->GetFlagCount();
    for (uint32_t fi = 0; fi < count; ++fi) {
        FlagView fv;
        if (!m_flagSource->GetFlag(fi, fv)) continue;

        float fx, fy;
        coords.NodeTileToWorld(fv.nodeX, fv.nodeY, fx, fy);

        int stackOrder = 0;
        for (int si = 0; si < fv.slotCount; ++si) {
            RenderFlagResource r;
            r.worldX = fx;
            r.worldY = fy;
            r.resourceType = fv.slotTypes[si];
            r.stackOrder = static_cast<int8_t>(stackOrder);
            r.tileY = fv.nodeY;

            outResources.push_back(r);
            ++stackOrder;
        }
    }
}

} // namespace Scene