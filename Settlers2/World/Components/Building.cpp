#include "stdafx.h"
#include "Building.h"
#include "Hunter.h"
#include "Fisher.h"

namespace World {

const float Hunter::IDLE_DURATION = 15.0f;
const float Hunter::HUNT_DURATION = 5.0f;
const float Hunter::WORKER_SPEED = 2.0f;
const float Fisher::IDLE_DURATION = 2.0f;
const float Fisher::FISHING_DURATION = 3.0f;

    void Building::Update(float dt)
    {
        if (state != State_Finished) return;

        switch (m_fsmState) {
        case BuildingFSM_Idle:
            UpdateIdle(dt);
            break;
        case BuildingFSM_Producing:
            UpdateProducing(dt);
            break;
        case BuildingFSM_OutputFull:
            UpdateOutputFull(dt);
            break;
        }
    }

    void Building::UpdateIdle(float /*dt*/)
    {
        if (CanProduce())
            m_fsmState = BuildingFSM_Producing;
    }

    void Building::UpdateProducing(float dt)
    {
        m_productionTimer += dt;

        while (m_productionTimer >= m_productionInterval)
        {
            m_productionTimer -= m_productionInterval;

            if (!ProduceOne())
            {
                m_fsmState = BuildingFSM_OutputFull;
                break;
            }
        }
    }

    void Building::UpdateOutputFull(float /*dt*/)
    {
        if (!IsOutputFull())
            m_fsmState = BuildingFSM_Idle;
    }

    bool Building::IsOutputFull() const
    {
        for (size_t i = 0; i < outputResources.size(); ++i) {
            if (m_storage[outputResources[i]] >= 5)
                return true;
        }
        return false;
    }

    bool Building::AddOutput(ResourceType type, int amount)
    {
        char dbg[256];
        _snprintf(dbg, sizeof(dbg), "[Building] AddOutput type=%d amount=%d current=%d capacity=5\n",
                 (int)type, amount, m_storage[type]);
        OutputDebugStringA(dbg);

        if (IsOutputFull()) {
            OutputDebugStringA("[Building] AddOutput FAILED: output full\n");
            return false;
        }
        m_storage[type] += amount;

        _snprintf(dbg, sizeof(dbg), "[Building] AddOutput OK storage=%d\n", m_storage[type]);
        OutputDebugStringA(dbg);
        return true;
    }

}
