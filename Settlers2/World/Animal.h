#pragma once
#include "AnimalTypes.h"

namespace World {
    enum AnimalState {
        AnimalState_Alive,
        AnimalState_Trapped
    };

    struct Animal {
        AnimalType type;
        AnimalState state;
        float x, y;
        int spawnerX, spawnerY;
        float vx, vy;
        float stopTimer;
    };
} // namespace World
