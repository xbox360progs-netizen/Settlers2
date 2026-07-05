#pragma once
#include <stdint.h>
#include "WorkerTypes.h"

namespace World {

    typedef uint8_t JobId;
    static const JobId kInvalidJobId = 0xFF;

    enum JobState {
        JobState_Waiting = 0,
        JobState_Assigned,
        JobState_Completed
    };

    enum JobType {
        JobType_None = 0,
        JobType_Construction,
        JobType_Production
    };

    static const uint16_t kDefaultJobDuration = 10;

    struct Job {
        JobId id;
        JobType type;
        JobState state;
        uint16_t targetFlag;
        uint8_t buildingIndex;
        WorkerId worker;
        uint16_t duration;

        Job()
            : id(0)
            , type(JobType_None)
            , state(JobState_Waiting)
            , targetFlag(0)
            , buildingIndex(0)
            , worker(kInvalidWorkerId)
            , duration(kDefaultJobDuration)
        {
        }
    };

    enum JobEventType {
        JET_Completed = 0
    };

    static const int kMaxJobEvents = 64;

    struct JobEvent {
        JobEventType type;
        JobId jobId;
        JobType jobType;
        WorkerId worker;

        JobEvent()
            : type(JET_Completed)
            , jobId(kInvalidJobId)
            , jobType(JobType_None)
            , worker(kInvalidWorkerId)
        {
        }
    };

}
