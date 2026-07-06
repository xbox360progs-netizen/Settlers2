#pragma once
#include <stdint.h>

// Temporary DTO used only during World → SimulationCore migration.
// Remove after LegacyBuildingSource is deleted.
// BuildingType uses legacy World::BuildingType enum values for renderer compatibility.

namespace Scene {

enum WorkerVisualState : uint8_t {
    WVS_None = 0,
    WVS_Idle,
    WVS_WalkingToNode,
    WVS_WalkingToBuilding,
    WVS_Working
};

struct BuildingView {
    int flagX;
    int flagY;

    // 0=flag, 1=completed building, 2=construction site
    uint8_t kind;

    // Legacy BuildingType enum value (renderer-compatible)
    uint8_t buildingType;

    // Unique ID for Inspector lookup (0 = unassigned)
    uint32_t buildingId;

    // Visual state
    uint8_t fsmState;
    bool    hasWorker;
    uint8_t workerVisualState;  // WorkerVisualState enum
    bool    depleted;
    uint32_t color;

    BuildingView()
        : flagX(0)
        , flagY(0)
        , kind(0)
        , buildingType(0)
        , buildingId(0)
        , fsmState(0)
        , hasWorker(false)
        , workerVisualState(WVS_None)
        , depleted(false)
        , color(0xFFFFFFFF)
    {
    }
};

} // namespace Scene
