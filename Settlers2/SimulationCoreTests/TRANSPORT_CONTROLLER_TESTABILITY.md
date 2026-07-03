# TransportController Testability Report

**PR11 Phase 1 — Verification (no code changes)**
**Date**: 2026-07-03

## Summary

| Count | PR11 (Verified) | PR12 (Complete) | Re-Verification (Expected) |
|-------|----------------|-----------------|---------------------------|
| 34 | ✅ Testable | ✅ Still testable | ✅ |
| 8 | ❌ Blocked | ✅ Resolved (PR12) | ✅ Expected: 0 |

## Per-Method Breakdown

### Constructor + query API

| Method | Status | Tests |
|--------|--------|-------|
| `TransportController(IRoadGraph&, IFlagInventory&, ICargoRepository&, IDemandService&)` | ✅ | Constructor with 4 inline stubs works. Stubs are 3-5 lines each, use only SimulationCore types. |
| `~TransportController()` | ✅ | No special behavior (empty destructor). |
| `GetActiveTaskCount()` | ✅ | 4 tests: initial (0), after CreateTask, after pool exhaustion, edge cases. |
| `GetWaitingCount(FlagId)` | ✅ | 3 tests: independent per-flag queues, empty flag, after CancelTask. |
| `PeekWaitingTask(FlagId)` | ✅ | 2 tests: returns first task, empty queue returns NULL. |
| `GetBlockedCount()` | ✅ | 2 tests: single blocked task, multiple blocked tasks. |
| `GetTaskById(TransportTaskId)` | ✅ | 3 tests: valid ID, invalid ID, after CancelTask. |
| `FindTask(TransportTaskId)` | ✅ | Same as GetTaskById (inline, identical logic). |
| `Update(float)` | ✅ | 1 test: no crash, tick increments. |

### Lifecycle

| Method | Status | Tests |
|--------|--------|-------|
| `CreateTask(ResourceType, FlagId, FlagId, TransportTaskReason)` | ✅ | 10 tests: valid route, blocked route, same origin/dest, ResourceType_None, pool exhaustion, priority by reason, sequential IDs, independent queues. |
| `CancelTask(TransportTaskId)` | ✅ | 5 tests on WaitingAtSource: state → Cancelled, queue cleared. 1 test on Blocked: state → Cancelled. 2 tests: non-existent ID (no-op), double cancel (no-op). |

### Carrier-dependent

| Method | Status | Blocker |
|--------|--------|---------|
| `NotifyCarrierIdle(void*, FlagId)` | ❌ | Carrier.h includes World headers: `Flag.h`, `FlagManager.h`, `Road.h`, `RoadManager.h`, `Entity.h`. Cannot compile in SimulationCoreTests. |
| `NotifyCarrierPickedUp(void*, void*)` | ❌ | Same Carrier dependency. Also needs Cargo (which is in SimulationCore but requires Carrier context). |
| `NotifyCarrierArrived(void*, FlagId)` | ❌ | Same Carrier dependency. Also needs full TransportController state (Assigned→Moving→Arrived). |
| `NotifyCarrierDropped(void*, FlagId)` | ❌ | Same Carrier dependency. Currently a no-op in implementation. |
| `CancelTask(Assigned)` | ❌ | Cancelling an Assigned task needs carrier release logic — requires Carrier. |
| `CancelTask(Moving)` | ❌ | Cancelling a Moving task needs carrier release + re-enqueue — requires Carrier. |

### Recovery

| Method | Status | Tests |
|--------|--------|-------|
| `NotifyRoadNetworkChanged()` | ✅ | 2 tests: retries blocked tasks with road now available, no-op when no blocked tasks. IRoadGraph stub flips `routeFound` to trigger retry. |
| `NotifyFlagRemoved(FlagId)` | ✅ | 3 tests: removes WaitingAtSource task from queue, no-op on Blocked task, no-op on non-existent flag. |

**FlagRemoved on Assigned/Moving tasks**: ❌ — blocked by Carrier dependency.

## Root Cause Analysis for ❌

### Blocker: `Carrier.h` includes World headers

```
SimulationCore/Transport/Carrier.h (PR11)
  → #include "../../World/Flag.h"
  → #include "../../World/FlagManager.h"
  → #include "../../World/Road.h"
  → #include "../../World/RoadManager.h"
  → #include "../../World/Entity.h"
  → pulls in <xtl.h>, DirectX math, platform APIs
```

This was the single point of failure for 8 blocked test categories.

### PR12 Fix

PR12 resolved this by:
1. **Removing all World `#include` directives** from `SimulationCore/Transport/Carrier.h`
2. **Replacing with forward declarations** for `Road`, `Flag`, `FlagManager`, `RoadManager`, `DemandManager`, `CargoManager`, `TransportController`
3. **Moving 5 method bodies** that dereference World types to `World/Carrier.cpp`:
   - `GetPathLen()`, `GetCenterEp()`, `GetFlagEp(Flag*)`, `Update(float)`, `UpdateWalkingToPost(float)`
4. The Phase 7 fields (`m_phase7Task`, `m_phase7TargetFlag`, `m_phase7Cargo`, `m_phase7Controller`) stay inline — they use only SimulationCore types.
5. `Vector2i.h` canonical copy at `SimulationCore/Core/Vector2i.h`; original `Core/Vector2i.h` → forwarding header.

**Expected**: all 8 ❌ now compile in SimulationCoreTests (42✅ 0❌) — pending verification on Xbox 360 SDK.

### Remaining: `void*` abstraction is opaque

```
NotifyCarrierIdle(void* carrier, FlagId atFlag)
```

The `void*` parameter hides the dependency from the interface signature but
not from the implementation — TransportController.cpp still does
`static_cast<Carrier*>(carrier)`. This remains as-is for now (deferred).

## Future Recommendation

To fully decouple TransportController from Carrier:: spatial fields, extract
an `ICarrierHandle` interface that exposes only the task/target fields:

```cpp
struct ICarrierHandle {
    virtual TransportTask* GetTask() = 0;
    virtual FlagId GetTargetFlag() = 0;
    virtual void AssignTask(TransportTask*, FlagId) = 0;
    virtual void ClearTask() = 0;
};
```

## Test File

`TransportControllerVerification.cpp` — 34 tests, 0 World dependencies.
Added to SimulationCoreTests.vcxproj.

## Next Step

Phase 2 — Re-verify: regenerate all 42 tests, build, run on Xbox 360 SDK.
Expected: 42✅ 0❌. After confirmation, replace "Expected" with "Verified" across all tables.
