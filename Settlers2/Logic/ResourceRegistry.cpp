#include "stdafx.h"
#include "ResourceRegistry.h"
#include <cstdlib>

namespace Logic {

    ResourceRegistry::ResourceRegistry() {
        for (int i = 0; i < World::ResourceType_Count; ++i) {
            m_planningReserved[i] = 0;
        }
    }

    void ResourceRegistry::Register(World::Building* building) {
        for (size_t j = 0; j < building->outputResources.size(); ++j) {
            World::ResourceType t = building->outputResources[j];
            m_producers[t].push_back(building);
        }
    }

    void ResourceRegistry::Unregister(World::Building* building) {
        for (size_t j = 0; j < building->outputResources.size(); ++j) {
            World::ResourceType t = building->outputResources[j];
            std::vector<World::Building*>& vec = m_producers[t];
            for (size_t k = 0; k < vec.size(); ++k) {
                if (vec[k] == building) {
                    vec[k] = vec.back();
                    vec.pop_back();
                    break;
                }
            }
        }
    }

    void ResourceRegistry::ReservePlanning(World::ResourceType type, int amount) {
        m_planningReserved[type] += amount;
    }

    void ResourceRegistry::ReleasePlanning(World::ResourceType type, int amount) {
        m_planningReserved[type] -= amount;
        if (m_planningReserved[type] < 0) m_planningReserved[type] = 0;
    }

    void ResourceRegistry::ClearPlanningReservations() {
        for (int i = 0; i < World::ResourceType_Count; ++i) {
            m_planningReserved[i] = 0;
        }
    }

    int ResourceRegistry::GetPlanningReserved(World::ResourceType type) const {
        return m_planningReserved[type];
    }

    World::Building* ResourceRegistry::FindBestSupplier(
        World::ResourceType type,
        int& outAmount,
        World::Building* exclude,
        const Vector2i& requesterPos,
        const int* deliveryReserved)
    {
        (void)deliveryReserved;

        std::vector<World::Building*>& vec = m_producers[type];
        World::Building* best = NULL;
        int bestDist = 999999;

        for (size_t i = 0; i < vec.size(); ++i) {
            World::Building* b = vec[i];
            if (b == exclude) continue;
            if (b->state != World::State_Finished) continue;

            std::map<World::ResourceType, int>::iterator it = b->inventory.find(type);
            if (it == b->inventory.end() || it->second <= 0) continue;

            int dist = abs(b->pos.x - requesterPos.x) + abs(b->pos.y - requesterPos.y);
            if (!best || dist < bestDist) {
                best = b;
                bestDist = dist;
                outAmount = it->second;
            }
        }

        return best;
    }

}
