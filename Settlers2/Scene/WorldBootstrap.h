#pragma once

#include <vector>

namespace Core {
    class EventBus;
    class CommandBus;
}
namespace World {
    class AnimalManager;
    class AnimalSystem;
    class CargoManager;
    class CarrierManager;
    class CarrierSystem;
    class ConstructionManager;
    class DemandManager;
    class EntityManager;
    struct FlagData;
    class FlagManager;
    class Map;
    class ObjectLifecycleManager;
    struct RoadData;
    class RoadManager;
    class SimulationSystem;
    class StorehouseManager;
    class TransportController;
    class TransportJobManager;
    class WildlifeSystem;
    class WorkerManager;
    class RoadNetworkRelinker;
}
namespace Logic {
    class AISystem;
    class EconomyManager;
}
namespace Scene {
    class BuildingPlacementManager;
    class ConstructionVisualizer;
    class RoadController;
    class WorldRestorer;
}

struct WorldBootstrapCtx {
    Core::EventBus*& eventBus;
    Core::CommandBus*& commandBus;
    World::EntityManager*& entityManager;
    World::AnimalSystem*& animalSystem;
    World::AnimalManager*& animalManager;
    World::WildlifeSystem*& wildlife;
    Logic::EconomyManager*& economy;
    World::CarrierSystem*& carrierSystem;
    World::CarrierManager*& carrierManager;
    World::WorkerManager*& workerManager;
    Logic::AISystem*& aiSystem;
    World::FlagManager*& flagManager;
    World::RoadManager*& roadManager;
    World::TransportJobManager*& transportJobs;
    World::TransportController*& transportController;
    World::CargoManager*& cargo;
    World::DemandManager*& demand;
    World::StorehouseManager*& storehouse;
    World::ConstructionManager*& construction;
    World::ObjectLifecycleManager*& lifecycle;
    Scene::BuildingPlacementManager*& placement;
    Scene::ConstructionVisualizer*& visualizer;
    Scene::RoadController* roadController;
    World::RoadNetworkRelinker* relinker;
};

namespace WorldBootstrap {
    void SetupSystems(World::Map* map,
                      const std::vector<World::FlagData>& flagData,
                      const std::vector<World::RoadData>& roadData,
                      World::SimulationSystem& simulation,
                      Scene::WorldRestorer& restorer,
                      WorldBootstrapCtx& ctx);

    void InitializeMapSprites(World::Map* map);

    void CreateStartingHQ(
        World::Map* map,
        Logic::EconomyManager* economy,
        World::FlagManager* flagManager,
        World::CarrierManager* carrierManager,
        World::StorehouseManager* storehouse,
        World::TransportJobManager* transportJobs,
        World::ConstructionManager* construction,
        World::DemandManager* demand,
        World::RoadNetworkRelinker& relinker);
}
