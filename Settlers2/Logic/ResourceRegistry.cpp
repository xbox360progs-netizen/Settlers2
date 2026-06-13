#include "stdafx.h"
#include "ResourceRegistry.h"
#include "../World/Map.h"
#include "../World/Flag.h"
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

    void ResourceRegistry::BuildWorldResourceCache(World::Map* map) {
        ClearWorldResources();
        // Resource map is stored at layer resolution (m_width*2 x m_height*4),
        // NOT ground resolution (m_width x m_height). Scan all nodes.
        int w = map->GetWidth() * 2;
        int h = map->GetHeight() * 4;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const World::ResourceNode& node = map->GetResourceNode(x, y);
                if (node.type != World::ResourceType_None) {
                    RegisterWorldResource(node.type, x, y);
                }
            }
        }
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
        int bestScore = 999999;

        static const int CONGESTION_FACTOR = 4;

        for (size_t i = 0; i < vec.size(); ++i) {
            World::Building* b = vec[i];
            if (b == exclude) continue;
            if (b->state != World::State_Finished) continue;

            if (b->m_storage[type] <= 0) continue;

            int baseDist = abs(b->pos.x - requesterPos.x) + abs(b->pos.y - requesterPos.y);
            int congestion = 0;
            if (b->connectedFlag) {
                for (int s = 0; s < 8; ++s) {
                    if (b->connectedFlag->slots[s].type == type) {
                        congestion += b->connectedFlag->slots[s].reserved;
                    }
                }
            }
            congestion = congestion / CONGESTION_FACTOR;
            int score = baseDist + congestion;

            if (!best || score < bestScore) {
                best = b;
                bestScore = score;
                outAmount = b->m_storage[type];
            }
        }

        return best;
    }

    bool ResourceRegistry::FindNearestWorldResource(World::ResourceType type, const Vector2i& pos, Vector2i& outPos) {
        std::vector<Vector2i>& vec = m_worldResources[type];
        int bestDist = 999999;
        bool found = false;

        for (size_t i = 0; i < vec.size(); ++i) {
            int dist = abs(vec[i].x - pos.x) + abs(vec[i].y - pos.y);
            if (dist < bestDist) {
                bestDist = dist;
                outPos = vec[i];
                found = true;
            }
        }
        return found;
    }

    void ResourceRegistry::RegisterWorldResource(World::ResourceType type, int x, int y) {
        Vector2i v;
        v.x = x;
        v.y = y;
        m_worldResources[type].push_back(v);
    }

    void ResourceRegistry::UnregisterWorldResource(World::ResourceType type, int x, int y) {
        std::vector<Vector2i>& vec = m_worldResources[type];
        for (size_t i = 0; i < vec.size(); ++i) {
            if (vec[i].x == x && vec[i].y == y) {
                vec[i] = vec.back();
                vec.pop_back();
                break;
            }
        }
    }

    void ResourceRegistry::ClearWorldResources() {
        for (int t = 0; t < World::ResourceType_Count; ++t) {
            m_worldResources[t].clear();
        }
    }

} // namespace Logic