#pragma once
#include <stdint.h>

#define MAX_TRANSIT_WORKERS 256
#define MAX_WORKER_ROUTE_FLAGS 64

// Forward declaration for cold route data
namespace World { class Flag; }

// Cold route data — stored in parallel array inside WorkerManager
// Only accessed when worker is actively moving; not in hot iteration path
struct WorkerRoute {
    World::Flag* flags[MAX_WORKER_ROUTE_FLAGS];
};

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
