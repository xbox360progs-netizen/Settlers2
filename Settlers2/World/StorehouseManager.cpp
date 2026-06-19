#include "stdafx.h"
#include "StorehouseManager.h"

namespace World {

    StorehouseManager::StorehouseManager()
        : m_activeCount(0)
    {
        for (int i = 0; i < ResourceType_Count; ++i) {
            m_globalStored[i] = 0;
            m_globalInTransit[i] = 0;
        }
    }

    int StorehouseManager::RegisterStorehouse()
    {
        if (m_activeCount >= MAX_STOREHOUSES)
            return -1;
        int idx = m_activeCount;
        m_activeIndices[m_activeCount++] = idx;
        for (int i = 0; i < ResourceType_Count; ++i)
            m_storehouses[idx].resources[i] = 0;
        return idx;
    }

    void StorehouseManager::UnregisterStorehouse(int handleIndex)
    {
        for (int i = 0; i < ResourceType_Count; ++i) {
            uint32_t stored = m_storehouses[handleIndex].resources[i];
            if (stored > m_globalStored[i])
                m_globalStored[i] = 0;
            else
                m_globalStored[i] -= stored;
        }
        for (int i = 0; i < m_activeCount; ++i) {
            if (m_activeIndices[i] == handleIndex) {
                m_activeIndices[i] = m_activeIndices[--m_activeCount];
                break;
            }
        }
    }

    Storehouse* StorehouseManager::GetStorehouse(int handleIndex)
    {
        if (handleIndex < 0 || handleIndex >= MAX_STOREHOUSES)
            return NULL;
        return &m_storehouses[handleIndex];
    }

    void StorehouseManager::AddResourceToStorehouse(int handleIndex, ResourceType type, uint32_t amount)
    {
        m_storehouses[handleIndex].resources[type] += amount;
        m_globalStored[type] += amount;
    }

    bool StorehouseManager::RemoveResourceFromStorehouse(int handleIndex, ResourceType type, uint32_t amount)
    {
        if (m_storehouses[handleIndex].resources[type] < amount)
            return false;
        m_storehouses[handleIndex].resources[type] -= amount;
        m_globalStored[type] -= amount;
        return true;
    }

    void StorehouseManager::ModifyTransitResource(ResourceType type, int delta)
    {
        if (delta > 0) {
            m_globalInTransit[type] += delta;
        } else if (delta < 0) {
            uint32_t absDelta = (uint32_t)(-delta);
            if (m_globalInTransit[type] >= absDelta)
                m_globalInTransit[type] -= absDelta;
            else
                m_globalInTransit[type] = 0;
        }
    }

    void StorehouseManager::Clear()
    {
        for (int i = 0; i < ResourceType_Count; ++i) {
            m_globalStored[i] = 0;
            m_globalInTransit[i] = 0;
        }
        m_activeCount = 0;
    }

}
