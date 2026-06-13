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

        AnimalHabitat()
            : id(0)
            , radius(8)
            , type(AnimalType_Deer)
            , maxAnimals(5)
        {
            center.x = 0;
            center.y = 0;
        }
    };
}
