#pragma once
#include <stdint.h>
#include <stddef.h>
#include "../Systems/ISimulationSystem.h"
#include "../Transport/TransportTypes.h"

namespace World {

    class TransportController;
    struct Cargo;
    class Carrier;

    class SimpleTransportDriver : public ISimulationSystem {
    public:
        SimpleTransportDriver(TransportController& controller);
        ~SimpleTransportDriver();

    virtual void Tick(WorldModel& world);

    int GetCargoCount() const { return m_cargoCount; }
    const Cargo* GetCargoAt(int index) const {
        if (index < 0 || index >= m_cargoCount) return NULL;
        return m_cargoPool[index];
    }

private:
        SimpleTransportDriver(const SimpleTransportDriver&);
        void operator=(const SimpleTransportDriver&);

        void DriveOne(FlagId sourceFlag);
        bool IsCargoConsumed(Cargo* cargo) const;

        TransportController& m_controller;

        static const int kMaxFlags = 256;
        static const int kMaxCargo = 64;

        Cargo* m_cargoPool[kMaxCargo];
        int m_cargoCount;
        uint32_t m_nextCargoId;
        int m_activeCarrierCount;
    };

} // namespace World
