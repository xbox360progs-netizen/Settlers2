#pragma once
#include <stdint.h>

namespace World {

    enum WorkerType {
        WT_None = 0,
        WT_Woodcutter,
        WT_Forester,
        WT_SawmillWorker,
        WT_Stonemason,
        WT_Fisher,
        WT_Hunter,
        WT_Farmer,
        WT_Miller,
        WT_Baker,
        WT_CoalMiner,
        WT_IronMiner,
        WT_IronSmelter,
        WT_Toolmaker,
        WT_Storekeeper,
        WT_WellWorker,
        WT_Builder,
        WT_Transit,
        WT_Count
    };

    typedef uint8_t WorkerId;
    static const WorkerId kInvalidWorkerId = 0xFF;

    enum WorkerState {
        WorkerState_Idle = 0,
        WorkerState_FindingJob,
        WorkerState_Assigned,
        WorkerState_Walking,
        WorkerState_Working
    };

}
