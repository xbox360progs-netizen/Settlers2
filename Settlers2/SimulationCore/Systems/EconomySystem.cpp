#include "EconomySystem.h"
#include "../World/WorldModel.h"
#include "../Definitions/BuildingDefinition.h"
#include "../Definitions/ProductionDefinition.h"

namespace World {

    EconomySystem::EconomySystem()
        : m_tickCount(0)
        , m_flowElapsed(0)
    {
        for (int r = 0; r < kResourceCount; ++r) {
            m_totalProduced[r] = 0;
            m_totalConsumed[r] = 0;
            m_currentFlow[r] = 0;
            m_flowAccumulator[r] = 0;
        }
        for (int b = 0; b < kMaxBuildings; ++b) {
            for (int p = 0; p < kMaxBuildingSlots; ++p) {
                m_lastOutput[b][p] = 0;
            }
        }
    }

    EconomySystem::~EconomySystem()
    {
    }

    void EconomySystem::Tick(WorldModel& world)
    {
        ScanProduction(world);
        ++m_tickCount;
    }

    void EconomySystem::ScanProduction(WorldModel& world)
    {
        for (int i = 0; i < world.productionBuildingCount && i < kMaxBuildings; ++i) {
            ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;

            BuildingType bt = pb.type;
            ProductionType pt = GetBuildingDefinition(bt).production;
            const ProductionDefinition& def = GetProductionDefinition(pt);

            for (int p = 0; p < kMaxBuildingSlots; ++p) {
                if (def.produces[p].resource == ResourceType_None) continue;
                if (def.produces[p].amount <= 0) continue;

                ResourceType res = def.produces[p].resource;
                int currentOutput = pb.totalOutput[p];
                int delta = currentOutput - m_lastOutput[i][p];

                if (delta > 0) {
                    int resIdx = static_cast<int>(res);
                    int produced = delta * def.produces[p].amount;
                    m_totalProduced[resIdx] += produced;
                    m_flowAccumulator[resIdx] += produced;
                    m_lastOutput[i][p] = currentOutput;

                    // Derive consumption: each output unit consumes inputs
                    for (int c = 0; c < kMaxBuildingSlots; ++c) {
                        if (def.consumes[c].resource == ResourceType_None) continue;
                        if (def.consumes[c].amount <= 0) continue;
                        m_totalConsumed[static_cast<int>(def.consumes[c].resource)] += delta * def.consumes[c].amount;
                    }
                }
            }
        }

        // Advance flow window — snapshot accumulator into current flow rate
        m_flowElapsed++;
        if (m_flowElapsed >= kFlowWindow) {
            for (int r = 0; r < kResourceCount; ++r) {
                m_currentFlow[r] = m_flowAccumulator[r];
                m_flowAccumulator[r] = 0;
            }
            m_flowElapsed = 0;
        }
    }

    int EconomySystem::GetTotalProduced(ResourceType type) const
    {
        int idx = static_cast<int>(type);
        if (idx < 0 || idx >= kResourceCount) return 0;
        return m_totalProduced[idx];
    }

    int EconomySystem::GetTotalConsumed(ResourceType type) const
    {
        int idx = static_cast<int>(type);
        if (idx < 0 || idx >= kResourceCount) return 0;
        return m_totalConsumed[idx];
    }

    int EconomySystem::GetResourceFlow(ResourceType type) const
    {
        int idx = static_cast<int>(type);
        if (idx < 0 || idx >= kResourceCount) return 0;
        return m_currentFlow[idx];
    }

    float EconomySystem::GetProductionPotential(ResourceType type, const WorldModel& world) const
    {
        float total = 0.0f;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            const ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;
            BuildingType bt = pb.type;
            ProductionType pt = GetBuildingDefinition(bt).production;
            if (pt == PT_None) continue;
            const ProductionDefinition& def = GetProductionDefinition(pt);
            for (int p = 0; p < kMaxBuildingSlots; ++p) {
                if (def.produces[p].resource == type && def.produces[p].amount > 0) {
                    total += (float)def.produces[p].amount / (float)def.cycleTime;
                    break;
                }
            }
        }
        return total;
    }

    int EconomySystem::GetAvailable(ResourceType type, const WorldModel& world) const
    {
        // Observational: sum outputBuffer across all production buildings.
        // No world mutation — pure query of current state.
        // Future: extend to include warehouse stockpiles, flag inventories,
        // and subtract reservations. Callers never change.
        int total = 0;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            const ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;
            for (int s = 0; s < kMaxBuildingSlots; ++s) {
                if (pb.outputResources[s] == type) {
                    total += pb.outputBuffer[s];
                }
            }
        }
        return total;
    }

    int EconomySystem::GetBuildingCount(ProductionType type, const WorldModel& world) const
    {
        int count = 0;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            const ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;
            if (GetBuildingDefinition(pb.type).production == type) {
                ++count;
            }
        }
        return count;
    }

    int EconomySystem::GetDemandBacklog(ResourceType type, const WorldModel& world) const
    {
        int count = 0;
        for (int i = 0; i < world.pendingRequestCount; ++i) {
            if (!world.pendingRequests[i].fulfilled && world.pendingRequests[i].resource == type) {
                ++count;
            }
        }
        return count;
    }

} // namespace World
