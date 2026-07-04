#include "ProductionSystem.h"
#include <stddef.h>
#include "../World/WorldModel.h"
#include "../Definitions/BuildingDefinition.h"
#include "../Definitions/ProductionDefinition.h"
#include "../Core/BuildingTypes.h"
#include "../Definitions/ConsumptionDefinition.h"
#include "../Systems/RenewableResourceSystem.h"
#include "../Systems/DemandManager.h"

namespace World {

    const FlagId ProductionSystem::kProductionFlagBase = 100;

    ProductionSystem::ProductionSystem()
        : m_demandManager(NULL)
        , m_renewableSystem(NULL)
        , m_consumptionEnabled(false)
        , m_tickCount(0)
    {
    }

    ProductionSystem::~ProductionSystem()
    {
    }

    void ProductionSystem::Tick(WorldModel& world)
    {
        ++m_tickCount;

        HandleDeliveryEvents(world);
        ProcessProduction(world);
    }

    // Data-driven production: reads BuildingDefinition -> ProductionDefinition chain.
    // No switch statements on building types.
    void ProductionSystem::ProcessProduction(WorldModel& world)
    {
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;

            const BuildingDefinition& bldDef = GetBuildingDefinition(pb.type);
            if (bldDef.production == PT_None) continue;

            const ProductionDefinition& prodDef = GetProductionDefinition(bldDef.production);

            // Check if this building is a mine that requires food
            if (m_consumptionEnabled && IsMine(bldDef.production) && !pb.fed) {
                continue;
            }

            // Check if this building needs inputs
            bool needsInputs = false;
            for (int c = 0; c < kMaxProductionInputs; ++c) {
                if (pb.inputResources[c] != ResourceType_None && pb.inputRequired[c] > 0) {
                    needsInputs = true;
                    break;
                }
            }

            if (needsInputs) {
                // Check if all inputs for current cycle are delivered
                bool allDelivered = true;
                for (int c = 0; c < kMaxProductionInputs; ++c) {
                    if (pb.inputResources[c] != ResourceType_None && pb.inputRequired[c] > 0) {
                        if (pb.inputDelivered[c] < pb.inputRequired[c]) {
                            allDelivered = false;
                            break;
                        }
                    }
                }

                if (allDelivered) {
                    // Inputs ready — run production cycle
                    if (pb.cycleTimer >= prodDef.cycleTime) {
                        pb.cycleTimer = 0;

                        // Consume inputs
                        for (int c = 0; c < kMaxProductionInputs; ++c) {
                            pb.inputDelivered[c] = 0;
                        }
                        pb.inputsRequested = false;

                        // Buffer output — accumulated in building inventory
                        for (int p = 0; p < kMaxProductionInputs; ++p) {
                            const ResourceAmount& produce = prodDef.produces[p];
                            if (produce.resource == ResourceType_None || produce.amount <= 0) continue;
                            pb.outputBuffer[p]++;
                            pb.totalOutput[p]++;
                        }
                    } else {
                        pb.cycleTimer++;
                    }
                } else if (!pb.inputsRequested && m_demandManager != NULL) {
                    // Request inputs via DemandManager
                    FlagId flag = kProductionFlagBase + i;
                    for (int c = 0; c < kMaxProductionInputs; ++c) {
                        if (pb.inputResources[c] != ResourceType_None && pb.inputRequired[c] > 0) {
                            uint32_t need = pb.inputRequired[c] - pb.inputDelivered[c];
                            m_demandManager->SetDemand(pb.inputResources[c], need, flag, TBP_Normal,
                                DemandOwner_Production, TTR_Production);
                        }
                    }
                    pb.inputsRequested = true;
                }
                // else: still waiting for input delivery, do nothing
            } else {
                // No inputs needed — run production cycle
                if (pb.cycleTimer >= prodDef.cycleTime) {
                    pb.cycleTimer = 0;

                    // Check renewable resource availability
                    if (m_renewableSystem != NULL && !m_renewableSystem->OnProductionCycle(pb.type, world)) {
                        continue;
                    }

                    for (int p = 0; p < kMaxProductionInputs; ++p) {
                        const ResourceAmount& produce = prodDef.produces[p];
                        if (produce.resource == ResourceType_None || produce.amount <= 0) continue;
                        pb.outputBuffer[p]++;
                        pb.totalOutput[p]++;
                    }
                } else {
                    pb.cycleTimer++;
                }
            }
        }
    }

    void ProductionSystem::HandleDeliveryEvents(WorldModel& world)
    {
        for (int i = 0; i < world.deliveryEventCount; ++i) {
            const DeliveryEvent& ev = world.deliveryEvents[i];
            if (ev.type != DET_Completed) continue;
            if (ev.reason != TTR_Production) continue;

            if (ev.destinationFlag < kProductionFlagBase) continue;
            int idx = static_cast<int>(ev.destinationFlag - kProductionFlagBase);
            if (idx < 0 || idx >= world.productionBuildingCount) continue;

            ProductionBuilding& pb = world.productionBuildings[idx];
            if (!pb.active) continue;

            for (int c = 0; c < kMaxProductionInputs; ++c) {
                if (pb.inputResources[c] == ev.resource && pb.inputDelivered[c] < pb.inputRequired[c]) {
                    pb.inputDelivered[c]++;
                    break;
                }
            }
        }
    }

} // namespace World