#include "stdafx.h"
#include "Building.h"

namespace World {

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
        if (IsOutputFull()) return false;
        m_storage[type] += amount;
        return true;
    }

}
