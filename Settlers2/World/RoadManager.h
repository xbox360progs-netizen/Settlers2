#pragma once
#include <vector>
#include <stdint.h>
#include "Road.h"

namespace World {
    class Flag;
    class FlagManager;
    class CarrierManager;
    class TransportJobManager;

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
    bool CanDestroy(Road* road, CarrierManager* cm, TransportJobManager* jm) const;
    bool HasRoadsConnectedToFlag(Flag* flag) const;
    Road* GetRoadBetween(Flag* a, Flag* b) const;
        void Clear();

        size_t GetCount() const { return m_roads.size(); }
        Road* GetRoad(size_t index) const { return (index < m_roads.size()) ? m_roads[index] : NULL; }

        std::vector<Flag*> FindFlagPath(Flag* start, Flag* goal) const;

        // Serialization
        std::vector<RoadData> GetRoadData() const;
        void LoadFromData(const std::vector<RoadData>& data, FlagManager* flagManager);

        static uint32_t s_nextId;

    private:
        std::vector<Road*> m_roads;
        FlagManager* m_flagManager;
    };
}
