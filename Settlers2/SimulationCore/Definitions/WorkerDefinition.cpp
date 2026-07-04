#include "WorkerDefinition.h"

namespace World {

    static const int kDefaultSpeed = 60;
    static const int kDefaultCarry = 1;

    static const WorkerDefinition g_workers[] = {
        { WT_None,          "None",        kDefaultSpeed, 0 },
        { WT_Woodcutter,    "Woodcutter",  kDefaultSpeed, kDefaultCarry },
        { WT_Forester,      "Forester",    kDefaultSpeed, kDefaultCarry },
        { WT_SawmillWorker, "SawmillWorker", kDefaultSpeed, kDefaultCarry },
        { WT_Stonemason,    "Stonemason",  kDefaultSpeed, kDefaultCarry },
        { WT_Fisher,        "Fisher",      kDefaultSpeed, kDefaultCarry },
        { WT_Hunter,        "Hunter",      kDefaultSpeed, kDefaultCarry },
        { WT_Farmer,        "Farmer",      kDefaultSpeed, kDefaultCarry },
        { WT_Miller,        "Miller",      kDefaultSpeed, kDefaultCarry },
        { WT_Baker,         "Baker",       kDefaultSpeed, kDefaultCarry },
        { WT_CoalMiner,     "CoalMiner",   kDefaultSpeed, kDefaultCarry },
        { WT_IronMiner,     "IronMiner",   kDefaultSpeed, kDefaultCarry },
        { WT_IronSmelter,   "IronSmelter", kDefaultSpeed, kDefaultCarry },
        { WT_Toolmaker,     "Toolmaker",   kDefaultSpeed, kDefaultCarry },
        { WT_Storekeeper,   "Storekeeper", kDefaultSpeed, kDefaultCarry },
        { WT_WellWorker,    "WellWorker",  kDefaultSpeed, kDefaultCarry },
        { WT_Builder,       "Builder",     kDefaultSpeed, kDefaultCarry },
        { WT_Transit,       "Transit",     kDefaultSpeed, kDefaultCarry },
    };

    const WorkerDefinition& GetWorkerDefinition(WorkerType type)
    {
        static const int kCount = sizeof(g_workers) / sizeof(g_workers[0]);
        int index = static_cast<int>(type);
        if (index < 0 || index >= kCount)
            index = 0;
        return g_workers[index];
    }

}
