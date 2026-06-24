#pragma once
#include "../../Core/EventBus.h"

namespace World {
    class WorkerManager;
    class RoadManager;
    class Map;
}

namespace World {

class WorkforceSystem : public Core::EventListener {
public:
    WorkforceSystem();
    ~WorkforceSystem();

    // External manager mode
    void SetExternalManager(WorkerManager* mgr);

    void Initialize(RoadManager* roadManager, Map* map, Core::EventBus* eventBus);

    void Update(float dt);

    WorkerManager* GetWorkerManager() { return m_workerManager; }

    int GetActiveWorkerCount() const;

    virtual void OnEvent(Core::EventType type, void* data);

private:
    WorkerManager* m_workerManager;
    RoadManager* m_roadManager;
    Map* m_map;
    Core::EventBus* m_eventBus;
    bool m_ownsManager;
};

} // namespace World
