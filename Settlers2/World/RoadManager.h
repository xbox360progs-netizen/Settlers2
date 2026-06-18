#pragma once
#include <vector>
#include <stdint.h>
#include <unordered_map>
#include "Road.h"

namespace World {
    class Flag;
    class FlagManager;
    class CarrierManager;
    class TransportJobManager;

    struct RouteKey {
        uint32_t src;
        uint32_t dst;
        RouteKey() : src(0), dst(0) {}
        RouteKey(uint32_t s, uint32_t d) : src(s), dst(d) {}
        bool operator==(const RouteKey& o) const { return src == o.src && dst == o.dst; }
    };

    struct RouteKeyHash {
        size_t operator()(const RouteKey& k) const {
            return (size_t)(k.src ^ (k.dst << 16) ^ (k.dst >> 16));
        }
    };

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

        // Routing cache
        Flag* GetNextHop(Flag* src, Flag* dst);
        void InvalidateRouteCache() { m_routeCache.clear(); }

        // Serialization
        std::vector<RoadData> GetRoadData() const;
        void LoadFromData(const std::vector<RoadData>& data, FlagManager* flagManager);

        static uint32_t s_nextId;

    private:
        std::vector<Road*> m_roads;
        FlagManager* m_flagManager;
        std::tr1::unordered_map<RouteKey, Flag*, RouteKeyHash> m_routeCache;
    };
}
