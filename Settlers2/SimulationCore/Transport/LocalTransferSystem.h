#pragma once
#include "../Systems/ISimulationSystem.h"

namespace World {

// Owns allocation decisions between ProductionBuilding and TransportNode.
// Runs local transfer (building <-> node buffer) before TransportNode::Tick()
// publishes deficits to DemandManager.
//
// Tick() does three stages:
//   1. Export building output to attached node buffer
//   2. Supply building input from attached node buffer
//   3. Tick all transport nodes (evaluate deficits/surplus)
//
// Invariant: local transfer completes before deficits are published.
class LocalTransferSystem : public ISimulationSystem {
public:
    void Tick(WorldModel& world);
};

} // namespace World
