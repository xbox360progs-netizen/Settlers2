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

} // namespace World
