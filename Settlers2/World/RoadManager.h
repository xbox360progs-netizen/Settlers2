#pragma once
#include <vector>
#include <stdint.h>
#include "Road.h"
#include "Pathfinding.h"

namespace World {
    class Flag;
    class FlagManager;
    class CarrierManager;
    static const size_t MAX_FLAGS = 256;

    class RoadManager {
    public:
        RoadManager();
        ~RoadManager();

        void SetFlagManager(FlagManager* fm) { m_flagManager = fm; }
        FlagManager* GetFlagManager() const { return m_flagManager; }

        Road* CreateRoad(Flag* a, Flag* b, std::vector<Vector2i> tiles);
        void RemoveRoad(Road* road);
        void RemoveRoadsForFlag(Flag* flag);
        void MarkForDeletion(Road* road);
        bool CanDestroy(Road* road, CarrierManager* cm) const;
        bool HasRoadsConnectedToFlag(Flag* flag) const;
        Road* GetRoadBetween(Flag* a, Flag* b) const;
        void Clear();

        size_t GetCount() const { return m_roads.size(); }
        Road* GetRoad(size_t index) const { return (index < m_roads.size()) ? m_roads[index] : NULL; }

        // Routing — O(1) via all-pairs next-hop table (replaces BFS)
        std::vector<Flag*> FindFlagPath(Flag* start, Flag* goal) const;
        Flag* GetNextHop(Flag* src, Flag* dst);
        Pathfinding* GetPathfinding() { return &m_pathfinding; }
        const Pathfinding* GetPathfinding() const { return &m_pathfinding; }

        // Serialization
        std::vector<RoadData> GetRoadData() const;
        void LoadFromData(const std::vector<RoadData>& data, FlagManager* flagManager);

        static uint32_t s_nextId;

    private:
        void LinkFlagsWithRoad(Flag* a, Flag* b, Road* road);
        void UnlinkFlagsWithRoad(Flag* a, Flag* b);

        std::vector<Road*> m_roads;
        FlagManager* m_flagManager;
        Road* m_roadGraph[MAX_FLAGS * MAX_FLAGS];
        Pathfinding m_pathfinding;
    };
} // namespace World