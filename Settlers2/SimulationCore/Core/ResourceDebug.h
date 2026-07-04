#pragma once
#include "../Definitions/ResourceDefinition.h"

namespace World {

inline const char* ResourceTypeToString(ResourceType type)
{
    return GetResourceDefinition(type).name;
}

}
