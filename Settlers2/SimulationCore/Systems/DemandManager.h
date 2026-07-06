#pragma once
#include <stdint.h>
#include "ISimulationSystem.h"
#include "../Core/ResourceTypes.h"
#include "../Transport/TransportTypes.h"
#include "../Interfaces/IDemandService.h"
#include "../World/Demand.h"

namespace World {

    struct WorldModel;

    class DemandManager : public ISimulationSystem, public IDemandService {
    public:
        DemandManager();
        ~DemandManager();

        virtual void Tick(WorldModel& world);

        void SetDemand(ResourceType type, uint32_t amount, FlagId targetFlag, int priority, DemandOwner owner = DemandOwner_Construction, TransportTaskReason reason = TTR_Construction);
        void ClearDemand(FlagId targetFlag);
        void ClearDemand(ResourceType type, FlagId targetFlag);

        // IDemandService
        virtual void CompleteDemand(uint32_t observerTicketId);
        virtual void OnTaskCreated(uint32_t demandIndex, uint32_t taskId);

        // Testability — expose demand state
        int GetDemandCount() const { return m_demandCount; }
        ResourceType GetDemandType(int index) const { return (index >= 0 && index < m_demandCount) ? m_demands[index].type : ResourceType_None; }
        uint32_t GetDemandRemaining(int index) const { return (index >= 0 && index < m_demandCount) ? m_demands[index].remaining : 0; }
        DemandOwner GetDemandOwner(int index) const { return (index >= 0 && index < m_demandCount) ? m_demands[index].owner : DemandOwner_Construction; }
        TransportTaskReason GetDemandReason(int index) const { return (index >= 0 && index < m_demandCount) ? m_demands[index].reason : TTR_Construction; }

    private:
        DemandManager(const DemandManager&);
        void operator=(const DemandManager&);

        void PublishTransportRequests(WorldModel& world);

        static const int kMaxDemands = 64;

        Demand m_demands[kMaxDemands];
        int m_demandCount;
        uint32_t m_tickCount;
    };

} // namespace World
