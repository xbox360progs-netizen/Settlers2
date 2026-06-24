#include "stdafx.h"
#include "WorkforceSystem.h"
#include "../WorkerManager.h"
#include "../RoadManager.h"
#include "../Map.h"
#include "../../Core/EventBus.h"

namespace World {

WorkforceSystem::WorkforceSystem()
    : m_workerManager(NULL)
    , m_roadManager(NULL)
    , m_map(NULL)
    , m_eventBus(NULL)
    , m_ownsManager(false)
{
}

WorkforceSystem::~WorkforceSystem()
{
    if (m_eventBus) {
        m_eventBus->UnregisterAll(this);
    }
    if (m_ownsManager && m_workerManager) {
        delete m_workerManager;
        m_workerManager = NULL;
    }
}

void WorkforceSystem::SetExternalManager(WorkerManager* mgr)
{
    m_workerManager = mgr;
    m_ownsManager = false;
}

void WorkforceSystem::Initialize(RoadManager* roadManager, Map* map, Core::EventBus* eventBus)
{
    m_roadManager = roadManager;
    m_map = map;
    m_eventBus = eventBus;

    if (!m_workerManager) {
        m_workerManager = new WorkerManager();
        m_ownsManager = true;
    }

    m_workerManager->SetRoadManager(m_roadManager);

    if (m_eventBus) {
        m_eventBus->Register(Core::Event_ConstructionComplete, this);
    }
}

void WorkforceSystem::Update(float dt)
{
    if (m_workerManager) {
        m_workerManager->Update(dt);
    }
}

int WorkforceSystem::GetActiveWorkerCount() const
{
    return m_workerManager ? m_workerManager->GetActiveCount() : 0;
}

void WorkforceSystem::OnEvent(Core::EventType type, void* data)
{
    (void)data;
    (void)type;
}

} // namespace World
