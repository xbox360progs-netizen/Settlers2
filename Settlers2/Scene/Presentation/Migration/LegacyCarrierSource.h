#pragma once
#include "ICarrierSource.h"

// Temporary — wraps legacy World::CarrierManager for migration.
// Remove when SimulationCore TransportCarrier publishes position data.

namespace World {
    class CarrierManager;
}

namespace Scene {

class LegacyCarrierSource : public ICarrierSource {
public:
    LegacyCarrierSource();

    void SetCarrierManager(World::CarrierManager* carrierManager);

    virtual uint32_t GetCarrierCount() const;
    virtual bool GetCarrier(uint32_t index, CarrierView& out) const;

private:
    World::CarrierManager* m_carrierManager;
};

} // namespace Scene
