#pragma once
#include "ResourceNode.h"

namespace World {
    class Building; // Forward declaration

    enum WorkerState {
        WorkerState_Idle,
        WorkerState_Working,
        WorkerState_ReturningHome,
        WorkerState_MovingToJob
    };

    class Worker {
    public:
        WorkerState state;
        Building* home;

        Worker(Building* h) : state(WorkerState_Idle), home(h) {}
        virtual ~Worker() {}
    };
}
