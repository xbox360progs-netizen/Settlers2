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
    bool CanDestroyFlag(Flag* flag, CarrierManager* cm, RoadManager* rm);
    bool CanDestroyRoad(Road* road, CarrierManager* cm);
    bool CanDestroyCarrier(Carrier* carrier);
    bool CanDestroyBuilding(Building* building, Logic::EconomyManager* em);
}
