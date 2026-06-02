#pragma once
#include "Worker.h"

namespace World {

    class Builder : public Worker {
    public:
        Builder(Building* h) : Worker(h) {}

        void Build(Building* target) {
            // Logic for builder to work on target building
        }
    };
}
