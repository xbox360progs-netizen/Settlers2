#pragma once
#include "../Core/BuildingTypes.h"
#include "../Core/Vector2i.h"

namespace World {

    struct ConstructionRequest {
        BuildingType type;
        Vector2i position;
        int owner;
        int priority;
        bool fulfilled;

        ConstructionRequest()
            : type(BuildingType_None)
            , position(0, 0)
            , owner(0)
            , priority(0)
            , fulfilled(false)
        {
        }
    };

}
