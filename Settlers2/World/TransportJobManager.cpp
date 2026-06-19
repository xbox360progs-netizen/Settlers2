#include "stdafx.h"
#include "TransportJobManager.h"

namespace World {

    TransportJobManager::TransportJobManager()
        : m_activeCount(0), m_freeCount(MAX_TRANSPORT_JOBS), m_nextJobId(1)
        , m_flagManager(NULL), m_roadManager(NULL), m_carrierManager(NULL), m_warehouse(NULL)
    {
        for (int i = 0; i < MAX_TRANSPORT_JOBS; ++i) {
            m_pool[i].Clear();
            m_freeSlots[i] = (MAX_TRANSPORT_JOBS - 1) - i;
        }
    }

    TransportJobManager::~TransportJobManager()
    {
    }

    TransportJob* TransportJobManager::CreateJob(ResourceType rType, Flag* src, Flag* dst)
    {
        if (m_freeCount == 0) return NULL;

        uint32_t index = m_freeSlots[--m_freeCount];
        TransportJob& job = m_pool[index];
        job.id = m_nextJobId++;
        job.resourceType = rType;
        job.sourceFlag = src;
        job.targetFlag = dst;
        job.state = JobState_Pending;

        m_activeIndices[m_activeCount++] = index;
        return &job;
    }

    void TransportJobManager::FreeJob(TransportJob* job)
    {
        if (!job) return;

        uint32_t index = (uint32_t)(job - m_pool);
        if (index >= MAX_TRANSPORT_JOBS) return;

        m_pool[index].Clear();

        // Swap-and-pop from active indices
        for (int i = 0; i < m_activeCount; ++i) {
            if (m_activeIndices[i] == index) {
                m_activeIndices[i] = m_activeIndices[--m_activeCount];
                break;
            }
        }

        m_freeSlots[m_freeCount++] = index;
    }

    void TransportJobManager::Clear()
    {
        for (int i = 0; i < MAX_TRANSPORT_JOBS; ++i) {
            m_pool[i].Clear();
            m_freeSlots[i] = (MAX_TRANSPORT_JOBS - 1) - i;
        }
        m_activeCount = 0;
        m_freeCount = MAX_TRANSPORT_JOBS;
    }

}
