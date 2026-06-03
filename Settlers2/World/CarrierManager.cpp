#include "stdafx.h"
#include "CarrierManager.h"
#include "Flag.h"
#include <algorithm>

namespace World {

    static bool TransportJobPriorityGreater(const TransportJob& a, const TransportJob& b) {
        return a.priority > b.priority;
    }

    void CarrierManager::AddCarrier(Carrier* carrier) {
        m_carriers.push_back(carrier);
    }

    void CarrierManager::AssignJob(const TransportJob& job) {
        m_pendingJobs.push_back(job);
    }

    void CarrierManager::SortAndAssign() {
        std::sort(m_pendingJobs.begin(), m_pendingJobs.end(), TransportJobPriorityGreater);

        for (std::vector<TransportJob>::iterator it = m_pendingJobs.begin(); it != m_pendingJobs.end();) {
            bool assigned = false;
            for (std::vector<Carrier*>::iterator carrierIt = m_carriers.begin(); carrierIt != m_carriers.end(); ++carrierIt) {
                Carrier* carrier = *carrierIt;
                if (carrier->state == Idle) {
                    carrier->currentJob = new TransportJob(*it);
                    carrier->state = WalkingToPickup;
                    assigned = true;
                    it = m_pendingJobs.erase(it);
                    break;
                }
            }
            if (!assigned) {
                ++it;
            }
        }
    }

    void CarrierManager::UpdateCarrierRange(int start, int end, float deltaTime) {
        if (start < 0) start = 0;
        if (end > (int)m_carriers.size()) end = (int)m_carriers.size();
        for (int i = start; i < end; ++i) {
            m_carriers[i]->Update(deltaTime);
        }
    }

    void CarrierManager::Update(float deltaTime) {
        SortAndAssign();
        UpdateCarrierRange(0, (int)m_carriers.size(), deltaTime);
    }
}
