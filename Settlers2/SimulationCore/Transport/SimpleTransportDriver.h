#pragma once
#include <stdint.h>
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
