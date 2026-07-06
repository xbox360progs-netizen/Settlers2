#include "stdafx.h"
#include "LegacyBuildingSource.h"
#include "../../../World/FlagManager.h"
#include "../../../World/ConstructionManager.h"
#include "../../../World/ConstructionSite.h"
#include "../../../World/Flag.h"
#include "../../../World/Components/Building.h"

// Temporary — reads legacy World managers. Remove when BuildingPresentationSystem
// switches to SimulationCoreBuildingSource.

namespace Scene {

void LegacyBuildingSource::SetManagers(
    World::FlagManager* flagManager,
    World::ConstructionManager* constructionManager)
{
    m_flagManager = flagManager;
    m_constructionManager = constructionManager;
}


// ─── IBuildingSource (completed buildings) ─────────────────────────────

uint32_t LegacyBuildingSource::GetBuildingCount() const
{
    if (!m_flagManager) return 0;
    const std::vector<std::pair<int,int> >& pairs = m_flagManager->GetFlagPairs();

    uint32_t count = 0;
    for (size_t i = 0; i < pairs.size(); ++i) {
        World::Flag* flag = m_flagManager->GetFlagAt(pairs[i].first, pairs[i].second);
        if (flag && flag->building) {
            ++count;
        }
    }
    return count;
}

bool LegacyBuildingSource::GetBuilding(uint32_t index, BuildingView& out) const
{
    if (!m_flagManager) return false;

    const std::vector<std::pair<int,int> >& pairs = m_flagManager->GetFlagPairs();

    uint32_t found = 0;
    for (size_t i = 0; i < pairs.size(); ++i) {
        int fx = pairs[i].first;
        int fy = pairs[i].second;
        World::Flag* flag = m_flagManager->GetFlagAt(fx, fy);
        if (!flag || !flag->building) continue;

        if (found == index) {
            out.flagX = fx;
            out.flagY = fy;
            out.kind = 1; // completed building
            out.buildingType = static_cast<uint8_t>(flag->building->type);
            out.buildingId = index + 1;
            out.depleted = flag->building->IsDepleted();
            out.fsmState = static_cast<uint8_t>(flag->building->GetFsmState());

            float dummyX, dummyY;
            int dummySprite;
            out.hasWorker = flag->building->GetWorkerRenderInfo(dummyX, dummyY, dummySprite);
            out.workerVisualState = out.hasWorker
                ? (out.fsmState == 1 ? WVS_Working : WVS_Idle)
                : WVS_None;

            out.color = 0xFFFFFFFF;
            return true;
        }
        ++found;
    }
    return false;
}


// ─── IConstructionSiteSource ─────────────────────────────────────────────

uint32_t LegacyBuildingSource::GetConstructionSiteCount() const
{
    if (!m_constructionManager) return 0;
    return static_cast<uint32_t>(m_constructionManager->GetAllSites().size());
}

bool LegacyBuildingSource::GetConstructionSite(uint32_t index, BuildingView& out) const
{
    if (!m_constructionManager) return false;

    const std::vector<World::ConstructionSite*>& sites = m_constructionManager->GetAllSites();
    if (index >= sites.size()) return false;

    World::ConstructionSite* site = sites[index];
    if (!site) return false;

    out.flagX = site->x;
    out.flagY = site->y;
    out.kind = 2; // construction site
    out.buildingType = static_cast<uint8_t>(site->buildingType);
    out.buildingId = index + 1;
    out.depleted = false;
    out.fsmState = 0;
    out.hasWorker = (site->builderState != World::Builder_None);
    out.workerVisualState = out.hasWorker ? WVS_Working : WVS_None;
    out.color = 0xFFFFFFFF;
    return true;
}

} // namespace Scene
