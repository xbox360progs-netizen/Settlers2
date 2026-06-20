#include "stdafx.h"
#include "RoadManager.h"
#include "Flag.h"
#include "FlagManager.h"
#include "CarrierManager.h"
#include "TransportJobManager.h"
#include <algorithm>
#include <cstring>

namespace World {

    uint32_t RoadManager::s_nextId = 1;

    RoadManager::RoadManager()
        : m_flagManager(NULL)
    {
        memset(m_roadGraph, 0, sizeof(m_roadGraph));
    }

    RoadManager::~RoadManager()
    {
        Clear();
    }

    void RoadManager::LinkFlagsWithRoad(Flag* a, Flag* b, Road* road)
    {
        if (!a || !b) return;
        uint32_t ai = a->handle.index;
        uint32_t bi = b->handle.index;
        if (ai >= MAX_FLAGS || bi >= MAX_FLAGS) return;
        m_roadGraph[ai * MAX_FLAGS + bi] = road;
        m_roadGraph[bi * MAX_FLAGS + ai] = road;
    }

    void RoadManager::UnlinkFlagsWithRoad(Flag* a, Flag* b)
    {
        if (!a || !b) return;
        uint32_t ai = a->handle.index;
        uint32_t bi = b->handle.index;
        if (ai >= MAX_FLAGS || bi >= MAX_FLAGS) return;
        m_roadGraph[ai * MAX_FLAGS + bi] = NULL;
        m_roadGraph[bi * MAX_FLAGS + ai] = NULL;
    }

    Road* RoadManager::CreateRoad(Flag* a, Flag* b, std::vector<Vector2i> tiles)
    {
        if (!a || !b || tiles.size() < 2) return NULL;
        if (GetRoadBetween(a, b)) return NULL;
        bool needsReverse = (a->id > b->id);
        if (needsReverse) {
            std::swap(a, b);
            std::reverse(tiles.begin(), tiles.end());
        }
        Road* road = new Road();
        road->id = s_nextId++;
        road->a = m_flagManager ? m_flagManager->GetFlagHandle(a) : FlagHandle();
        road->b = m_flagManager ? m_flagManager->GetFlagHandle(b) : FlagHandle();
        road->tileCount = (tiles.size() < MAX_ROAD_TILES) ? (uint32_t)tiles.size() : MAX_ROAD_TILES;
        for (uint32_t _ti = 0; _ti < road->tileCount; ++_ti) road->tiles[_ti] = tiles[_ti];
        road->carrier = Handle<Carrier>();
        m_roads.push_back(road);

        a->roads.push_back(road);
        b->roads.push_back(road);
        LinkFlagsWithRoad(a, b, road);

        m_pathfinding.RebuildRoutingTable((const void* const*)m_roadGraph, MAX_FLAGS);
        return road;
    }

    void RoadManager::RemoveRoad(Road* road)
    {
        if (!road) return;

        Flag* ra = m_flagManager ? m_flagManager->ResolveFlag(road->a) : NULL;
        Flag* rb = m_flagManager ? m_flagManager->ResolveFlag(road->b) : NULL;

        UnlinkFlagsWithRoad(ra, rb);

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
                m_pathfinding.RebuildRoutingTable((const void* const*)m_roadGraph, MAX_FLAGS);
                return;
            }
        }
    }

    void RoadManager::RemoveRoadsForFlag(Flag* flag)
    {
        if (!flag) return;
        for (size_t i = 0; i < m_roads.size(); ++i) {
            Road* r = m_roads[i];
            Flag* ra = m_flagManager ? m_flagManager->ResolveFlag(r->a) : NULL;
            Flag* rb = m_flagManager ? m_flagManager->ResolveFlag(r->b) : NULL;
            if (ra == flag || rb == flag) {
                UnlinkFlagsWithRoad(ra, rb);
                r->state = PendingDelete;
            }
        }
        m_pathfinding.RebuildRoutingTable((const void* const*)m_roadGraph, MAX_FLAGS);
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
        uint32_t ai = a->handle.index;
        uint32_t bi = b->handle.index;
        if (ai >= MAX_FLAGS || bi >= MAX_FLAGS) return NULL;
        return m_roadGraph[ai * MAX_FLAGS + bi];
    }

    Flag* RoadManager::GetNextHop(Flag* src, Flag* dst)
    {
        if (!src || !dst) return NULL;
        if (src == dst) return src;

        uint8_t nextIdx = m_pathfinding.GetNextFlagIdx(src->handle.index, dst->handle.index);
        if (nextIdx == PATH_NO_ROUTE) return NULL;
        return m_flagManager ? m_flagManager->GetFlagByIndex(nextIdx) : NULL;
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
        memset(m_roadGraph, 0, sizeof(m_roadGraph));
        m_pathfinding.Clear();
    }

    std::vector<Flag*> RoadManager::FindFlagPath(Flag* start, Flag* goal) const
    {
        std::vector<Flag*> path;
        if (!start || !goal) return path;
        if (start == goal) {
            path.push_back(start);
            return path;
        }

        uint8_t startIdx = (uint8_t)start->handle.index;
        uint8_t goalIdx = (uint8_t)goal->handle.index;

        // Reconstruct path from routing table: follow next-hop chain
        uint8_t cur = startIdx;
        uint16_t guard = (uint16_t)PATH_MAX_FLAGS; // prevent infinite loop
        while (cur != goalIdx && guard > 0) {
            Flag* f = m_flagManager ? m_flagManager->GetFlagByIndex(cur) : NULL;
            if (!f) return std::vector<Flag*>(); // stale index
            path.push_back(f);
            cur = m_pathfinding.GetNextFlagIdx(cur, goalIdx);
            if (cur == PATH_NO_ROUTE) return std::vector<Flag*>();
            --guard;
        }
        if (guard == 0) return std::vector<Flag*>(); // cycle detected

        // Add goal flag
        Flag* gf = m_flagManager ? m_flagManager->GetFlagByIndex(goalIdx) : NULL;
        if (gf) path.push_back(gf);

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
            rd.tileCount = r->tileCount;
            for (uint32_t _ti = 0; _ti < rd.tileCount; ++_ti) rd.tiles[_ti] = r->tiles[_ti];
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
            road->tileCount = rd.tileCount;
            for (uint32_t _ti = 0; _ti < road->tileCount; ++_ti) road->tiles[_ti] = rd.tiles[_ti];
            if (needsReverse) {
                for (uint32_t _ti = 0; _ti < road->tileCount / 2; ++_ti) {
                    Vector2i _tmp = road->tiles[_ti];
                    road->tiles[_ti] = road->tiles[road->tileCount - 1 - _ti];
                    road->tiles[road->tileCount - 1 - _ti] = _tmp;
                }
            }
            road->carrier = Handle<Carrier>();
            m_roads.push_back(road);
            a->roads.push_back(road);
            b->roads.push_back(road);
            LinkFlagsWithRoad(a, b, road);
            if (rd.id > maxId) maxId = rd.id;
        }
        if (maxId >= s_nextId) {
            s_nextId = maxId + 1;
        }
        m_pathfinding.RebuildRoutingTable((const void* const*)m_roadGraph, MAX_FLAGS);
    }
} // namespace World