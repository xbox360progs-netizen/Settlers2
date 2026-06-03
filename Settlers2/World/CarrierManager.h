#pragma once
#include <vector>
#include "Carrier.h"
#include "TransportJob.h"

namespace World {

    class CarrierManager {
    public:
        void AddCarrier(Carrier* carrier);
        void AssignJob(const TransportJob& job);
        void Update(float deltaTime);

        int GetCarrierCount() const { return (int)m_carriers.size(); }
        void SortAndAssign();
        void UpdateCarrierRange(int start, int end, float deltaTime);

    private:
        std::vector<Carrier*> m_carriers;
        std::vector<TransportJob> m_pendingJobs;
    };
}
