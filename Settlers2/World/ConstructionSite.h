#pragma once
#include "Components/Building.h"

#define MAX_BUILDER_FLAGS 64

namespace World {

    class Flag;
    struct Road;

    enum BuilderState {
        Builder_None,
        Builder_Walking,
        Builder_Building,
        Builder_Returning
    };

    typedef unsigned int ConstructionSiteId;

    class ConstructionSite {
    public:
        static ConstructionSiteId GenerateId();

        ConstructionSiteId id;
        int x;
        int y;
        BuildingType buildingType;
        Flag* flag;

        int woodNeeded;
        int stoneNeeded;
        int woodDelivered;
        int stoneDelivered;
        int lastWoodRequested;
        int lastStoneRequested;
        int woodRequested;
        int stoneRequested;
        float buildProgress;

        BuilderState builderState;
        Flag* builderRoute[MAX_BUILDER_FLAGS];
        uint32_t builderRouteCount;
        uint32_t builderRouteIndex;
        float builderEp;
        float builderWalkDir;

        ConstructionSite(int x, int y, BuildingType type, Flag* flag);
        ~ConstructionSite() {}

        bool NeedsWood() const { return woodDelivered < woodNeeded; }
        bool NeedsStone() const { return stoneDelivered < stoneNeeded; }
        bool NeedsResources() const { return NeedsWood() || NeedsStone(); }
        int WoodMissing() const { return woodNeeded - woodDelivered; }
        int StoneMissing() const { return stoneNeeded - stoneDelivered; }
        bool CanBuild() const { return woodDelivered >= woodNeeded && stoneDelivered >= stoneNeeded; }
        bool IsComplete() const { return buildProgress >= 100.0f; }
    };

}
