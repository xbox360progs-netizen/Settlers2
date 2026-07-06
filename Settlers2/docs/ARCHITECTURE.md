# Architecture Baseline — Milestone 3 Complete (2026-07-06)

## Three-Layer Architecture

```
    ┌─────────────────────────────────────────────────────┐
    │                  Domain Systems                      │
    │  Production  │  Construction  │  Settlement AI       │
    │  Warehouse   │  Economy       │  Worker              │
    │  Consumption │  Renewable     │  ...                 │
    └──────────────┴────────────────┴──────────────────────┘
                           │
            ┌──────────────┴──────────────┐
            ▼                              ▼
    ┌─────────────────┐          ┌─────────────────────┐
    │  DemandManager   │          │     JobManager       │
    │  (Demand Owner)  │          │  (Worker Assignment) │
    └────────┬─────────┘          └──────────────────────┘
             │
    ┌────────▼──────────────────────────────────────────┐
    │              Transport Layer                       │
    │  ┌───────────────────┐   ┌──────────────────────┐ │
    │  │ DemandManager      │   │ LocalTransferSystem  │ │
    │  │   SetDemand/       │   │   Tick: evaluate     │ │
    │  │   CompleteDemand   │   │   deficit → pending  │ │
    │  └────────┬──────────┘   └───────────┬──────────┘ │
    │           │ pendingDemand[]           │            │
    │           ▼                           ▼            │
    │  ┌──────────────────────────────────────────────┐  │
    │  │          TransportController                  │  │
    │  │   CreateTask │ Dispatcher │ Carrier Pool      │  │
    │  │   RoadGraph  │ Tick() pipeline                │  │
    │  └──────────────────────┬───────────────────────┘  │
    │                         │                          │
    │  ┌──────────────────────▼───────────────────────┐  │
    │  │                 Carrier                       │  │
    │  │    Pickup → Travelling → Delivering → Idle   │  │
    │  └──────────────────────────────────────────────┘  │
    │                                                     │
    │  TransportNodes (passive buffer + attachment reg.) │
    └─────────────────────────────────────────────────────┘
```

## Milestone 3 — Feature Freeze

**Transport subsystem is architecturally complete.** Any functional change requires
a failing integration or soak test demonstrating necessity.

### Frozen contracts

| Component | Responsibility | May Not |
|-----------|---------------|---------|
| `DemandManager` | Demand lifecycle; exclusive publisher of `TransportRequest[]` | Create tasks, evaluate deficits, observe buildings |
| `LocalTransferSystem` | Deficit evaluation → `pendingDemand[]` | Create/complete demands, touch TransportController |
| `TransportController` | Task creation, dispatch, carrier lifecycle, delivery | Create demand, evaluate deficits, observe buildings |
| `Carrier` | Pickup, travel hops, delivery; spatial execution only | Choose destination, evaluate deficits, interpret reason |
| `Dispatcher` | Score-based selection (`priority + age`, FIFO tiebreak) | Inspect reason, owner, domain origin |
| `TransportNode` | Passive storage (`ResourceBuffer`), attachment registry | Evaluate deficits, make routing decisions |
| `RoadGraph` | BFS pathfinding, adjacency matrix, `AddEdge`/`RemoveEdge` | Observe domain state, evaluate deficits |

### Key invariants

```
1. TransportController never calls SetDemand.
2. TransportController never reads pendingDemand.
3. TransportController never modifies outputBuffer.
4. TransportController never inspects building state.
5. TransportController never chooses which demand to fulfill.
6. pendingDemand is a cache (TransportNode owns). Demand = DemandManager owns.
7. One active task per demand (activeTask guard).
8. No reservation — carrier checks buffer atomically at pickup.
9. Pickup decrements before delivery increments (conservation: Σbuffer + Σcargo = constant).
10. CompleteDemand is called by TransportController on successful delivery.
11. ReleaseCarrierForTask is the single cleanup point for all carriers.
12. Carrier addressable only through TransportCarrier pool — Phase 7 Carrier cleaned up
    by ReleaseCarrierForTask (single path).
```

### Test baseline

- 212/212 unit tests pass
- 10 RoadGraph tests (BFS, multi-hop, bidirectional, no-path)
- 13 dispatch tests (priority, FIFO, cancellation, carrier pool)
- 11 carrier execution tests (full lifecycle, multi-hop, conservation, demand completion)
- 12 DemandManager tests (lifecycle, dedup, activeTask, pendingDemand)
- 20 LocalTransfer tests
- 22 TransportNode tests
- All legacy integration scenarios (T1–T50) pass without regression

### Dependency cleanup

| File | Status |
|------|--------|
| `Scene/Presentation/Migration/` | `CarrierView.h`, `ICarrierSource.h`, `LegacyCarrierSource.h/.cpp` added |
| `Scene/Transport/CarrierPresentation.h/.cpp` | Uses `ICarrierSource*` — zero World/ includes |
| `World/TransportTypes.h` | Forwarding header → `SimulationCore/Transport/TransportTypes.h` |
| `World/TransportRoute.h` | Forwarding header → `SimulationCore/Transport/TransportRoute.h` |
| `#include.*World/` in Scene/Presentation | Zero matches verified |
| `#include.*(windows.h|xtl.h|xbox)` in SimulationCore | Zero matches verified |

## Milestone 4 — Carrier Visualization (planned)

### Hypothesis

Carrier is currently a purely logical object (`TransportCarrier` pool: state, taskId,
cargoType, cargoAmount — no spatial data). Visualization reads from the legacy
Phase 7 `World::Carrier` which has position data (transitTiles, road, ep, walkDir, cargo).

Milestone 4 will introduce the first **position-computing model** for the new pipeline:

```
TransportCarrier (logical)  →  CarrierPosition (spatial)  →  CarrierView (visual DTO)
```

### Design — separate position model

`TransportCarrier` remains pure logical state — **no spatial fields added**:

```cpp
struct TransportCarrier {
    TransportCarrierState state;
    TransportTaskId      taskId;
    ResourceType         cargoType;
    uint16_t             cargoAmount;
};
```

Position is a separate model, computed by a **Position Computer**:

```cpp
struct CarrierPosition {
    CarrierId  carrier;
    FlagId     routeFlags[kMaxRouteLength];
    uint8_t    routeCount;
    uint8_t    currentHop;
    float      progress;        // 0.0 = current hop source, 1.0 = current hop destination
    bool       carrying;
    ResourceType cargoType;
};
```

### Separation of concerns

```
TransportCarrier          CarrierPosition          CarrierView
    owns task              owns spatial             owns visual identity
    owns cargo             interpolation             (read by renderer)
    owns state
           │                      │
           │   Position Computer  │
           └──── reads ─────► computes
                                 │
                          ┌──────┴──────┐
                          │             │
                   ICarrierSource  CarrierView
                   (Scene/Migration)  (DTO)
```

### Benefits of separate model

| Concern | Lives in | Testable without |
|---------|----------|-----------------|
| Carrier state machine | `TransportCarrier` | coordinates, RoadGraph |
| Route computation | `RoadGraph` | TransportCarrier, renderer |
| Position interpolation | `CarrierPosition` | DemandManager, Economy |
| Visual representation | `CarrierView` + `WorkerPass` | any simulation logic |

### Key invariant — visual state is derived state

```
Renderer never modifies Carrier.
PositionModel never modifies Demand, TransportController, or TransportNode.
No simulation logic depends on coordinates.
```

This means:
- `CarrierPosition` is **read-only** from the simulation's perspective — computed from `TransportCarrier.taskId` → `TransportTask.route` + `hopIndex` at render time
- `TransportCarrier` state transitions (Pickup→Travelling→Delivering) remain driven by `TransportController::Tick()` — position interpolation is layered on top
- If `CarrierPosition` is removed or broken, the simulation still runs correctly — only visuals degrade

### Boundary

| In scope | Out of scope |
|----------|-------------|
| `CarrierPosition` struct + position computer | Changes to `TransportController`, `DemandManager`, `LTS`, `ProductionSystem` |
| Route→world interpolation (hop flags → tile path → screen coords) | Changes to `TransportCarrier` pool structure or lifespan |
| `SimulationCoreCarrierSource` (reads `TransportCarrier` + computed position → `CarrierView`) | Changes to carrier state machine or lifecycle |
| `CarrierView` extended with spatial fields | Animated sprites (already exist in `WorkerPass`) |
| Remove `LegacyCarrierSource` after parity verified | Debug overlay (separate, parallel workstream) |

Transport logic remains frozen. Milestone 4 adds ONLY spatial data computed on top of the existing logical carrier state machine.

## Milestone 1 — Economy Core Freeze (verified ✅)

See `ECONOMY_ARCHITECTURE.md`.

## Milestone 2 — Local Transport Foundation (verified ✅)

See `TRANSPORTNODE_CONTRACT.md` and `AGENTS.md` (Phase 2 section).
