#pragma once

namespace Logic {
    class EconomyManager;
}

namespace World {
    class Building;
    class Carrier;
    class CarrierManager;
    class Flag;
    class FlagManager;
    struct Road;
    class RoadManager;
    class TransportJobManager;

    bool CanDestroyFlag(Flag* flag, CarrierManager* cm, TransportJobManager* jm, RoadManager* rm);
    bool CanDestroyRoad(Road* road, CarrierManager* cm, TransportJobManager* jm);
    bool CanDestroyCarrier(Carrier* carrier, TransportJobManager* jm);
    bool CanDestroyBuilding(Building* building, Logic::EconomyManager* em);
}
