#pragma once
#include "../Core/WorkerTypes.h"

namespace World {

    struct WorkerDefinition {
        WorkerType type;
        const char* name;
        int walkSpeed;
        int carryCapacity;
    };

    const WorkerDefinition& GetWorkerDefinition(WorkerType type);

}
