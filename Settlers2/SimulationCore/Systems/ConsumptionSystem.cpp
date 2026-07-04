#include "ConsumptionSystem.h"
#include <stddef.h>
#include <string.h>
#include "../World/WorldModel.h"
#include "../Definitions/ConsumptionDefinition.h"
#include "../Definitions/BuildingDefinition.h"
#include "../Systems/DemandManager.h"

namespace World {

    const FlagId ConsumptionSystem::kConsumptionFlagBase = 200;
    const int ConsumptionSystem::kMaxMines = 32;

    ConsumptionSystem::ConsumptionSystem()
        : m_demandManager(NULL)
        , m_tickCount(0)
    {
        memset(m_foodCycleTimers, 0, sizeof(m_foodCycleTimers));
    }

    ConsumptionSystem::~ConsumptionSystem()
    {
    }

    void ConsumptionSystem::Tick(WorldModel& world)
    {
        ++m_tickCount;
        ProcessMineFood(world);
    }

    void ConsumptionSystem::ProcessMineFood(WorldModel& world)
    {
        // Step 1: Process delivery events for mines
        for (int e = 0; e < world.deliveryEventCount; ++e) {
            const DeliveryEvent& ev = world.deliveryEvents[e];
            if (ev.type != DET_Completed) continue;
            if (ev.reason != TTR_Production) continue;
            if (ev.destinationFlag < kConsumptionFlagBase) continue;

            int idx = static_cast<int>(ev.destinationFlag - kConsumptionFlagBase);
            if (idx < 0 || idx >= world.productionBuildingCount) continue;

            ProductionBuilding& pb = world.productionBuildings[idx];
            if (!pb.active) continue;

            ProductionType pt = GetConsumptionMineType(pb.type);
            if (pt == PT_None) continue;
            if (!IsMine(pt)) continue;

            pb.foodStored += ev.amount;
        }

        // Step 2: Process each mine
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (i >= kMaxMines) break;
            ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;

            ProductionType pt = GetConsumptionMineType(pb.type);
            if (pt == PT_None) continue;
            if (!IsMine(pt)) continue;

            const ConsumptionDefinition& def = GetConsumptionDefinition(pt);

            if (pb.foodStored > 0) {
                pb.fed = true;
                m_foodCycleTimers[i]++;

                // Consume food every fedRate ticks
                if (m_foodCycleTimers[i] >= def.fedRate) {
                    m_foodCycleTimers[i] = 0;
                    pb.foodStored--;
                }
                pb.foodRequested = false; // reset for next cycle
            } else {
                pb.fed = false;
                m_foodCycleTimers[i] = 0;

                // Request food via DemandManager
                if (m_demandManager != NULL && !pb.foodRequested) {
                    FlagId flag = kConsumptionFlagBase + i;
                    // Use first available food type (simplest for v1)
                    ResourceType foodType = def.foodOptions[0].resource;
                    if (foodType != ResourceType_None) {
                        m_demandManager->SetDemand(
                            foodType,
                            def.foodPerCycle,
                            flag,
                            TBP_Normal,
                            DemandOwner_Production,
                            TTR_Production);
                    }
                    pb.foodRequested = true;
                }
            }
        }
    }

    int ConsumptionSystem::GetFoodCycleTimer(int buildingIndex) const
    {
        if (buildingIndex < 0 || buildingIndex >= kMaxMines) return 0;
        return m_foodCycleTimers[buildingIndex];
    }

    bool ConsumptionSystem::IsMineFed(int buildingIndex, const WorldModel& world) const
    {
        if (buildingIndex < 0 || buildingIndex >= world.productionBuildingCount) return false;
        return world.productionBuildings[buildingIndex].fed;
    }

}
