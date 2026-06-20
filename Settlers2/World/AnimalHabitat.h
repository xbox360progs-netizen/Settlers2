#pragma once

#include "../Core/Vector2i.h"
#include "AnimalTypes.h"

namespace World
{
struct AnimalHabitat
{
    uint32_t id;
    Vector2i center;
    int radius;
    AnimalType type;
    int maxAnimals;
    int currentCount;

    AnimalHabitat()
        : id(0)
        , radius(8)
        , type(AnimalType_Deer)
        , maxAnimals(5)
        , currentCount(0)
    {
        center.x = 0;
        center.y = 0;
    }
};
}
