#pragma once
#include "ResourceNode.h"

namespace World {
    class Building; // Forward declaration

    enum class WorkerState {
        Idle,
        Working,
        ReturningHome,
        MovingToJob
    };

    class Worker {
    public:
        WorkerState state;
        Building* home;

        Worker(Building* h) : state(WorkerState::Idle), home(h) {}
        virtual ~Worker() {}
    };
}
