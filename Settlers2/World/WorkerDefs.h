#pragma once
#include <stdint.h>

#define MAX_TRANSIT_WORKERS 256
#define MAX_WORKER_ROUTE_FLAGS 64

// Cold route data — stored in parallel array inside WorkerManager
// Only accessed when worker is actively moving; not in hot iteration path
namespace World { class Flag; }

struct WorkerRoute {
    World::Flag* flags[MAX_WORKER_ROUTE_FLAGS];
};

namespace World {

enum WorkerState : uint8_t {
    WorkerState_Idle,
    WorkerState_MovingToJob,
    WorkerState_WorkingAnimation,
    WorkerState_ReturningToHome
};

enum ProfessionType : uint8_t {
    Profession_Transit,
    Profession_Woodcutter,
    Profession_Forester,
    Profession_Fisher,
    Profession_Hunter,
    Profession_Stonemason,
    Profession_Miner,
    Profession_Builder
};

}