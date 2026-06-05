#pragma once
#include "Components/Building.h"

namespace World {

    class Flag;

    class ConstructionSite {
    public:
        int x;
        int y;
        BuildingType buildingType;
        Flag* flag;

        int woodNeeded;
        int stoneNeeded;
        int woodDelivered;
        int stoneDelivered;
        int woodRequested;
        int stoneRequested;
        float progress;

        ConstructionSite(int x, int y, BuildingType type, Flag* flag);
        ~ConstructionSite() {}

        bool NeedsWood() const { return woodDelivered < woodNeeded; }
        bool NeedsStone() const { return stoneDelivered < stoneNeeded; }
        bool NeedsResources() const { return NeedsWood() || NeedsStone(); }
        int WoodMissing() const { return woodNeeded - woodDelivered - woodRequested; }
        int StoneMissing() const { return stoneNeeded - stoneDelivered - stoneRequested; }
        bool CanBuild() const { return woodDelivered >= woodNeeded && stoneDelivered >= stoneNeeded; }
        bool IsComplete() const { return progress >= 100.0f; }
    };

}
