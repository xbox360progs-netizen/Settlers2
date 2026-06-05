#pragma once
#include <vector>
#include "../World/Components/Building.h"
#include "../Core/Vector2i.h"

namespace Logic {

    class ResourceRegistry {
    public:
        ResourceRegistry();
        void Register(World::Building* building);
        void Unregister(World::Building* building);

        // Planning reservation — frame-local, prevents intra-frame over-allocation
        void ReservePlanning(World::ResourceType type, int amount);
        void ReleasePlanning(World::ResourceType type, int amount);
        void ClearPlanningReservations();
        int GetPlanningReserved(World::ResourceType type) const;

        // Find nearest producer with available stock (accounting for delivery + planning reservations)
        // deliveryReserved[] — persistent commitments tracked via flag reservations
        World::Building* FindBestSupplier(
            World::ResourceType type,
            int& outAmount,
            World::Building* exclude,
            const Vector2i& requesterPos,
            const int* deliveryReserved);

        // Find nearest resource node in the world
        bool FindNearestWorldResource(World::ResourceType type, const Vector2i& pos, Vector2i& outPos);
        void RegisterWorldResource(World::ResourceType type, int x, int y);
        void UnregisterWorldResource(World::ResourceType type, int x, int y);
        
        // Populate cache from map
        void BuildWorldResourceCache(World::Map* map);
        void ClearWorldResources();

    private:
        std::vector<World::Building*> m_producers[World::ResourceType_Count];
        std::vector<Vector2i> m_worldResources[World::ResourceType_Count];
        int m_planningReserved[World::ResourceType_Count];
    };

}
