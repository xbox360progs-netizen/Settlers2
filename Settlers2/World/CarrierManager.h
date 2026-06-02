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

    private:
        std::vector<Carrier*> m_carriers;
        std::vector<TransportJob> m_pendingJobs;
    };
}
