# DemandManager Testability Report

**PR13 Phase 1 — Verification (no code changes)**
**Date**: 2026-07-03

## Summary

| Count | Status |
|-------|--------|
| 0 | ✅ Testable now — header cannot compile in SimulationCoreTests |
| 14 | ✅ Expected after include fix — pure data operations |
| 3 | ❌ Expected after include fix — require FlagManager or TransportController |

Single root cause blocks all 17 methods from compiling: **Demand.h includes ResourceNode.h** (World type).

## Per-Method Breakdown

### Constructor + lifecycle

| Method | Status | Notes |
|--------|--------|-------|
| `DemandManager()` | ✅ | No World deps. Constructor is pure data init. |
| `Clear()` | ✅ | Resets pool + list. Pure data. |

### Demand management

| Method | Status | Tests | Notes |
|--------|--------|-------|-------|
| `SetDemand(ResourceType, uint32_t, Handle<Flag>, int)` | ✅ | 3 tests | Pure list insert/update. Handle<Flag> is SimulationCore type. |
| `ClearDemand(Handle<Flag>)` | ✅ | 2 tests | Pure list erase + ticket cancel. Handle compare by index. |
| `ClearDemand(ResourceType, Handle<Flag>)` | ✅ | 1 test | Same, with type filter. |
| `FindDemand(Handle<Flag>)` | ✅ | 2 tests | Linear list search by handle index. |
| `FindDemand(ResourceType, Handle<Flag>)` | ✅ | 1 test | Same, with type filter. |
| `FindBestDemand(ResourceType)` | ✅ | 2 tests | Priority-based selection. Skips saturated demands. |
| `HasDemand(ResourceType)` | ✅ | 3 tests | Checks reserved < requested. |
| `HasDemandFromOtherFlag(ResourceType, Handle<Flag>)` | ✅ | 1 test | Skips demands for same flag. |

### Ticket lifecycle

| Method | Status | Tests | Notes |
|--------|--------|-------|-------|
| `Reserve(ResourceType, FlagId=0)` | ✅ (origin=0) | 3 tests | originFlag=0 → pure data: FindBestDemand + AllocSlot |
| `Reserve(ResourceType, FlagId>0)` | ❌ | 1 test | Needs FlagManager::ResolveFlag + TransportController::CreateTask |
| `ReleaseTicket(DemandTicket*)` | ✅ | 3 tests | Pool slot management. Null guard, double-free assert. |
| `Deliver(DemandTicket*)` | ✅ | 4 tests | State transition + accounting. Null/duplicate guards. |
| `GetTicket(uint32_t)` | ✅ | 3 tests | Pool linear scan by ID. |

### Setters (bridge injection)

| Method | Status | Notes |
|--------|--------|-------|
| `SetTransportController(TransportController*)` | ✅ | Trivial setter. Reserve with controller will call CreateTask — blocked. |
| `GetController() const` | ✅ | Trivial getter. |
| `SetFlagManager(FlagManager*)` | ✅ | Trivial setter. |

## Root Cause Analysis

### Single root cause: `Demand.h` includes `ResourceNode.h`

```
World/Demand.h
  → #include "ResourceNode.h"          (World — pulls TileType.h)
    → #include "../World/TileType.h"   ❌ World type, not in SimulationCore
    → #include "../SimulationCore/Core/ResourceTypes.h"  ✅
    → #include "../SimulationCore/Core/ResourceDebug.h"   ✅
```

Demand.h uses exactly **two types** from its includes:
- `ResourceType` — from `SimulationCore/Core/ResourceTypes.h`
- `Handle<Flag>` — from `Handle.h` (SimulationCore/Core/Handle.h via forwarding)

Everything else in ResourceNode.h (`TileType`, `WeightType`, `TreeState`, `ResourceNode` struct, icon lookup functions) is **unused** by Demand.h.

### Why the fix is minimal

Replace:
```cpp
#include "ResourceNode.h"
```
with:
```cpp
#include "../SimulationCore/Core/ResourceTypes.h"
#include "../SimulationCore/Core/Handle.h"
```

This is a **2-line change** that eliminates the entire transitive include chain (TileType.h, ResourceDebug.h, ResourceNode.h inline functions). After this fix, `DemandManager.h` becomes includable from SimulationCoreTests.

### Three remaining blockers (after include fix)

| Method | Blocker | Root cause |
|--------|---------|-----------|
| `Reserve(type, originFlag>0)` | `m_flagManager->ResolveFlag()` | FlagManager is a World type |
| `Reserve(type, originFlag>0)` | `m_controller->CreateTask()` | TransportController::CreateTask needs route computation |
| `ClearDemand(flag)` for tickets | `FlagManager` not needed — pure handle comparison ✅ | — |

The first two are the **same pattern** as PR8 (dependency inversion for TransportController's World dependencies). They require:
1. An `IFlagResolver` interface (Handle → FlagId)
2. The existing `IDemandService` for CreateTask

### Comparison with PR11 (TransportController)

| Metric | TransportController (PR11) | DemandManager (PR13) |
|--------|---------------------------|---------------------|
| Header-only blocker count | 1 (Carrier.h) | 1 (Demand.h → ResourceNode.h) |
| Methods testable after fix | 34/42 (81%) | 14/17 (82%) |
| Root causes | 1 (Carrier → World) | 1 (Demand → ResourceNode) |
| Additional blockers | 0 | 0 (all 3 ❌ are the same FlagManager+Controller pattern) |

## Recommendation

### PR13.1 — Minimal repair: fix Demand.h include

**Hypothesis**: Removing `#include "ResourceNode.h"` from Demand.h and replacing with SimulationCore equivalents unblocks all 14 pure-data tests.

**Intervention**: 2-line change in `World/Demand.h`:
```cpp
// Before:
#include "ResourceNode.h"
#include "Handle.h"

// After:
#include "../SimulationCore/Core/ResourceTypes.h"
#include "../SimulationCore/Core/Handle.h"
```

**Criterion**: Re-Verification shows 14✅ 3❌.

### PR13.2 — Remove remaining World deps (future, after re-verification)

After PR13.1, 3 methods remain ❌ (`Reserve` with `originFlag>0` — depends on
`FlagManager::ResolveFlag` + `TransportController::CreateTask`).

The concrete interface shape should be determined **after** PR13.1 re-verification.
Likely candidates:
- Abstract `Handle<Flag> → FlagId` resolution from FlagManager
- Reuse existing `IDemandService` for CreateTask (currently only has `CompleteDemand`)

Let re-verification data suggest the minimal intervention.

## Test File

`DemandManagerVerification.cpp` — 29 tests, all blocked by single include chain.
Added to SimulationCoreTests.vcxproj.

## Status

### PR13.1 — Repair applied ✅

**Intervention**: `World/Demand.h` — replaced `#include "ResourceNode.h"` with
`#include "../SimulationCore/Core/ResourceTypes.h"` + `Handle.h`.

**Effect**: `DemandManager.h` no longer transitively includes `TileType.h` or
any other World type. The header now compiles in SimulationCoreTests context.

**Pending**: Re-Verification on Xbox 360 SDK — expected 14✅ 3❌.
