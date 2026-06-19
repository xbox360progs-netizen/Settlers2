#pragma once
#include <stdint.h>
#include "ResourceNode.h"

namespace World {
    class Flag;

    enum JobState {
        JobState_Pending,
        JobState_Active,
        JobState_Done
    };

    struct TransportJob {
        uint32_t id;
        ResourceType resourceType;
        Flag* sourceFlag;
        Flag* targetFlag;
        JobState state;

        TransportJob() : id(0), resourceType(ResourceType_None), sourceFlag(NULL), targetFlag(NULL), state(JobState_Pending) {}

        void Clear() {
            id = 0;
            resourceType = ResourceType_None;
            sourceFlag = NULL;
            targetFlag = NULL;
            state = JobState_Pending;
        }
    };
}
