#include "WarehouseSystem.h"
#include <stddef.h>
#include "../World/WorldModel.h"
#include "../Systems/DemandManager.h"
#include "../Definitions/BuildingDefinition.h"
#include "../Definitions/ProductionDefinition.h"

namespace World {

    const FlagId WarehouseSystem::kWarehouseFlag = 250;

    WarehouseSystem::WarehouseSystem()
        : m_demandManager(NULL)
        , m_tickCount(0)
        , m_stockpileCount(0)
    {
        for (int i = 0; i < kMaxStockpile; ++i) {
            m_stockpile[i].type = ResourceType_None;
            m_stockpile[i].amount = 0;
        }
    }

    WarehouseSystem::~WarehouseSystem()
    {
    }

    int WarehouseSystem::GetStockpileAmount(ResourceType type) const
    {
        for (int i = 0; i < m_stockpileCount; ++i) {
            if (m_stockpile[i].type == type)
                return m_stockpile[i].amount;
        }
        return 0;
    }

    void WarehouseSystem::Tick(WorldModel& world)
    {
        ++m_tickCount;
        HandleDeliveryEvents(world);
        ScanTransportBuffers(world);
    }

    void WarehouseSystem::HandleDeliveryEvents(WorldModel& world)
    {
        for (int i = 0; i < world.deliveryEventCount; ++i) {
            const DeliveryEvent& ev = world.deliveryEvents[i];
            if (ev.type != DET_Completed) continue;
            if (ev.destinationFlag != kWarehouseFlag) continue;

            // Add to stockpile
            int idx = -1;
            for (int s = 0; s < m_stockpileCount; ++s) {
                if (m_stockpile[s].type == ev.resource) {
                    idx = s;
                    break;
                }
            }
            if (idx < 0 && m_stockpileCount < kMaxStockpile) {
                idx = m_stockpileCount++;
                m_stockpile[idx].type = ev.resource;
            }
            if (idx >= 0) {
                m_stockpile[idx].amount++;
            }

            // Decrement source transport node's buffer (resources already exported
            // from building to node by LocalTransferSystem)
            for (int n = 0; n < world.transportNodeCount; ++n) {
                TransportNode& node = world.transportNodes[n];
                if (node.buffer.Remove(ev.resource, 1) > 0) {
                    break;
                }
            }
        }
    }

    void WarehouseSystem::ScanTransportBuffers(WorldModel& world)
    {
        if (!m_demandManager) return;

        for (int n = 0; n < world.transportNodeCount; ++n) {
            TransportNode& node = world.transportNodes[n];
            if (node.outgoingCount <= 0) continue;

            for (int s = 0; s < kNodeBufferSlots; ++s) {
                ResourceType r = node.buffer.slots[s].type;
                if (r == ResourceType_None) continue;
                if (node.buffer.slots[s].amount <= 0) continue;
                if (node.FindDemand(r) >= 0) continue;

                m_demandManager->SetDemand(
                    r,
                    1,
                    kWarehouseFlag,
                    TBP_Normal,
                    DemandOwner_Production,
                    TTR_WarehouseBalance);
            }
        }
    }

} // namespace World
