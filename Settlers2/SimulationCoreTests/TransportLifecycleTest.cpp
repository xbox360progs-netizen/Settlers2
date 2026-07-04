#include "TestRunner.h"
#include "../SimulationCore/Transport/TransportController.h"
#include "../SimulationCore/Transport/TransportTypes.h"
#include "../SimulationCore/Transport/TransportRoute.h"
#include "../SimulationCore/Transport/Carrier.h"
#include "../SimulationCore/Transport/Cargo.h"
#include "../SimulationCore/Transport/TransportTask.h"
#include "../SimulationCore/Core/ResourceTypes.h"
#include "../SimulationCore/Interfaces/IRoadGraph.h"
#include "../SimulationCore/Interfaces/IFlagInventory.h"
#include "../SimulationCore/Interfaces/ICargoRepository.h"
#include "../SimulationCore/Interfaces/IDemandService.h"

struct StubFlagInventory : public World::IFlagInventory {
    virtual bool ReceiveDelivery(World::FlagId, World::ResourceType, uint8_t, uint32_t) { return true; }
};

struct StubCargoRepository : public World::ICargoRepository {
    virtual void Release(uint32_t) {}
};

struct StubDemandService : public World::IDemandService {
    virtual void CompleteDemand(uint32_t) {}
};

struct StubRoadGraph_Direct : public World::IRoadGraph {
    virtual bool FindRoute(World::FlagId source, World::FlagId destination, World::TransportRoute& outRoute) {
        outRoute.count = 2;
        outRoute.flags[0] = source;
        outRoute.flags[1] = destination;
        return true;
    }
};

struct StubRoadGraph_ThreeHop : public World::IRoadGraph {
    virtual bool FindRoute(World::FlagId, World::FlagId, World::TransportRoute& outRoute) {
        outRoute.count = 3;
        outRoute.flags[0] = 1;
        outRoute.flags[1] = 3;
        outRoute.flags[2] = 5;
        return true;
    }
};

TEST(SingleHop_FullLifecycle) {
    StubRoadGraph_Direct roads;
    StubFlagInventory inv;
    StubCargoRepository cargoRepo;
    StubDemandService demand;
    World::TransportController ctrl(roads, inv, cargoRepo, demand);

    World::TransportTask* task = ctrl.CreateTask(
        World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    EXPECT_TRUE(task != NULL);
    EXPECT_EQ(task->state, World::TTS_WaitingAtSource);
    EXPECT_EQ(ctrl.GetWaitingCount(1), 1u);

    World::Carrier* carrier = new World::Carrier(NULL);

    ctrl.NotifyCarrierIdle(carrier, 1);
    EXPECT_EQ(task->state, World::TTS_Assigned);
    EXPECT_TRUE(task->carrier == carrier);
    EXPECT_TRUE(carrier->m_phase7Task == task);
    EXPECT_EQ(carrier->m_phase7TargetFlag, 5u);

    World::Cargo* cargo = new World::Cargo();
    cargo->id = 1;
    cargo->type = World::ResourceType_Wood;
    cargo->amount = 1;

    ctrl.NotifyCarrierPickedUp(carrier, cargo);
    EXPECT_EQ(task->state, World::TTS_Moving);
    EXPECT_TRUE(task->cargo == cargo);
    EXPECT_TRUE(cargo->ownerTask == task);
    EXPECT_TRUE(carrier->m_phase7Cargo == cargo);

    ctrl.NotifyCarrierArrived(carrier, 5);
    EXPECT_EQ(task->state, World::TTS_Delivered);
    EXPECT_TRUE(task->cargo == NULL);
    EXPECT_TRUE(task->carrier == NULL);
    EXPECT_TRUE(carrier->m_phase7Task == NULL);
    EXPECT_TRUE(carrier->m_phase7Cargo == NULL);

    EXPECT_EQ(ctrl.GetRecentDeliveryCount(), 1);
    const World::TransportController::DeliveryRecord& rec = ctrl.GetRecentDelivery(0);
    EXPECT_EQ(rec.resource, World::ResourceType_Wood);
    EXPECT_EQ(rec.destinationFlag, 5u);
    EXPECT_EQ(rec.reason, World::TTR_Construction);

    delete cargo;
    delete carrier;
}

TEST(IntermediateHop_AdvanceHop) {
    StubRoadGraph_ThreeHop roads;
    StubFlagInventory inv;
    StubCargoRepository cargoRepo;
    StubDemandService demand;
    World::TransportController ctrl(roads, inv, cargoRepo, demand);

    World::TransportTask* task = ctrl.CreateTask(
        World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    EXPECT_TRUE(task != NULL);
    EXPECT_EQ(task->state, World::TTS_WaitingAtSource);
    EXPECT_EQ(task->hopIndex, 0u);
    EXPECT_EQ(task->route.count, 3u);

    World::Carrier* carrier = new World::Carrier(NULL);

    ctrl.NotifyCarrierIdle(carrier, 1);
    EXPECT_EQ(task->state, World::TTS_Assigned);
    EXPECT_EQ(carrier->m_phase7TargetFlag, 3u);

    World::Cargo* cargo = new World::Cargo();
    cargo->id = 1;
    cargo->type = World::ResourceType_Wood;
    cargo->amount = 1;
    ctrl.NotifyCarrierPickedUp(carrier, cargo);
    EXPECT_EQ(task->state, World::TTS_Moving);

    // Arrive at intermediate flag 3 → AdvanceHop re-assigns same carrier
    ctrl.NotifyCarrierArrived(carrier, 3);
    EXPECT_EQ(task->state, World::TTS_Assigned);
    EXPECT_EQ(task->hopIndex, 1u);
    EXPECT_EQ(task->targetFlag, 5u);
    EXPECT_TRUE(task->cargo == cargo);
    EXPECT_TRUE(task->carrier == carrier);
    EXPECT_TRUE(carrier->m_phase7Task == task);
    EXPECT_EQ(carrier->m_phase7TargetFlag, 5u);
    EXPECT_EQ(ctrl.GetRecentDeliveryCount(), 0);

    // Carrier continues to second hop
    ctrl.NotifyCarrierPickedUp(carrier, cargo);
    EXPECT_EQ(task->state, World::TTS_Moving);

    ctrl.NotifyCarrierArrived(carrier, 5);
    EXPECT_EQ(task->state, World::TTS_Delivered);
    EXPECT_EQ(ctrl.GetRecentDeliveryCount(), 1);

    delete cargo;
    delete carrier;
}

TEST(CancelDuringAssignment) {
    StubRoadGraph_Direct roads;
    StubFlagInventory inv;
    StubCargoRepository cargoRepo;
    StubDemandService demand;
    World::TransportController ctrl(roads, inv, cargoRepo, demand);

    World::TransportTask* task = ctrl.CreateTask(
        World::ResourceType_Wood, 1, 5, World::TTR_Construction);

    World::Carrier* carrier = new World::Carrier(NULL);

    ctrl.NotifyCarrierIdle(carrier, 1);
    EXPECT_EQ(task->state, World::TTS_Assigned);

    ctrl.CancelTask(task->id);
    EXPECT_EQ(task->state, World::TTS_Cancelled);
    EXPECT_TRUE(task->carrier == NULL);
    EXPECT_TRUE(carrier->m_phase7Task == NULL);

    delete carrier;
}

TEST(CancelDuringMoving) {
    StubRoadGraph_Direct roads;
    StubFlagInventory inv;
    StubCargoRepository cargoRepo;
    StubDemandService demand;
    World::TransportController ctrl(roads, inv, cargoRepo, demand);

    World::TransportTask* task = ctrl.CreateTask(
        World::ResourceType_Wood, 1, 5, World::TTR_Construction);

    World::Carrier* carrier = new World::Carrier(NULL);
    ctrl.NotifyCarrierIdle(carrier, 1);

    World::Cargo* cargo = new World::Cargo();
    cargo->id = 1;
    cargo->type = World::ResourceType_Wood;
    ctrl.NotifyCarrierPickedUp(carrier, cargo);
    EXPECT_EQ(task->state, World::TTS_Moving);

    ctrl.CancelTask(task->id);
    EXPECT_EQ(task->state, World::TTS_WaitingAtSource);
    EXPECT_TRUE(task->carrier == NULL);
    EXPECT_TRUE(carrier->m_phase7Task == NULL);
    EXPECT_TRUE(carrier->m_phase7Cargo == NULL);
    EXPECT_EQ(ctrl.GetWaitingCount(1), 1u);

    delete cargo;
    delete carrier;
}

TEST(MultiHop_SameCarrier) {
    // Route 1→3→5. Same carrier handles both hops via same-carrier handoff
    // (AdvanceHop calls NotifyCarrierIdle which re-assigns the idle carrier).
    StubRoadGraph_ThreeHop roads;
    StubFlagInventory inv;
    StubCargoRepository cargoRepo;
    StubDemandService demand;
    World::TransportController ctrl(roads, inv, cargoRepo, demand);

    World::TransportTask* task = ctrl.CreateTask(
        World::ResourceType_Wood, 1, 5, World::TTR_Construction);

    World::Carrier* carrier = new World::Carrier(NULL);
    World::Cargo* cargo = new World::Cargo();
    cargo->id = 1;
    cargo->type = World::ResourceType_Wood;
    cargo->amount = 1;

    // Hop 1
    ctrl.NotifyCarrierIdle(carrier, 1);
    ctrl.NotifyCarrierPickedUp(carrier, cargo);
    ctrl.NotifyCarrierArrived(carrier, 3);
    EXPECT_EQ(task->hopIndex, 1u);
    EXPECT_EQ(task->state, World::TTS_Assigned);
    EXPECT_TRUE(task->cargo == cargo);

    // Hop 2
    ctrl.NotifyCarrierPickedUp(carrier, cargo);
    ctrl.NotifyCarrierArrived(carrier, 5);
    EXPECT_EQ(task->state, World::TTS_Delivered);
    EXPECT_EQ(ctrl.GetRecentDeliveryCount(), 1);
    const World::TransportController::DeliveryRecord& rec = ctrl.GetRecentDelivery(0);
    EXPECT_EQ(rec.resource, World::ResourceType_Wood);
    EXPECT_EQ(rec.destinationFlag, 5u);

    delete cargo;
    delete carrier;
}
