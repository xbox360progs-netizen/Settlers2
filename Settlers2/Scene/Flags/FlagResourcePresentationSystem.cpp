#include "stdafx.h"
#include "FlagResourcePresentationSystem.h"
#include "../../World/FlagManager.h"
#include "../../World/Flag.h"
#include "../../Logic/CoordinateSystem.h"

namespace Scene {

void FlagResourcePresentationSystem::BuildRenderFrame(std::vector<RenderFlagResource>& outResources)
{
    outResources.clear();
    if (!m_flagManager) return;

    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    for (size_t fi = 0; fi < m_flagManager->GetCount(); ++fi) {
        World::Flag* flag = m_flagManager->GetFlag(fi);
        if (!flag) continue;

        float fx, fy;
        coords.NodeTileToWorld(flag->pos.x, flag->pos.y, fx, fy);

        int stackOrder = 0;
        for (int si = 0; si < 8; ++si) {
            if (flag->slots[si].type == World::ResourceType_None || flag->slots[si].amount <= 0) continue;

            RenderFlagResource r;
            r.worldX = fx;
            r.worldY = fy;
            r.resourceType = static_cast<uint8_t>(flag->slots[si].type);
            r.stackOrder = static_cast<int8_t>(stackOrder);
            r.tileY = flag->pos.y;

            outResources.push_back(r);
            ++stackOrder;
        }
    }
}

} // namespace Scene
