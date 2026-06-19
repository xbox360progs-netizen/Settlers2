#pragma once
#include <vector>
#include "Worker.h"
#include "Warehouse.h"

namespace World {

    class RoadManager;

    class WorkerManager {
    public:
        WorkerManager();
        ~WorkerManager();

        void SetWarehouse(Warehouse* wh) { m_warehouse = wh; }
        void SetRoadManager(RoadManager* rm) { m_roadManager = rm; }
        void SpawnWorker(Building* home, float startX, float startY);
        void Update(float dt);
        void Clear();

        size_t GetCount() const { return m_transit.size(); }
        Worker* GetWorker(size_t idx) const { return m_transit[idx]; }

    private:
        std::vector<Worker*> m_transit;
        Warehouse* m_warehouse;
        RoadManager* m_roadManager;
    };

} // namespace World
