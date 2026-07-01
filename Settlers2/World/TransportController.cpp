// Phase 7 — Controller stub. No logic until Phase 7.2+.

#include "TransportController.h"
#include "TransportTask.h"

namespace World {

    TransportController::TransportController()
        : m_nextTaskId(1)
        , m_activeCount(0)
    {
        for (int i = 0; i < kMaxTasks; ++i) {
            m_pool[i].id = 0;
            m_pool[i].resource = ResourceType_None;
            m_pool[i].state = TTS_Delivered;
            m_pool[i].cargo = NULL;
            m_pool[i].carrier = NULL;
        }
    }

    TransportController::~TransportController() {}

    TransportTask* TransportController::CreateTask(
        ResourceType /*resource*/,
        FlagId /*origin*/,
        FlagId /*destination*/,
        TransportTaskReason /*reason*/)
    {
        return NULL; // stub — Phase 7.2
    }

    void TransportController::CancelTask(TransportTaskId /*taskId*/) {}

    void TransportController::NotifyCarrierIdle(void* /*carrier*/, FlagId /*atFlag*/) {}
    void TransportController::NotifyCarrierReachedTarget(void* /*carrier*/, FlagId /*flagId*/) {}
    void TransportController::NotifyCarrierPickedUp(void* /*carrier*/) {}
    void TransportController::NotifyCarrierDropped(void* /*carrier*/, FlagId /*flagId*/) {}
    void TransportController::NotifyRoadNetworkChanged() {}
    void TransportController::NotifyFlagRemoved(FlagId /*flagId*/) {}

    int TransportController::GetActiveTaskCount() const { return m_activeCount; }

    TransportTask* TransportController::GetTaskById(TransportTaskId taskId)
    {
        for (int i = 0; i < kMaxTasks; ++i) {
            if (m_pool[i].id == taskId) return &m_pool[i];
        }
        return NULL;
    }

    void TransportController::Update(float /*deltaTime*/) {}

} // namespace World
