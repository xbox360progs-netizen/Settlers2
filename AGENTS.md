# Transport v1 — Complete ✅ (tag: `transport-v1-complete`)

## Final Architecture

```
Layer           Domain         Decision
─────────────────────────────────────────────
Economy         WHY            requests movement
Route planner   WHERE          immutable execution plan
Controller      WHAT STAGE     state machine + lifecycle
Dispatcher      WHEN           priority + age-based selection
Carrier         HOW            spatial execution only
Telemetry       IS IT HEALTHY  passive observation
```

Route planner(`FindPath`) builds an immutable route at `CreateTask` time.
No code changes route after creation (except `RetryBlockedTasks` rebuilds it).
Controller owns all state transitions via `SetTaskState` (single point, `transitionCount` guard).
Dispatcher(`PickNextTask`) selects among waiting tasks; never touches route/hopIndex/targetFlag.
Carrier moves spatially toward `targetFlag`; never reads route, never modifies task.
Telemetry(`LogTelemetry`) scans state every 600 ticks, `assert oldestWaitingAge < 10000`.

## Key Architectural Decisions

1. **Route = immutable execution plan** — built once at `CreateTask`, never mutated.
2. **One task = one physical unit** — no `amount` field, no batching at task level.
3. **Event-driven lifecycle** — no per-frame scanning, all state transitions from `Notify*` callbacks.
4. **Carrier is a dumb executor** — knows only `targetFlag`, never `route[]` or task state.
5. **Age-based anti-starvation** — `ageBonus = min(tick - createdTick, 200)` computed on selection.
6. **Telemetry = passive observation** — `LogTelemetry()` never modifies state.
7. **transitionCount < 64** — catches infinite state loops.
8. **`Update()` is NOT a decision loop** — exists only for telemetry
   (`LogTelemetry` every 600 ticks) and monotonic tick increment
   (`m_currentTick++` for age bonus). Assignment, route mutation,
   retries, and state transitions remain event-driven from `Notify*`
   callbacks only. See `TransportController::Update()`.

## Build Config
- **Platform**: Xbox 360 (C++03, no variadic templates, `std::function`, auto, range-for)
- **SDK**: Not available for local builds — correctness by code review only

# Architecture — Cycle 2 Complete ✅ (tag: `architecture-cycle-2`)

## Domain First

Domain types and entities are the only stable source of truth. Rendering, UI, localization, metadata and serialization are projections of the domain model and must never become the primary source of game state.

## Milestone: Unified Domain Model

Two parallel migration lines converged:

```
Logistics:  EconomyManager → DemandManager → DemanTicket → Carrier → CargoManager
            (single source of truth for routing)

UI:         Game → MenuBuilder → MenuModel → RadialMenu → UiAction → Game
            (single source of truth for menu content)
```

## Completed

- **UI1** — `UiMessageId` enum + `LocalizationService` (2D table, `Get`/`Format`)
- **UI2** — `NotificationManager` (fixed pool, ID-based `Notify`, `FillFrameContext`)
- **UI3** — `StatusManager` (persistent status, decay timer, ID-based)
- **UI4a** — All 6 `UiEventSystem` event handlers migrated to `NotificationManager`; `GetResourceName()`/`GetBuildingName()` → `GetResourceNameId()`/`GetBuildingNameId()`
- **UI4b** — Dead confirm system fully removed: `IUiInputHost`, confirm fields, GameRenderer confirm block, `"A = Yes B = No"`
- **UI5a** — MenuModel + MenuScene: `MenuItem { labelId, enabled, action }`, `ICommandDispatcher`, zero user-facing string literals
- **UI5b** — GridMenu migrated to MenuModel + UiAction; building labels → UiMessageId
- **UI5c** — RadialMenu migrated: `std::wstring name` → `UiMessageId labelId`; `int typeId` → `UI::UiAction action`; selection delegates to internal `MenuModel`; sync asserts added
- **Logistic PR 1** — Carrier no longer decides cargo destination: `GetDemandTarget`/`GetNextHop` routing → `HasDemand(type)` wake-up only
- **Logistic PR 2** — `DemandTicket` fixed pool (256): `AllocSlot()`/`FreeSlot()`, `assert` on exhaustion + double-free
- **Logistic PR 3** — `destFlagId` audit: proven as ownership tag (not routing); `ResourceDeliveredData::destFlagId` removed (dead); `GetDemandTarget()` removed (0 callers)
- **PR 1** — `BuildingState` enum removed (was always `BS_DONE`)
- **PR 2** — `Building::state` field removed; 13 always-true guards removed; dead `constructionMaterials`/`deliveredMaterials` removed

## Current Layer Stack

```
Gameplay → UiMessageId → NotificationManager/StatusManager → LocalizationService → UiFrameState → GameRenderer
publishes IDs                    stores IDs                    resolves text          DTO              reads only
```

## Component Responsibility Map

| Subsystem | Single Source of Truth | Domain Type(s) |
|-----------|----------------------|----------------|
| Construction | `ConstructionSite` | — |
| Building | Invariants (no `state` field) | — |
| Logistics (routing) | `TransportTask` | `TransportTask` |
| Logistics (stationary) | `ResourceSlot::destFlagId` (ownership tag) | `Flag` |
| UI Menu content | `MenuModel` | `MenuItem`, `UiAction` |
| UI Menu view | `RadialMenu` / `GridMenu` | Geometry, sprites, animation |
| User action | `UiAction` | `UiAction` (command + value) |
| Localization | `LocalizationService` | `UiMessageId` → `char[]` |
| Notifications | `NotificationManager` | `UiMessageId` |
| Status line | `StatusManager` | `UiMessageId` |

## Architecture Audit

### Zero UI string literals in gameplay core (post UI4b)

| File | Before | After |
|------|--------|-------|
| `InputController.cpp` | ~15 raw status text | **0** |
| `RoadController.cpp` | ~6 raw road messages | **0** |
| `GameScene.cpp` | ~5 raw confirm/banner | **0** |
| `GeologistController.cpp` | ~15 raw geologist text | **0** |
| `GameRenderer.cpp` | ~30 confirm block | **0** |
| `UiEventSystem.cpp` | ~53 (6 handlers) | **0** |

### String literals remaining (non-UI)

| Category | Where | Status |
|----------|-------|--------|
| UI text (localized) | `LocalizationService.cpp` (~188) | ✅ Centralized |
| Asset names (sprites/atlas) | `GameRenderer.cpp`, `GameScene.cpp`, `RoadController.cpp`, `MenuBootstrap.cpp`, etc. | ✅ Not UI text |
| Debug/log | `SceneManager.cpp`, `InputController.cpp`, `WorldBootstrap.cpp`, `CarrierSystem.cpp` | ✅ Diagnostics only |
| Editor/Menu UI | `EditorScene.cpp`, `TilePalette.cpp` | ⏳ UI6 target (~268 literals) |

## Transport Contract (Architectural Invariants)

### Rule 1 — TransportTask owns the shipment lifecycle

> **TransportTask owns lifecycle. No other object may create, route, deliver or cancel a shipment.**
> Economy observes shipment completion through `observerTicketId` and never participates in transport execution.

```
CreateTask → Assign → PickUp → AdvanceHop → Delivered → DemandManager::Deliver()
                                                      → Cancel → DemandManager::CancelTicket()
```

**Violation**: any code outside `TransportController::CreateTask`, `SetTaskState`, or `DeliverTask`
that modifies task state or completes/cancels a shipment without going through the Controller.

### Rule 2 — Single mutable representation

> **TransportTask is the only mutable representation of a shipment.**

```
DemandManager  →  observerTicketId (read-only observation)
TransportTask  →  owns all mutable state
Cargo          →  physical representation (resource + ownerTask)
Carrier        →  execution state only (what they carry, where they walk)
```

- `DemandManager` creates the request, never mutates transport state.
- `TransportTask` stores everything: route, hopIndex, cargo link, carrier link, state.
- `Cargo` stores only physical identity (resource type) and back-link to its task.
- `Carrier` stores only execution state: what it's carrying and where it's heading.
- `DemandManager` receives only final notification via `observerTicketId`.

**Violation**: modifying a shipment's route, state, or destination from anywhere
other than `TransportController` member functions.

### Rule 3 — Ownership chain (no DemandTicket in transport)

> **DemandTicket is an adapter at the economy boundary. Transport never sees it.**

```
Economy boundary:
  DemandManager → Reserve() → DemandTicket → observerTicketId on TransportTask

Transport core:
  TransportTask → Cargo (ownerTask) → Carrier (targetFlag)
```

`Cargo` no longer holds a `DemandTicket*`. The ownership chain is:

```
Ground
  → Flag inventory (stationary via ResourceSlot::destFlagId)
    → TransportTask (in transit, owns lifecycle)
      → Carrier (spatial executor, knows only targetFlag)
        → Building inventory (consumed)
```

**Violation**: a `Cargo` containing a `DemandTicket*`, or any transport code
calling `DemandManager::Reserve()`/`ReleaseTicket()` directly.

### Acceptable transitional state — `Ground|Task`

After `CreateTask()` and before `PickUp()`, a resource is physically on the
ground/flag but logically owned by a TransportTask. The telemetry mask
`Ground|Task` is **correct** — ownership precedes physical pickup.

This is not a violation. The invariant is: before PickUp the physical location
and logical owner may differ; after PickUp they must converge (Carrier carries).

### Architectural boundaries

#### Route vs Dispatch (Phase 7.4)
> **Route planner decides WHERE cargo moves.**
> **Priority dispatcher decides WHEN cargo moves.**
> The dispatcher (`PickNextTask`) selects among waiting tasks but never modifies
> route, hopIndex, or targetFlag. These responsibilities must never be mixed.

#### Economy vs Transport (Phase 8)
> **Economy requests movement. Transport performs movement.**
> Economy never moves resources directly.
> DemandTicket lives at the boundary as an adapter — it connects DemandManager's
> request to TransportTask's lifecycle, then drops out of the model.

---

### Roadmap to Transport v1 complete

```
8.3   Parallel Validation    → log [MIGRATION] old=new per delivery
8.3.5 Soak Tests             → T1–T8, 30+ min runs, telemetry clean
8.4   Legacy Removal          → delete DemandTicket, TransportJobManager, Reserve, bridge code
tag   transport-v1-complete   → stable baseline for Cycle 3
Cycle 3  Definition Pattern  → BuildingDefinition table
```

# Phase 8 Migration Checklist

## 8.1 — Demand → TransportTask adapter (bridge)
- [ ] DemandManager creates TransportTasks (old pipeline still alive)
- [ ] Invariant: one Demand = at most one active TransportTask
- [ ] CargoManager reports delivery completion to DemandManager

## 8.2 — Resource ownership migration
- [ ] Ownership chain enforced: Ground → Flag → Task → Carrier → Building
- [ ] Runtime audit: `[Resource] id=712 owner=TransportTask(17)` (debug)
- [ ] Remove old ownership paths (Reserve/Allocate)

## 8.3 — Parallel validation mode
- [ ] old TransportJobManager = observe only
- [ ] new TransportController = execute
- [ ] Log: `[MIGRATION] demand=81 old=flag12 new=flag12 OK`

## 8.3.5 — Soak tests (destructive first)
- [ ] T4: Road break — retry blocked tasks, no orphaned TransportTask/Cargo
- [ ] T5: Flag deletion during active transport — cancel task, restore demand, no Cargo leak
- [ ] T6: Cancel during PickUp
- [ ] T7: Cancel during Move
- [ ] T2: 10 simultaneous construction sites (producer + consumer scaling)
- [ ] T3: Long road chains — multiple AdvanceHop across 5+ flags
- [ ] T8: 30–60 min gameplay — telemetry clean (mask/res/ownHash/blocked)
- [ ] T1: Full regression — single resource, single carrier (baseline sanity)

## 8.4 — Remove legacy transport
- [ ] All scenarios pass: wood→warehouse, warehouse→construction, mine→smelter, food→worker, blocked recovery, flag deletion
- [ ] Same save → old and new produce equivalent resource distribution (± delivery timing)
- [ ] Remove TransportJobManager, DemandTicket, `kUseTransportJobs`, old Carrier routing
- [ ] Transport tests green

### Equivalence criteria for 8.4

**Strong invariants** (must be identical):
- Final resource counts at each building/flag
- Destination inventories (what arrived where)
- Construction completion (same buildings finish)
- Blocked recovery result (same resources reachable)

**Weak invariants** (allowed to differ):
- Delivery timestamps (timing shifts are fine)
- Carrier identity (who carried is irrelevant)
- Hop count (new route may differ)
- Queue order (priority dispatching reorders fairly)

**Conservation invariant** (global — applies to whole economy):
```
Σ(world resources) before migration
==
Σ(world resources) after migration
```
No resource may disappear, duplicate, or simultaneously belong to two layers.
This holds across all explicit create/destroy events.

**Migration complete** ⇔ new transport produces economically equivalent world state without ownership violations.

---

# Current Status — Full Cycle Verified ✅

## What works (end-to-end confirmed via log analysis)

```
ConstructionSite → DemandManager → TransportController → Carrier → Flag → CheckDeliveries → ConstructionSite → Builder → Completed
```

Full single-site cycle confirmed: Woodcutter at (20,33) — dispatch, walk, build, resource delivery (3/3 Wood), completion, builder return, site removal.

Multi-site with shared roads: Hunter at (18,38) received all 3 Wood via road 2 carrier. Fisher at (24,30) transport chain functional after Reserve fix.

## Recent fixes (Phase 8.3)

| Fix | File | What |
|-----|------|------|
| `HasDemandFromOtherFlag` | `DemandManager.h:41`, `DemandManager.cpp:355` | Carrier idle check skips demands targeting the same flag |
| `Reserve` same-flag filter | `DemandManager.cpp:184-196` | When originFlag > 0, skip demands whose resolved target flag ID matches origin — prevents warehouse demand (priority 10) from blocking construction demand (priority 5) at the same flag |
| Carrier idle uses `HasDemandFromOtherFlag` | `Carrier.h:250` | Prevents wasted wake → walk → Reserve → Release cycles for resources already at their destination |

## Remaining issues

- **OVERDELIVER telemetry** (`delivered=3/1`): artifact of `SetDemand(woodMissing)` overwriting `requested` downward each frame. Harmless — delivery and ticket release complete normally. Fix: log initial amount separately, or don't reduce `requested` in `SetDemand`.
- **Builder dispatch timing**: Builder waits at `"no road to site"` until player builds road. This is correct Settlers behavior — not a bug.

# Stabilization Checklist

## Logistics
- [x] Building receives all required resources (confirmed: Woodcutter 3/3, Hunter 3/3)
- [ ] Production buildings get input resources
- [x] Warehouses collect only truly free resources (Reserve filter prevents same-flag pickup)
- [ ] Flag deletion leaves no orphaned resources
- [ ] No DemandTicket pool asserts triggered
- [ ] No DemandTicket leaks on map clear / return to menu

## Construction
- [x] Open build menu
- [x] Select any building
- [x] Place building
- [x] Wood delivery
- [ ] Stone delivery
- [x] Construction completion (confirmed: Woodcutter, Hunter)

## Known Pre-existing Bugs (not caused by architecture cycle)
- **Construction completion order**: `ConstructionManager::Update()` deletes completed sites before `PostUpdate()` fires `Event_ConstructionComplete`
- **Building placement**: cursor does not change after selecting building icon from build menu (traced through menu selection → EnterBuildMode chain; cause not in PR 1)

---

# Next Steps

1. **Soak tests** (8.3.5): T1–T8 — multiple buildings, road delete, flag delete, long routes, mass construction
2. **Phase 8.4**: Remove legacy TransportJobManager, DemandTicket, Reserve bridge code, `kUseTransportJobs`
3. **Tag**: `transport-v1-complete`
4. **UI6**: EditorScene migration to MenuModel / UiMessageId pattern (~268 string literals in EditorScene.cpp + TilePalette.cpp)
5. **Cycle 3**: Definition Pattern — `BuildingDefinition` table

## Definition Pattern (Architectural Invariant for Cycle 3)

## Definition Pattern (Architectural Invariant for Cycle 3)

Domain types are the only stable identifiers. UI assets, render assets, metadata and gameplay properties must be obtained from definition tables/services keyed by the domain type. Systems must not derive domain types from resource names (sprite names, icon names, localized strings).

**Permitted:**
```
BuildingType → sprite name
BuildingType → icon name
BuildingType → cost
BuildingType → metadata
```

**Forbidden:**
```
sprite name → BuildingType
icon name   → BuildingType
```

### Precedent in current architecture

| Domain Type | Definition Source |
|-------------|------------------|
| `UiMessageId` | `LocalizationService` (2D enum→string table) |
| `BuildingType` | `BuildingDefinition` (planned: sprite, icon, cost, size, class) |
| `ResourceType` | `ResourceDefinition` (future) |
| `WorkerType` | `WorkerDefinition` (future) |

## Phase 6b — ARCHIVED (Research Prototype)

Phase 6b (`kUseTransportJobs` experiment) successfully identified a fundamental architectural flaw:
> A logistics system needs **one TransportTask per shipment**, not one TransportJob per hop.
> Split responsibility between Job/Cargo/Carrier caused sourceFlag mismatch on intermediate hops.

All lessons and the new architecture are in `docs/LOGISTICS_ARCHITECTURE.md` (FROZEN v1.0).

## Phase 7 — Clean-slate Logistics

**Principle:** TransportController is the sole decision-maker. Carrier only executes PickUp/Walk/Drop.
One `TransportTask` = one physical item, one lifecycle, one id.

See `docs/PHASE7_IMPLEMENTATION_PLAN.md` for detailed phase order.

## Boundary Rules for Future PRs
- Carrier never decides cargo destination — routing decision centralised in TransportController (Phase 7)
- PRs change one architectural aspect each; never combine cleanup with behavioural change
- GameRenderer remains read-only (no mutation of world state)
- UI widgets speak `UiAction`; `ICommandDispatcher` is single execution point
- Dead code removal is consequence, not goal; API surfaces unchanged in architectural PRs
- **Rule during Phase 7**: No features outside `docs/LOGISTICS_ARCHITECTURE.md`. Extensions after stable baseline.
