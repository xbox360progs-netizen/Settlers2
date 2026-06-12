#include "stdafx.h"
#include "RoadManager.h"
#include "Flag.h"
#include "FlagManager.h"
#include "CarrierManager.h"
#include "TransportJobManager.h"
#include <queue>
#include <map>
#include <algorithm>

namespace World {

    uint32_t RoadManager::s_nextId = 1;

    RoadManager::RoadManager()
        : m_flagManager(NULL)
    {
    }

    RoadManager::~RoadManager()
    {
        Clear();
    }

    Road* RoadManager::CreateRoad(Flag* a, Flag* b, std::vector<Vector2i> tiles)
    {
        if (!a || !b || tiles.size() < 2) return NULL;
        // Normalize: a→b and b→a are the same road.
        // Keep tiles aligned so tiles[0] == a->pos and tiles[last] == b->pos.
        bool needsReverse = (a->id > b->id);
        if (needsReverse) std::swap(a, b);
        if (GetRoadBetween(a, b)) return NULL;
        if (needsReverse) std::reverse(tiles.begin(), tiles.end());
        Road* road = new Road();
        road->id = s_nextId++;
        road->a = m_flagManager ? m_flagManager->GetFlagHandle(a) : FlagHandle();
        road->b = m_flagManager ? m_flagManager->GetFlagHandle(b) : FlagHandle();
        road->tiles = tiles;
        road->carrier = Handle<Carrier>();
        m_roads.push_back(road);

        a->roads.push_back(road);
        b->roads.push_back(road);

        return road;
    }

    void RoadManager::RemoveRoad(Road* road)
    {
        if (!road) return;

        Flag* ra = m_flagManager ? m_flagManager->ResolveFlag(road->a) : NULL;
        Flag* rb = m_flagManager ? m_flagManager->ResolveFlag(road->b) : NULL;

        if (ra) {
            for (size_t i = 0; i < ra->roads.size(); ++i) {
                if (ra->roads[i] == road) {
                    ra->roads.erase(ra->roads.begin() + i);
                    break;
                }
            }
        }
        if (rb) {
            for (size_t i = 0; i < rb->roads.size(); ++i) {
                if (rb->roads[i] == road) {
                    rb->roads.erase(rb->roads.begin() + i);
                    break;
                }
            }
        }

        for (size_t i = 0; i < m_roads.size(); ++i) {
            if (m_roads[i] == road) {
                delete m_roads[i];
                m_roads.erase(m_roads.begin() + i);
                return;
            }
        }
    }

    void RoadManager::RemoveRoadsForFlag(Flag* flag)
    {
        if (!flag) return;
        for (size_t i = 0; i < m_roads.size(); ++i) {
            Flag* ra = m_flagManager ? m_flagManager->ResolveFlag(m_roads[i]->a) : NULL;
            Flag* rb = m_flagManager ? m_flagManager->ResolveFlag(m_roads[i]->b) : NULL;
            if (ra == flag || rb == flag) {
                m_roads[i]->state = PendingDelete;
            }
        }
    }

    void RoadManager::MarkForDeletion(Road* road)
    {
        if (road) road->state = PendingDelete;
    }

    bool RoadManager::CanDestroy(Road* road, CarrierManager* cm, TransportJobManager* jm) const
    {
        if (!road) return true;
        return !cm->IsRoadInUse(road) && !jm->IsRoadInUse(road);
    }

    bool RoadManager::HasRoadsConnectedToFlag(Flag* flag) const
    {
        for (size_t i = 0; i < m_roads.size(); ++i) {
            Flag* ra = m_flagManager ? m_flagManager->ResolveFlag(m_roads[i]->a) : NULL;
            Flag* rb = m_flagManager ? m_flagManager->ResolveFlag(m_roads[i]->b) : NULL;
            if (ra == flag || rb == flag)
                return true;
        }
        return false;
    }

    Road* RoadManager::GetRoadBetween(Flag* a, Flag* b) const
    {
        if (!a || !b) return NULL;
        for (size_t i = 0; i < m_roads.size(); ++i) {
            Road* r = m_roads[i];
            Flag* ra = m_flagManager ? m_flagManager->ResolveFlag(r->a) : NULL;
            Flag* rb = m_flagManager ? m_flagManager->ResolveFlag(r->b) : NULL;
            if ((ra == a && rb == b) || (ra == b && rb == a))
                return r;
        }
        return NULL;
    }

    void RoadManager::Clear()
    {
        for (size_t i = 0; i < m_roads.size(); ++i) {
            Road* r = m_roads[i];
            Flag* ra = m_flagManager ? m_flagManager->ResolveFlag(r->a) : NULL;
            Flag* rb = m_flagManager ? m_flagManager->ResolveFlag(r->b) : NULL;
            if (ra) {
                for (size_t j = 0; j < ra->roads.size(); ++j) {
                    if (ra->roads[j] == r) {
                        ra->roads.erase(ra->roads.begin() + j);
                        break;
                    }
                }
            }
            if (rb) {
                for (size_t j = 0; j < rb->roads.size(); ++j) {
                    if (rb->roads[j] == r) {
                        rb->roads.erase(rb->roads.begin() + j);
                        break;
                    }
                }
            }
            delete r;
        }
        m_roads.clear();
    }

    std::vector<Flag*> RoadManager::FindFlagPath(Flag* start, Flag* goal) const
    {
        std::vector<Flag*> path;
        if (!start || !goal || start == goal) {
            if (start == goal && start) path.push_back(start);
            return path;
        }

        std::queue<Flag*> q;
        std::map<uint32_t, Flag*> parent;
        std::map<uint32_t, bool> visited;

        q.push(start);
        visited[start->id] = true;
        parent[start->id] = NULL;

        while (!q.empty()) {
            Flag* current = q.front();
            q.pop();

            if (current == goal) {
                Flag* node = goal;
                while (node) {
                    path.push_back(node);
                    node = parent[node->id];
                }
                std::reverse(path.begin(), path.end());
                return path;
            }

            for (size_t i = 0; i < current->roads.size(); ++i) {
                Road* r = current->roads[i];
                Flag* ra = m_flagManager ? m_flagManager->ResolveFlag(r->a) : NULL;
                Flag* rb = m_flagManager ? m_flagManager->ResolveFlag(r->b) : NULL;
                Flag* next = (ra == current) ? rb : ra;
                if (next && !visited[next->id]) {
                    visited[next->id] = true;
                    parent[next->id] = current;
                    q.push(next);
                }
            }
        }

        return path;
    }

    std::vector<RoadData> RoadManager::GetRoadData() const
    {
        std::vector<RoadData> data;
        data.reserve(m_roads.size());
        for (size_t i = 0; i < m_roads.size(); ++i) {
            Road* r = m_roads[i];
            Flag* ra = m_flagManager ? m_flagManager->ResolveFlag(r->a) : NULL;
            Flag* rb = m_flagManager ? m_flagManager->ResolveFlag(r->b) : NULL;
            RoadData rd;
            rd.id = r->id;
            rd.flagAId = ra ? ra->id : 0;
            rd.flagBId = rb ? rb->id : 0;
            rd.tiles = r->tiles;
            data.push_back(rd);
        }
        return data;
    }

    void RoadManager::LoadFromData(const std::vector<RoadData>& data, FlagManager* flagManager)
    {
        Clear();
        SetFlagManager(flagManager);
        uint32_t maxId = 0;
        for (size_t i = 0; i < data.size(); ++i) {
            const RoadData& rd = data[i];
            Flag* a = flagManager ? flagManager->GetFlagById(rd.flagAId) : NULL;
            Flag* b = flagManager ? flagManager->GetFlagById(rd.flagBId) : NULL;
            if (!a || !b) continue;
            bool needsReverse = (a->id > b->id);
            if (needsReverse) std::swap(a, b);
            if (GetRoadBetween(a, b)) continue;
            Road* road = new Road();
            road->id = rd.id;
            road->a = flagManager ? flagManager->GetFlagHandle(a) : FlagHandle();
            road->b = flagManager ? flagManager->GetFlagHandle(b) : FlagHandle();
            road->tiles = rd.tiles;
            if (needsReverse) std::reverse(road->tiles.begin(), road->tiles.end());
            road->carrier = Handle<Carrier>();
            m_roads.push_back(road);
            a->roads.push_back(road);
            b->roads.push_back(road);
            if (rd.id > maxId) maxId = rd.id;
        }
        if (maxId >= s_nextId) {
            s_nextId = maxId + 1;
        }
    }
}
