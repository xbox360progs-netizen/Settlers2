#pragma once

namespace World
{
    enum AnimalType
    {
        AnimalType_Deer = 0,
        AnimalType_Rabbit,
        AnimalType_Crocodile,
        AnimalType_Snake,

        AnimalType_Count
    };

    inline const char* AnimalTypeToIconName(AnimalType type)
    {
        switch (type) {
            case AnimalType_Deer:      return "r_deer";
            case AnimalType_Rabbit:    return "r_rabbit";
            case AnimalType_Crocodile: return "r_crocodile";
            case AnimalType_Snake:     return "r_snake";
            default:                   return "";
        }
    }

    // 4 diagonal directions: 0=NE, 1=SE, 2=NW, 3=SW
    inline int VelocityToDirIndex(float vx, float vy)
    {
        if (vy < 0.0f)
            return (vx >= 0.0f) ? 0 : 2;
        else
            return (vx > 0.0f) ? 1 : 3;
    }

    inline int AnimalDirSpriteCount()
    {
        return 4;
    }
}
