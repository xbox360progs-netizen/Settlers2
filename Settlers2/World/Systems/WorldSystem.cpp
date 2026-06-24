#include "stdafx.h"
#include "WorldSystem.h"
#include "../Map.h"
#include "../FlagManager.h"
#include "../Flag.h"
#include "../CargoManager.h"
#include "../ResourceNode.h"
#include "../../Core/EventBus.h"

namespace World {

const float WorldSystem::WILDLIFE_REGEN_INTERVAL = 60.0f;
const float WorldSystem::TREE_GROWTH_INTERVAL = 30.0f;

WorldSystem::WorldSystem()
    : m_map(NULL)
    , m_flagManager(NULL)
    , m_cargoManager(NULL)
    , m_eventBus(NULL)
    , m_wildlifeRegenTimer(0.0f)
    , m_treeGrowthTimer(0.0f)
{
}

WorldSystem::~WorldSystem()
{
    if (m_eventBus) {
        m_eventBus->UnregisterAll(this);
    }
}

void WorldSystem::Initialize(Map* map, FlagManager* flagManager, CargoManager* cargoManager,
                             Core::EventBus* eventBus)
{
    m_map = map;
    m_flagManager = flagManager;
    m_cargoManager = cargoManager;
    m_eventBus = eventBus;

    if (m_eventBus) {
        m_eventBus->Register(Core::Event_ResourceDelivered, this);
    }
}

void WorldSystem::Update(float dt)
{
    if (!m_map) return;

    // Wildlife resource node regeneration
    m_wildlifeRegenTimer += dt;
    if (m_wildlifeRegenTimer >= WILDLIFE_REGEN_INTERVAL) {
        m_wildlifeRegenTimer = 0.0f;
        m_map->RegenerateWildlifeResources();
    }

    // Tree growth (Sapling → Young → Mature)
    m_treeGrowthTimer += dt;
    if (m_treeGrowthTimer >= TREE_GROWTH_INTERVAL) {
        m_treeGrowthTimer = 0.0f;
        m_map->GrowTrees();
    }
}

void WorldSystem::CollectGroundResourcesToNearestFlag(uint32_t whFlagId)
{
    if (!m_map || !m_flagManager) return;

    int n = m_map->GetGroundResourceCount();
    for (int gi = n - 1; gi >= 0; --gi) {
        GroundResource* gr = m_map->GetGroundResource(gi);
        if (!gr) continue;

        // Wood ground resources handled by Woodcutter (CarryWoodHome), not generic loop
        if (gr->type == ResourceType_Wood) continue;

        // Find nearest player-owned flag with an open slot
        float bestDist = 1e9f;
        Flag* bestFlag = NULL;
        for (size_t fi = 0; fi < m_flagManager->GetCount(); ++fi) {
            Flag* f = m_flagManager->GetFlag(fi);
            if (!f) continue;
            float dx = (float)(gr->pos.x - f->pos.x);
            float dy = (float)(gr->pos.y - f->pos.y);
            float d = dx * dx + dy * dy;
            if (d < bestDist) {
                bestDist = d;
                bestFlag = f;
            }
        }

        if (bestFlag) {
            bool added = bestFlag->AddResource(gr->type, 1, whFlagId);
            if (added) {
                gr->amount--;
                if (gr->amount <= 0)
                    m_map->RemoveGroundResource(gi);
            }
        }
    }
}

void WorldSystem::OnEvent(Core::EventType type, void* data)
{
    (void)data;
    (void)type;
}

} // namespace World
