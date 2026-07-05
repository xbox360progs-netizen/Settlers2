#pragma once
#include "../Core/ResourceTypes.h"
#include "../Definitions/ProductionDefinition.h"

namespace World {

    // Renewable resources are produced directly by the world, not by buildings
    inline bool IsRenewable(ResourceType r)
    {
        switch (r) {
            case ResourceType_Wood:
            case ResourceType_Fish:
            case ResourceType_Meat:
            case ResourceType_Wheat:
            case ResourceType_Water:
                return true;
            default:
                return false;
        }
    }

    inline bool IsNoneOrZero(const ResourceAmount& ra)
    {
        return ra.resource == ResourceType_None || ra.amount <= 0;
    }

} // namespace World