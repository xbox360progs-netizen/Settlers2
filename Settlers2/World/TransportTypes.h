#pragma once
#include <stdint.h>

// Phase 7 — Data model types. No logic.

namespace World {

    typedef uint32_t TransportTaskId;
    typedef uint32_t FlagId;

    enum TransportTaskState {
        TTS_Created,
        TTS_Blocked,
        TTS_WaitingAtSource,
        TTS_Assigned,
        TTS_Moving,
        TTS_ArrivedAtHop,
        TTS_Cancelled,
        TTS_Delivered
    };

    enum TransportTaskReason {
        TTR_Construction,
        TTR_Production,
        TTR_Food,
        TTR_WarehouseBalance,
        TTR_Military,
        TTR_Emergency
    };

} // namespace World
