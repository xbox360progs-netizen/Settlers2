#pragma once
#include "../Core/BuildingTypes.h"
#include "../Core/Vector2i.h"
#include "../Core/ResourceTypes.h"
#include "ConstructionState.h"

namespace World {

    static const int kMaxBuildResources = 4;

    struct BuildResourceSlot {
        ResourceType resource;
        int required;
        int delivered;
        bool requested;
    };

    struct ConstructionSite {
        ConstructionState state;
        BuildingType type;
        Vector2i position;
        int owner;
        int progress;

        BuildResourceSlot resources[kMaxBuildResources];
        int resourceCount;
        bool builderAssigned;
        int requiredProgress;
        int lastStateChangeTick;

        ConstructionSite()
            : state(CS_Pending)
            , type(BuildingType_None)
            , position(0, 0)
            , owner(0)
            , progress(0)
            , resourceCount(0)
            , builderAssigned(false)
            , requiredProgress(100)
            , lastStateChangeTick(0)
        {
            for (int i = 0; i < kMaxBuildResources; ++i) {
                resources[i].resource = ResourceType_None;
                resources[i].required = 0;
                resources[i].delivered = 0;
                resources[i].requested = false;
            }
        }
    };

}
