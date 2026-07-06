#include "LocalTransferSystem.h"
#include "../World/WorldModel.h"

namespace World {

static bool BuildingNeedsResource(const ProductionBuilding& building, ResourceType r)
{
    for (int j = 0; j < kMaxProductionInputs; ++j) {
        if (building.inputResources[j] == r && building.inputDelivered[j] < building.inputRequired[j]) {
            return true;
        }
    }
    return false;
}

void LocalTransferSystem::Tick(WorldModel& world)
{
    // Stage 1 & 2: Local transfer per node — export + supply
    for (int n = 0; n < world.transportNodeCount; ++n)
    {
        TransportNode& node = world.transportNodes[n];

        for (int a = 0; a < node.attachmentCount; ++a)
        {
            BuildingAttachment& att = node.attachments[a];
            if (att.buildingId >= world.productionBuildingCount) continue;

            ProductionBuilding& building = world.productionBuildings[att.buildingId];
            if (!building.active) continue;

            // Export: building output -> node buffer
            if (att.role == AR_Producer || att.role == AR_ProducerConsumer)
            {
                for (int o = 0; o < kMaxProductionInputs; ++o)
                {
                    ResourceType r = building.outputResources[o];
                    if (r == ResourceType_None) continue;

                    while (building.outputBuffer[o] > 0 && node.HasCapacity(r, 1))
                    {
                        node.ReceiveExport(r, 1);
                        building.outputBuffer[o]--;
                    }
                }
            }

            // Supply: node buffer -> building input
            if (att.role == AR_Consumer || att.role == AR_ProducerConsumer)
            {
                for (int i = 0; i < att.inputCount; ++i)
                {
                    ResourceType r = att.inputs[i];
                    if (r == ResourceType_None) continue;

                    // Only supply if building still needs this resource
                    if (!BuildingNeedsResource(building, r)) continue;

                    // Find the matching input slot to increment
                    for (int j = 0; j < kMaxProductionInputs; ++j)
                    {
                        if (building.inputResources[j] == r && building.inputDelivered[j] < building.inputRequired[j])
                        {
                            while (building.inputDelivered[j] < building.inputRequired[j] && node.TakeForBuilding(att.buildingId, r, 1))
                            {
                                building.inputDelivered[j]++;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

    // Stage 3: Evaluate deficits and set pendingDemand (with building state visibility)
    for (int n = 0; n < world.transportNodeCount; ++n)
    {
        TransportNode& node = world.transportNodes[n];

        // Clear existing demand
        for (int d = 0; d < kMaxNodeDemands; ++d) {
            node.pendingDemand[d].active = false;
        }
        int demandCount = 0;

        for (int a = 0; a < node.attachmentCount && demandCount < kMaxNodeDemands; ++a)
        {
            if (node.attachments[a].role == AR_Producer) continue;
            if (node.attachments[a].buildingId >= world.productionBuildingCount) continue;

            ProductionBuilding& building = world.productionBuildings[node.attachments[a].buildingId];
            if (!building.active) continue;

            for (int i = 0; i < node.attachments[a].inputCount && demandCount < kMaxNodeDemands; ++i)
            {
                ResourceType r = node.attachments[a].inputs[i];
                if (r == ResourceType_None) continue;

                // Only publish demand if building STILL needs this resource after local transfer
                if (BuildingNeedsResource(building, r) && node.FindDemand(r) < 0)
                {
                    node.pendingDemand[demandCount].resource = r;
                    node.pendingDemand[demandCount].amount = 1;
                    node.pendingDemand[demandCount].active = true;
                    node.pendingDemand[demandCount].targetFlag = (FlagId)(kNodeDemandFlagBase + node.id);
                    ++demandCount;
                }
            }
        }

        // outgoingCount = buffer resources not in active deficit
        node.outgoingCount = 0;
        for (int s = 0; s < node.buffer.slotCount; ++s) {
            ResourceType r = node.buffer.slots[s].type;
            if (r == ResourceType_None) continue;
            if (node.FindDemand(r) < 0) {
                node.outgoingCount += node.buffer.slots[s].amount;
            }
        }
    }
}

} // namespace World
