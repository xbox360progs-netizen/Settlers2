#pragma once
#include <stdint.h>

// Phase 7 — Data model types. No logic.
// Definitions skipped when SimulationCore's canonical version is already included.
#ifndef SIMCORE_TRANSPORT_TYPES_H_
#define SIMCORE_TRANSPORT_TYPES_H_

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

    // Phase 7.4 — Priority dispatching
    enum TransportBasePriority {
        TBP_Low = 0,
        TBP_Normal = 100,
        TBP_High = 200,
        TBP_Critical = 300
    };

    // Convert reason to base priority (set once at creation, immutable)
    inline uint16_t PriorityForReason(TransportTaskReason reason) {
        switch (reason) {
            case TTR_Emergency:        return TBP_Critical;
            case TTR_Food:             return TBP_High;
            case TTR_Military:         return TBP_High;
            case TTR_Construction:     return TBP_Normal;
            case TTR_Production:       return TBP_Normal;
            case TTR_WarehouseBalance: return TBP_Low;
            default:                   return TBP_Normal;
        }
    }

} // namespace World

#endif // SIMCORE_TRANSPORT_TYPES_H_
