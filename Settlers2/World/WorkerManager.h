#pragma once
#include "WorkerDefs.h"
#include "Worker.h"

namespace World {
    class RoadManager;
    class Building;

    class WorkerManager {
    public:
        WorkerManager();
        ~WorkerManager();

        void SetRoadManager(RoadManager* rm) { m_roadManager = rm; }

        int SpawnWorker(Building* home, float startX, float startY);
        void FreeWorker(int index);

        void Update(float dt);
        void Clear();

        int GetActiveCount() const { return m_activeCount; }
        const Worker* GetWorkerByActiveIdx(int i) const {
            return &m_pool[m_activeIndices[i]];
        }
        Worker* GetWorkerByActiveIdx(int i) {
            return &m_pool[m_activeIndices[i]];
        }
        Flag** GetRouteByPoolIdx(int poolIdx) {
            if (poolIdx < 0 || poolIdx >= MAX_TRANSIT_WORKERS) return NULL;
            return m_routes[poolIdx].flags;
        }
        const Flag* const* GetRouteByPoolIdx(int poolIdx) const {
            if (poolIdx < 0 || poolIdx >= MAX_TRANSIT_WORKERS) return NULL;
            return m_routes[poolIdx].flags;
        }

    private:
        Worker m_pool[MAX_TRANSIT_WORKERS];
        WorkerRoute m_routes[MAX_TRANSIT_WORKERS];  // cold parallel array
        int m_activeIndices[MAX_TRANSIT_WORKERS];
        int m_activeCount;

        int m_freeSlots[MAX_TRANSIT_WORKERS];
        int m_freeCount;

        int m_updateIndex;

        RoadManager* m_roadManager;
    };
} // namespace World