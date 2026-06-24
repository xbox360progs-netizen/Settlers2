#pragma once
#include "../../Core/EventBus.h"

namespace World {
    class CarrierManager;
    class CarrierSystem;
    class TransportJobManager;
    class CargoManager;
    class DemandManager;
    class FlagManager;
    class RoadManager;
    class EntityManager;
    class Flag;
}

namespace World {

class TransportSystem : public Core::EventListener {
public:
    TransportSystem();
    ~TransportSystem();

    // External manager mode: accept already-created managers
    void SetExternalManagers(
        CarrierManager* carriers,
        CarrierSystem* carrierSystem,
        TransportJobManager* transportJobs,
        CargoManager* cargo,
        DemandManager* demand,
        FlagManager* flagManager,
        RoadManager* roadManager);

    void Initialize(
        EntityManager* entityManager,
        FlagManager* flagManager,
        RoadManager* roadManager,
        Core::EventBus* eventBus);

    void Update(float dt);

    void SetWarehouseFlag(Flag* flag);
    void SyncCarriersForFlag(Flag* flag);

    CarrierManager* GetCarrierManager() { return m_carrierManager; }
    CarrierSystem* GetCarrierSystem() { return m_carrierSystem; }
    TransportJobManager* GetJobManager() { return m_transportJobManager; }
    CargoManager* GetCargoManager() { return m_cargoManager; }
    DemandManager* GetDemandManager() { return m_demandManager; }
    FlagManager* GetFlagManager() { return m_flagManager; }

    bool HasCargoManager() const { return m_cargoManager != NULL; }
    bool HasDemandManager() const { return m_demandManager != NULL; }

    int GetCarrierCount() const;

    virtual void OnEvent(Core::EventType type, void* data);

private:
    CarrierManager* m_carrierManager;
    CarrierSystem* m_carrierSystem;
    TransportJobManager* m_transportJobManager;
    CargoManager* m_cargoManager;
    DemandManager* m_demandManager;
    FlagManager* m_flagManager;
    RoadManager* m_roadManager;
    Core::EventBus* m_eventBus;

    bool m_ownsManagers;
    bool m_externalMode;
    bool m_initialized;
};

} // namespace World
