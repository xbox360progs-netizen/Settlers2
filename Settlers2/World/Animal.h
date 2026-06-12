#pragma once

namespace World {

enum AnimalType {
    AnimalType_Deer,
    AnimalType_Rabbit,
    AnimalType_Crocodile,
    AnimalType_Snake
};

enum AnimalState {
    AnimalState_Alive,
    AnimalState_Trapped
};

struct Animal {
    AnimalType type;
    AnimalState state;
    int x, y;
    int spawnerX, spawnerY;
};

inline const char* AnimalTypeToIconName(AnimalType type) {
    switch (type) {
        case AnimalType_Deer:     return "r_deer";
        case AnimalType_Rabbit:   return "r_rabbit";
        case AnimalType_Crocodile: return "r_crocodile";
        case AnimalType_Snake:    return "r_snake";
        default: return "";
    }
}

} // namespace World
