# AGENTS.md — Architectural Invariants & Cross-References

## Transport v2 — Stable ✅ (PR A/B/C + Priority A/B/C) — Milestone 2026-07-03

```
Layer           Domain         Decision
─────────────────────────────────────────────
DemandManager   WHY            requests movement (exclusive publisher)
Route planner   WHERE          immutable execution plan
Controller      WHAT STAGE     state machine + lifecycle
Dispatcher      WHEN           priority + age-based selection
Carrier         HOW            spatial execution only
Telemetry       IS IT HEALTHY  passive observation
```

Route planner(`FindPath`) builds an immutable route at `CreateTask` time.
Controller owns all state transitions via `SetTaskState` (single point, `transitionCount` guard, max 64).
Dispatcher(`PickNextTask`) selects among waiting tasks: `score = basePriority + age`, FIFO tiebreaker (`enqueueOrder`). Never touches route/hopIndex/targetFlag.
**PriorityForReason** is the single mapping from `TransportTaskReason` → `basePriority`. Dispatcher never inspects reason, owner, or domain origin — only `task.basePriority`.
Carrier moves spatially toward `targetFlag`; never reads route, never modifies task.
Telemetry(`LogTelemetry`) scans state every 600 ticks, `assert oldestWaitingAge < 10000`.

### Feature-freeze invariant

**Transport subsystem is considered complete.** Any functional change to `Dispatcher`,
scheduling, or task lifecycle requires a failing integration or soak test demonstrating
the necessity of the change. Bug fixes, optimization, new tests, and new domain clients
(via `DemandManager`) are permitted without exception.

**Future domain systems connect through `DemandManager` — never by modifying Transport:**

```
ProductionSystem  →  DemandManager  →  Transport   (unchanged)
WorkerAI          →  DemandManager  →  Transport   (unchanged)
MilitaryLogistics →  DemandManager  →  Transport   (unchanged)
```

If a new domain system requires changes to `TransportController`, the contract with
Transport must be redesigned — Transport is not extended by default.

## Render Pipeline — Seven Invariants

```
1. Simulation  →  never knows about Rendering.
2. Presentation  →  never knows about Graphics API.
3. Projection  →  the only place world becomes screen.
4. swap()  →  the only frame publication boundary.
5. After swap()  →  RenderFrame is immutable.
6. RenderGraph  →  the sole owner of pass execution order.
7. Pass  →  only receives RenderFrame, RenderContext, CommandBuffer.
```

Derived: `Presentation reads Simulation, writes RenderFrame. After swap(), only RenderGraph reads RenderFrame. No Pass may reference a Simulation Manager, Controller, Map, or FrameContext. Infrastructure services (TextRenderer, FontService) are permitted in Pass.`

## GameScene Render Invariant

```
GameScene rendering is fully RenderFrame-driven.
All remaining direct rendering paths belong to standalone auxiliary scenes
(EditorScene, MenuScene, LoadingScene) and are outside the gameplay rendering architecture.
```

Auxiliary scenes use their own `Render()` methods and are not required to follow the `RenderFrame → RenderGraph → Pass` pipeline. They have no simulation state to snapshot and no Core0/Core1 split requirement.

## Core Assignments (Xbox 360 target)

```
Core2      AI
Core1      Simulation  →  Presentation  →  Projection  →  Publish(RenderFrame)
Core0      Acquire(RenderFrame)  →  RenderGraph  →  CommandBuffer  →  GPU
```

## Critical Invariant — `hasBuilding` vs `building`

```
Flag::hasBuilding  == reservation / planned building
Flag::building != NULL == instantiated building object
```
**Never use `hasBuilding` as a check for object existence.** A flag with a planned construction site has `hasBuilding=true` but `building=NULL`. Use `flag->building != NULL` when you need to know whether a `Building` object actually exists.

## Simulation Foundation v2 — Definition of Done ✅

```
1. Simulation — единственная точка входа.
2. WorldModel — единственный источник доменного состояния.
3. Все доменные системы реализуют ISimulationSystem.
4. Simulation знает только интерфейс систем и порядок их выполнения.
5. Межсистемное взаимодействие — через WorldModel (Requests/Events).
6. Headless execution.
7. Deterministic execution.
8. Platform-independent.
9. Новые системы регистрируются через AddSystem() без изменения Simulation.cpp.
10. Существует минимум один полностью замкнутый сквозной pipeline.
```

## Definition Pattern (Cycle 3)

Domain types are the only stable identifiers. UI assets, render assets, metadata and gameplay properties must be obtained from definition tables/services keyed by the domain type. Systems must not derive domain types from resource names.

**Permitted:** `BuildingType → sprite name`, `BuildingType → icon name`, `BuildingType → cost`
**Forbidden:** `sprite name → BuildingType`, `icon name → BuildingType`

**Implementation plan:** Replace hardcoded switch-based tables (GetBuildingCost, requiredProgress)
with data-driven `BuildingDefinition[]`, `ResourceDefinition[]`, `WorkerDefinition[]`
in a new `Definitions/` namespace. Systems read from definitions, not from switch statements.

```
BuildingType → BuildingDefinition { cost[], buildTime, footprint, produces, consumes, workerSlots }
```

`GetBuildingCost(type)` → `BuildingDefinitions[type].buildCost`
`requiredProgress = 100` → `BuildingDefinitions[type].buildTime`

### Evolution: from data storage to knowledge base (Cycle 4+)

The Definition Pattern evolves beyond refactoring — it becomes the model Settlement AI
queries to reason about the economy:

```
ProductionDefinition
    → consumes[], produces[], cycleTime
    → resource transformation graph (implicit)

Query API:
    GetProducer(ResourceType)          → BuildingType that produces this resource
    GetConsumers(ResourceType)         → BuildingType[] that consume this resource
    GetInputs(BuildingType)            → ResourceType[] required for production
    GetOutputs(BuildingType)           → ResourceType[] produced
```

Settlement AI no longer contains building-specific knowledge:

```
Before:  if (resource == Stone) BuildQuarry();
After:   producer = GetProducer(stone);  Build(producer);
```

This enables automatic dependency chain resolution:

```
Need Tools
    → Producer(Tools) = Toolmaker
    → Inputs(Toolmaker) = { Wood, Stone }
    → Need Stone → Producer(Stone) = Quarry
    → Inputs(Quarry) = {}  → ready to build
```

## Boundary Rules for PRs

- Carrier never decides cargo destination
- PRs change one architectural aspect each; never combine cleanup with behavioural change
- GameRenderer remains read-only (no mutation of world state)
- UI widgets speak `UiAction`; `ICommandDispatcher` is single execution point
- Dead code removal is consequence, not goal
- All 3rd-party types in Scene/ headers must be forward-declared in their **real** namespace

## SimulationCore — Cross-System Communication

```
All cross-system communication is expressed as changes to WorldModel
or explicit domain requests. Systems never invoke domain logic in
other systems directly.
```

Each system publishes **Requests** and consumes **Events** through WorldModel.
Direct calls between domain systems are prohibited.

Derived from PR12/PR16: Outbound contract (Request) and inbound contract (Event)
form symmetric communication:

```
System (Outbound) → Request → WorldModel → Simulation → Executor
System (Inbound)  ← Event  ← WorldModel ← Simulation ← Executor
```

Concrete example (Construction → Transport → Delivery):

```
ConstructionSystem
        │
        ▼  (outbound)
TransportRequest  (via DemandManager::SetDemand)
        │
        ▼  (coordinator routes)
TransportController
        │
        ▼  (inbound)
DeliveryEvent  (via DemandManager::CompleteDemand)
        │
        ▼
ConstructionSystem
```

## DeliveryEvent Lifecycle Invariant — PR 1.5

```
DeliveryEvent is published by Transport at the end of Tick N.
All domain systems consume it at the start of Tick N+1.
ClearDeliveryEvents() runs AFTER all consumers, BEFORE the next transport batch.
```

**Tick order enforced in `Simulation::Tick()`:**
```
1. Systems run — read DeliveryEvent/JobEvent from previous tick
2. ClearDeliveryEvents() + ClearJobEvents() — clear consumed events
3. ProcessTransportRequests — convert pending to tasks
4. transportDriver.Tick() — drive carrier lifecycle
5. WorkerSystem::CaptureJobEvents() — publish job events for next tick
6. Telemetry from WorldModel
7. transport.Update() + CaptureDeliveryEvents() — publish delivery events for next tick
```

**Consequence:** A DeliveryEvent lives for exactly one system processing stage.
It is never available for two ticks, never consumed twice, never lost.

**Consequence:** A JobEvent lives for exactly one system processing stage.
It is never available for two ticks, never consumed twice, never lost.

**Test verification (T8 integration):**
- ConstructionSystem resources are marked `delivered` after transport completes
- ProductionBuilding `inputDelivered` increments after transport delivers input
- Both prove ProcessDeliveryEvents / HandleDeliveryEvents received the event

DemandManager is the exclusive publisher of `TransportRequest[]` for construction
(and future domain needs). EconomySystem must never create transport requests directly.
Simulation reads requests and converts; TransportController executes.
Neither system knows the other's internals.

## SimulationCore — Adding New Systems

New `ISimulationSystem` implementations are plugged in via public `AddSystem()` — `Simulation.cpp` is never modified:

```cpp
sim.AddSystem(new EconomySystem());
sim.AddSystem(new ConstructionSystem());
```

```
Simulation::Tick():
    for each system: system.Tick(world)       → reads DeliveryEvent (prev tick), writes requests
    ClearDeliveryEvents()                      → consumed events cleared
    ClearJobEvents()                           → consumed events cleared
    ProcessTransportRequests()                 → Convert to transport tasks
    if transportDriver: transportDriver.Tick() → drive carrier lifecycle
    WorkerSystem::CaptureJobEvents()           → publish job events for next tick
    Telemetry from WorldModel
    transport.Update(dt)                       → Execute movement
    CaptureDeliveryEvents()                    → publish events for next tick
```

## Three Development Tracks

After Cycle 2 (PR18) → PR19 (Validation) → PR20 (BuildingDefinition) → PR21 (ResourceDefinition) → PR22 (ProductionDefinition) → PR23 (WorkerDefinition) → PR24 (Scenario Framework + Validation v2), development splits into independent tracks:

**A. Simulation Engine** — new domain systems:
Worker AI, Production, Economy expansion, Save/Load, Building lifecycle.

**B. Definition Pattern** — data-driven definitions:
BuildingDefinition ✅, ResourceDefinition ✅, ProductionDefinition ✅, WorkerDefinition ✅.
Each definition PR is a behavior-preserving refactoring: no logic change, only data source.
Systems appear **after** definitions (e.g. ProductionSystem after ProductionDefinition).
PR sequence: PR21 ResourceDefinition → PR22 ProductionDefinition → PR23 WorkerDefinition → PR24 Scenario Framework + Validation v2 → PR25 ProductionSystem ✅ (data-driven, no switch statements).

**C. Simulation Validation** — scenarios + invariants:
`IScenario` interface ✅, ScenarioRegistry, T1–T22 regression suite, categorized assertions (Transport, Construction, World) with `Assert::AllInvariants()` facade, AI fuzzing.

These tracks are independent: Definition Pattern is tested by existing scenarios, new Engine systems add their own scenarios, Validation expands coverage without touching domain code.

## Build Config

- **Platform**: Xbox 360 (C++03, no variadic templates, `std::function`, auto, range-for)
- **SDK**: Not available for local builds — correctness by code review only

## Temporary Compatibility Layer — Duplicate Type Definitions

While legacy `World/` and `SimulationCore/` are both compiled into the game project (`Settlers2.vcxproj`), identical types
in `World::` collide. The current fix uses shared include guards so the SimulationCore canonical version wins:

| Legacy (World) | Canonical (SimulationCore) | Plan |
|---|---|---|
| `World/TransportTypes.h` (enums + PriorityForReason) | `Transport/TransportTypes.h` | forwarding header → `SimulationCore/Transport/TransportTypes.h` |
| `World/TransportRoute.h` (struct + constant) | `Transport/TransportRoute.h` | forwarding header → `SimulationCore/Transport/TransportRoute.h` |
| `World/ResourceNode.h` (enum ResourceType only) | `Core/ResourceTypes.h` | forwarding header → `SimulationCore/Core/ResourceTypes.h` |

**Rule — no new duplicates:** Until migration is complete, all new shared types must be created only in
`SimulationCore`. Legacy `World` headers may not introduce new type definitions that overlap with
`SimulationCore`. If a type needs to be visible in both, add a forwarding header in `World/`.

### Cleanup PR Exit Criteria

When the migration is deemed complete, a dedicated cleanup PR must satisfy:

1. World transport/resource headers no longer contain their own type definitions — only `#include` or forwarding.
2. All shared types are defined only in SimulationCore.
3. Temporary include guards (`SIMCORE_*`) are removed.
4. `Settlers2`, `SimulationCore`, `SimulationCoreTests`, and `SimulationRunner` all build without redefinition errors.
5. All unit tests and scenarios (T1–T8) pass with identical behavior.
6. No files outside `World/` include the legacy headers directly (they include SimulationCore versions instead).

---

## Documentation Split

| File | Content |
|------|---------|
| `ARCHITECTURE.md` | Full architecture documentation (Transport v2, Render Pipeline, Component Responsibility Map, Platform Milestones v1: Production/Economy/Warehouse) |
| `ROADMAP.md` | Pipeline stages, Scene maturity, PR sequence, Cycle 2 (Domain Systems) plan |
| `CHANGELOG.md` | Cycle history, Phase 6b, Post-merge stabilization, Cycle 1 completion |
| `MIGRATION.md` | Phase 8 checklist, Current status, Stabilization checklist, Verification criteria |
| This file | Essential invariants, cross-references, build config |

## Production Integration ✅

Four PRs completed:

**PR 1 — DemandOwner** ✅:
- `enum DemandOwner { Construction, Production }`
- `Demand.owner`, `TransportRequest.owner`
- `PublishTransportRequests` copies owner
- Integration tests: Construction + Production publish simultaneously

**PR 1.5 — DeliveryEvent lifecycle ordering** ✅:
- `ClearDeliveryEvents()` moved after system ticks (consumers), before transport (publishers)
- DeliveryEvent lifecycle invariant
- Integration test: T8 verifies Construction + Production delivery events are consumed

**PR 2 — ProductionSystem as demand publisher** ✅:
- `ProductionBuilding` → `SetDemand(...)` when input resource missing
- `Demand.reason` field added, `SetDemand` accepts `TransportTaskReason`
- Integration test: T8 (Sawmill + planks), T9 (regression hardening)

**PR 3 — Delivery → Production close loop** ✅:
- `CompleteDemand()` via `IDemandService` → Production resumes on delivery
- First closed production cycle verified end-to-end

**Invariant:** No changes to `TransportController`, `Carrier`,
`SimpleTransportDriver`, or `TransportTask` throughout these PRs.

## Priority Differentiation ✅

**PR A — PriorityForReason**:
- `TTR_Construction` → `TBP_High` (200), `TTR_Production` → `TBP_Normal` (100)
- Single change in `TransportTypes.h` — `PriorityForReason` is the exclusive mapping
- Dispatcher (`PickNextTask`) remains domain-agnostic: `score = basePriority + age`
- Tiebreaker: `enqueueOrder` (FIFO) for equal scores — deterministic, no domain inspection

**PR B — T10 Priority Test**:
- Mixed construction (`TBP_High`) + production (`TBP_Normal`) scenario
- Verifies both priority levels coexist and complete
- Validates `PriorityForReason` returns correct values for both reasons

**PR C — T11 Fairness Soak**:
- Long-run (50k ticks) stress test with mixed priorities
- 3 Woodcutters (Construction → `TBP_High`) + 2 Sawmills (Production → `TBP_Normal`)
- Periodic assertion checks, demand tracking, fulfillment rate monitoring
- Verifies `priority + age` is sufficient: no starvation, both classes served deterministically
- Establishes baseline: if starvation is ever observed, only THEN design new scheduler

## Production v1 — Stable ✅

Three PRs completed:

**PR 1 — Output buffer + multi-cycle validation** ✅:
- `ProductionBuilding` gains `outputResources[]`, `outputBuffer[]`, `totalOutput[]`
- `ProcessProduction` buffers output on cycle completion (replaces orphan TransportRequest with origin=destination=0)
- T12 verifies: multi-cycle output accumulation, monotonic totalOutput, Woodcutter→Sawmill pipeline

**PR 3 — Production Soak** ✅:
- 50k ticks, 3 Woodcutters + 2 Sawmills
- Verifies sustained output, demand stability, no buffer corruption
- T13: periodic checks, monotonic totalOutput, throughput >= 1500 per Woodcutter

**PR 2 — Multi-input production** ✅:
- Toolmaker (Wood + Stone → Tools) end-to-end
- Verifies: partial deliveries, allDelivered gate, SetDemand per resource, CompleteDemand by ticket
- T14: 2000 ticks, Woodcutter + Stonemason + Toolmaker, checks Tools produced
- Zero changes to ProductionSystem — existing input arrays already handle multi-input

### Production v1 invariants

```
Output accumulates monotonically — per-building totalOutput never decreases.
Cycle consumes inputs atomically — all inputDelivered reset simultaneously.
SetDemand fires exactly once per cycle — inputsRequested guard prevents duplicates.
CompleteDemand unblocks production — input delivery triggers cycle progression.
ProductionSystem never publishes transport requests for completed output.
Finished goods remain in the building's output buffer until another domain system
(e.g. Warehouse, Market, or Consumer) creates demand for them.
```

### Feature-freeze invariant

**Production v1 is considered complete.** Any functional change to `ProductionSystem`,
cycle logic, or input/output handling requires a failing integration or soak test
demonstrating the necessity of the change. Bug fixes, new tests, and connections to
new domain systems (via `DemandManager` / `outputBuffer`) are permitted without exception.

Future domain systems consume `ProductionBuilding.outputBuffer[]` through
`DemandManager` — never by modifying ProductionSystem.

## Warehouse v1 — Stable ✅

WarehouseSystem is a DemandManager client that consumes production output. It monitors
`ProductionBuilding.outputBuffer[]` and creates `TTR_WarehouseBalance` transport requests
to move finished goods to warehouse storage. On delivery, it decrements the source
building's `outputBuffer` and increments its own stockpile.

```
Production → outputBuffer → WarehouseSystem (SetDemand) → Transport → Warehouse
```

Zero changes to Transport or Production — both systems remain under feature freeze.

### PR 1 — Production → Warehouse pipeline ✅
- `WarehouseSystem` monitors `outputBuffer[]`, creates demands via `DemandManager`
- `HandleDeliveryEvents` processes warehouse deliveries, decrements source buffer
- Internal stockpile tracking (`GetStockpileAmount`)
- T15: 2 Woodcutters + 1 Sawmill → warehouse receives Wood + Planks
- Stockpile grows, outputBuffer drains, no changes to ProductionSystem

### PR 2 — Warehouse Soak ✅
- 50k ticks, 3 Woodcutters + 2 Sawmills
- Periodic monotonic stockpile check (never decreases)
- Final resource balance: produced = warehouse + outputBuffer (within in-flight tolerance)
- Demand tracking: <10 stuck demands acceptable
- T17: 50k warehouse soak — verified stable throughput, no resource leak, demand tracking stable
- No functional changes — pure soak

## Economy v1 — Stable ✅

EconomySystem tracks cumulative resource production and derived consumption using
data-driven `ProductionDefinition`. No changes to Transport or Production — both
remain under feature freeze.

- `GetTotalProduced(type)` — accumulated from `ProductionBuilding.totalOutput[]` across all buildings
- `GetTotalConsumed(type)` — derived from production definitions (e.g. Sawmill consumes 2 Wood per Plank)
- `EconomySystem* m_economySystem` member on `Simulation`, exposed via `GetEconomySystem()`
- T16: 2 Woodcutters + 1 Sawmill, 500 ticks, verifies:
  - EconomySystem totals match direct `totalOutput` reads
  - Wood consumption == Planks produced × 2 (per Sawmill definition)
  - Planks consumption == 0 (no consumer defined)
  - Production > consumption (Wood surplus)
- `enableEconomy = true` required (same flag used by DemandManager)

### Economy observational invariant

```
EconomySystem is observational. It derives economic metrics from world state
and never drives gameplay by mutating ProductionBuilding, Transport, or
DemandManager. ProductionBuilding::totalOutput is the canonical source for
all resource production data. Consumption is derived from ProductionDefinition,
not from direct counters — every new building type automatically extends
economic coverage. EconomySystem adds zero overhead to production, transport,
or warehouse hot paths.
```

Consequence: If EconomySystem is removed, gameplay is unaffected — only
telemetry is lost.

### Warehouse v1 invariants

```
WarehouseSystem monitors outputBuffer, never writes it.
    → Production is the sole writer of outputBuffer.
    → Warehouse only calls SetDemand/CompleteDemand through DemandManager.

Warehouse decrements outputBuffer on delivery receipt, one unit at a time.
    → Matches CaptureDeliveryEvents granularity (amount=1).
    → Prevents double-counting: each delivery decrements exactly its own unit.

Stockpile per resource = sum of deliveries received - sum of decrements.
    → Invariant verified in soak: stockpile never decreases unintentionally.

Transport never knows about warehouse.
    → TTR_WarehouseBalance is routed through existing Dispatcher (priority+age).
    → No new Dispatcher logic, no task modification, no domain inspection.

No changes to Production, Transport, or Economy.
    → Feature freeze on all three remains in effect.
```

## Worker v1 — Complete ✅ (PR 1: Job System)

### Architecture (v1)

```
Domain Systems (Production, Construction)
         │
         ▼  CreateJob()
    JobManager
         │
         ▼  AcquireJob()
   WorkerSystem
         │
         ▼
   Worker (Idle → FindingJob → Assigned → Walking)
```

Worker owns at most one active Job. Job is owned by at most one Worker.
Assignment is exclusive. JobManager is the sole publisher of jobs —
WorkerSystem never scans the world for work.

### v1 invariants

```
WorkerState flow: Idle → FindingJob → Assigned → Walking
Each tick: Idle workers transition to FindingJob
    → FindingJob calls JobManager::AcquireJob()
    → If job acquired → Assigned (worker.currentJob = job.id)
    → If no job → back to Idle
    → Assigned → Walking (movement not implemented — immediately transitions to Working)

A Worker owns at most one active Job.
A Job is assigned to at most one Worker.
JobManager is separate from WorkerSystem.
    → WorkerSystem never creates, stores, or queues jobs.
    → It only acquires and completes them.
```

### PR 1 — Job acquisition pipeline ✅
- `JobManager` (new `ISimulationSystem`): `CreateJob`, `AcquireJob`, `CompleteJob`, `ReleaseJob`
- `WorkerSystem` v1: state machine — `Idle → FindingJob → Assigned → Walking`
- `WorldModel::workers[]` — per-worker state storage
- `Core/JobTypes.h` — `JobId`, `JobState`, `JobType`, `Job` struct
- `Core/WorkerTypes.h` — extended with `WorkerId`, `WorkerState`
- T18: 3 jobs, 2 workers — 2 assigned, 1 waiting; exclusive assignment invariant
- T19: 1 worker, 2 jobs — acquire → release → re-acquire; verifies ReleaseJob contract, no leaks

## Worker v2 — Execution ✅

### Extended Architecture

```
Idle → FindingJob → Assigned → Walking (immediate)
                                         │
                                         ▼
                                     Working
                                         │
                                    duration--
                                         │
                                         0
                                         │
                                         ▼
                                  CompleteJob()
                                         │
                                         ▼
                                      Idle
```

WorkerSystem advances `worker.workTicksRemaining` each tick while Working.
When it reaches 0, WorkerSystem calls `JobManager::CompleteJob()`.
JobManager changes job state to `JobState_Completed` and clears the worker.
Worker returns to Idle — next tick restarts the acquire cycle.

### v2 invariants

```
WorkerState flow: Idle → FindingJob → Assigned → Walking → Working → Idle
Walking completes immediately (v2) — copies job.duration to worker.workTicksRemaining
Working decrements workTicksRemaining each tick
On 0 → CompleteJob() → worker returns to Idle

WorkerSystem may advance Job execution, but only JobManager changes
Job ownership and lifetime. Worker calls CompleteJob() via JobManager.

duration lives in Job (not Worker) — work duration is defined by the job,
not the worker. Keeps separation: JobManager owns job data, WorkerSystem
tracks execution progress (workTicksRemaining is on Worker).

No switch on JobType — all jobs are N ticks of work in v2.
```

### PR 2 — Job execution pipeline ✅
- `Job.duration` — work time defined by the job (default 10)
- `Worker.workTicksRemaining` — per-worker countdown, copied from job on arrival
- `WorkerState_Working` — new state in the lifecycle
- `JobState_Completed` — new terminal state, set only by `JobManager::CompleteJob()`
- `T20`: 1 worker, 1 job(duration=10) — full lifecycle verified (25 ticks):
  - Worker returns to Idle, job Completed, currentJob reset, workTicksRemaining=0
- `T21`: 1 worker, 3 jobs(duration=5 each) — sequential execution (50 ticks):
  - All 3 jobs completed, worker idle, no waiting/assigned jobs remain
  - Proves acquire→execute→complete→re-acquire cycle works indefinitely
- Zero changes to JobManager's acquisition/release contract — v2 adds only `duration` param and `CompleteJob → JobState_Completed`

## Worker v3 — Job Completion Events ✅

### Architecture

```
WorkerSystem (Tick)
      │
CompleteJob()
      │
      ▼
CaptureCompletedJob()  →  m_pendingJobEvents[]
      │
      ▼  (after system loop, before next tick)
CaptureJobEvents()     →  WorldModel::jobEvents[]
      │
      ▼  (next tick, system loop)
Domain subscribers     read  jobEvents[]
      │
      ▼
ClearJobEvents()       →  consumed events cleared
```

### JobEvent Lifecycle Invariant — PR 3

```
JobEvent is published by WorkerSystem at the end of Tick N
(after the system loop, via CaptureJobEvents()).
All domain systems consume it at the start of Tick N+1
(during the system loop).
ClearJobEvents() runs AFTER all consumers, BEFORE WorkerSystem
captures new events.
```

**Tick order enforced in `Simulation::Tick()`:**
```
1. Systems run — read JobEvent from previous tick's WorkerSystem
2. ClearDeliveryEvents() + ClearJobEvents() — clear consumed events
3. ProcessTransportRequests
4. transportDriver.Tick()
5. WorkerSystem::CaptureJobEvents() — publish events for next tick
6. transport.Update() + CaptureDeliveryEvents()
```

**Consequence:** A JobEvent lives for exactly one system processing stage.
It is never available for two ticks, never consumed twice, never lost.

### Platform symmetry

```
Subsystem   Manager         Executor            Event
──────────  ──────────────  ──────────────────  ─────────────────
Transport   DemandManager   TransportController DeliveryEvent
Worker      JobManager      WorkerSystem        JobEvent
```

### PR 3 — JobEvent pipeline ✅
- `JobEventType` enum (`JET_Completed`) and `JobEvent` struct — universal, extensible
- `WorldModel::jobEvents[]`, `jobEventCount` — event storage (max 64, matching DeliveryEvent)
- `WorkerSystem::CaptureJobEvents()` — flushes internal buffer to WorldModel after system loop
- `Simulation::ClearJobEvents()` — resets event count after consumers
- `WorkerSystem::CaptureCompletedJob()` — internal buffer, written during `ProcessWorkingWorker`
- Zero changes to WorkerSystem state machine — events are published AFTER completion, not instead of it
- Zero changes to domain systems — no subscriber exists yet (PR 4+)
- Invariant added: `WorkerSystem publishes JobEvents but never interprets them. Domain systems subscribe to JobEvents and perform world mutations.`

## Settlement v1 — Observing ✅

### Architecture

```
Observe → Decide → Publish Intent → Wait for Events → Observe
```

Settlement AI is the first proactive system on the platform. It reads world state,
decides what to do next, publishes Job/Demand, and waits for completion events.
It never executes decisions — only publishes intent.

### v1 invariants

```
SettlementSystem publishes only Job and Demand — never writes to WorldModel directly.
SettlementSystem reads only published state (WorldModel, EconomySystem, WarehouseSystem).
SettlementSystem does not create TransportTask, assign Worker, or change Job/Demand state.
Each decision is stateless and re-evaluated every tick. Guard conditions prevent duplicate publishing.
```

### PR 1 — Decision loop ✅
- `SettlementSystem` (new `ISimulationSystem`): `Observe → Decide → Publish`
- First rule: Ensure bootstrap production (Woodcutter)
- Guard: if building exists, is under construction, or has pending Job → skip
- `CreateJob(JobType_Construction, targetFlag, (uint8_t)BuildingType_Woodcutter, duration)`
- Zero changes to JobManager, WorkerSystem, or any existing platform pattern
- T22: Settlement publishes exactly one BuildWoodcutter Job, no duplicates, no direct world mutations

## Development Process: Circuit-Driven

### Principle
Each economic circuit is defined by an integration scenario that serves as its
specification. PRs are named after circuits, not numbers. Progress is measured
by completed circuits, not subsystem implementation percentage.

### Development cycle
```
1. Define next economic circuit (goal + invariants)
2. Write integration scenario that encodes the expected behaviour
3. Add minimal bootstrap/expansion rules to pass the scenario
4. Extend EconomySystem observations only when scenario reveals a blind spot
5. Lock circuit with regression — scenario becomes specification
```

### Circuit completion tracker

Each circuit has two goals: a gameplay capability and an architectural hypothesis it validates.

| Circuit | Status | Scenario | Gameplay Goal | Architectural Hypothesis |
|---------|--------|----------|--------------|-------------------------|
| Observation API | ✅ | T32 | — | Give AI the data it needs to decide |
| Definition Query API | ✅ | T33 | — | AI discovers producers from definitions, not hardcoded names |
| Wood    | ✅     | T31      | Woodcutter → Sawmill pipeline | Observe → Decide → Publish works end-to-end |
| Stone   | ✅     | T34      | Stonemason bootstrap + production | Definition Query API enables adding a new resource without AI changes |
| Tools   | ✅     | T35      | Toolmaker multi-input pipeline | AI walks dependency chain via Definitions (Tools → Stone → Wood) |
| Bootstrap | ✅     | T36      | Settlement starts autonomously | All bootstrap rules work together without conflicts |
| Full Autonomous | ✅ | T37 | Complete settlement runs without manual setup | Multiple economic circuits coexist and self-regulate |
| Soak 50k | ✅ | T38 | 50k tick stability | EconomyMetrics framework works at scale |
| Soak 100k | ✅ | T39 | 100k tick stability | Invariants hold past 50k |
| Soak 250k | ✅ | T40 | 250k tick stability | Invariants hold past 100k |
| Soak 500k | □ | T41 | 500k tick stability | Invariants hold past 250k |

### Implementation sequence per circuit

Each circuit follows this internal order:

```
1. Observation API          — ensure EconomySystem exposes what the decision rule needs
2. Decision Rule            — add to SettlementSystem (Observe → Decide → Publish)
3. Integration scenario     — encode the expected behaviour end-to-end
4. Regression              — scenario becomes specification, soak test validates stability
```

### Observation API as contract

Each circuit requires exactly the observations its rules need — no more. The API
surface is the contract between World and Settlement AI:

```
Circuit N → requires Observation API N → enables Rule N
```

New observations appear only when a scenario proves the existing API is insufficient.
No "API for future use".

### Decision rule types (in order of complexity)

| Type | Example | Required Observations |
|------|---------|----------------------|
| Bootstrap (v1) | "Build first Woodcutter" | `HasBuilding(type)` |
| Expansion | "Build second Sawmill when saturated" | `Flow`, `Potential`, `Available` |
| Chain balance | "Woodcutter:Sawmill ratio" | `GetBuildingCount`, `Flow` |
| Warehouse-aware | "Don't build when storage full" | `GetStockpileAmount`, `OutputBufferPressure` |
| Technology chain | "Need Tools → need Stone → need Wood" | `GetTotalProduced`, `GetAvailable` |

### Rule refactoring — future (not before 10+ rules)

When the number of decision rules exceeds ~10–15, extract as independent objects:

```
Observe();
for each rule: rule.Evaluate(snapshot);
Publish();
```

`ISettlementRule` would mirror `ISimulationSystem` — a common interface with
isolated logic, composed by SettlementSystem. Not needed at 3–5 rules.
When needed, it's a mechanical extraction, not an architectural redesign.

### Design constraint

**SettlementSystem never grows new subsystems.** All decisions are expressed through
existing APIs (`DemandManager`, `JobManager`). Transport, Production, Construction,
Warehouse, and Economy remain under their existing freeze invariants.

### Architecture evolves by accumulated instances

Throughout the project, abstractions follow the same path:

```
prove with simple code → accumulate instances → extract interface → freeze contract
```

Examples:
- `Simulation`: one `Tick()` → multiple `ISimulationSystem`
- `RenderGraph`: one pass → multiple `Pass`
- `SettlementSystem`: several `if` rules → `ISettlementRule` (future)

A new abstraction layer appears only after multiple concrete implementations
demonstrate the pattern. No premature generalisation: the observation layer
(`EconomySnapshot`) waits for 3+ domain consumers, `ISettlementRule` for 10+
decision rules.

### Observation layer — future consolidation

`EconomySystem`, `WarehouseSystem`, `ProductionSystem`, and `SettlementSystem`
each observe world state independently. Over time, a shared `EconomySnapshot` or
`WorldObservation` layer should centralise per-tick aggregation to eliminate
duplicate computation. Not a priority until 3+ circuits are operational.

### What this replaces
- %-based progress tracking (e.g. "Decision Rules — 15%")
- PR-number-based roadmap (PR7a, PR7b, PR8…)
- Subsystem-completion checklists (Production done ✅, Warehouse done ✅)

The only question that matters: "How many economic circuits does the simulation
complete autonomously?"

This question was answered at T37 — **7/7 circuits complete**. Settlement AI
derives its entire economy from Definitions, with zero hardcoded BuildingType
references in decision rules.

---

## Epoch 2 — Behavioral Validation (from T37 onward)

The question has changed. From:

> *"Can we build data-driven Settlement AI?"* ✅

To:

> *"Will this economy remain stable for 1,000,000 ticks
under the full Settlers II production chain?"*

### Core hypothesis of Epoch 2

**A data-driven economy built on bootstrap rules alone, without a central planner,
no recursive dependency solver, and no domain-specific heuristics, remains
deterministically stable as the production chain grows from 5 buildings to 30+,
and from 1M ticks to sustained operation.**

### Roadmap

| Phase | Focus | Key deliverables |
|-------|-------|-----------------|
| Phase 2 — Economic Validation | Stability proof | `EconomyMetrics`, soak 50k/100k/250k/500k, deadlock/oscillation detection, transport stability invariants |
| Phase 3 — Closed Loop Economy | First sustainable circuit | Forester tree regrowth → infinite Wood, no external resource dependency |
| Phase 4 — Long Production Chains | Food chain | Grain → Mill → Flour → Bakery → Bread via Definition Pattern; validation at scale |
| Phase 5 — Strategic Economy | Military & social | Coal, Iron, Gold, Beer, Weapons — each adds new gameplay rules, not just production |
| Debug Overlay (parallel) | Observability | `SettlementDebugOverlay` — ticks, buildings, resources, flow, demand, transport, rule firings, current goals |

### Priority order

```
1. Fix known regressions      — T25, T27 (tick 16 failures) ✅
2. EconomyMetrics framework   — struct, soak harness, invariants ✅
3. Soak at scale              — 50k ✅ → 100k ✅ → 250k ✅ → 500k □
4. Tree regrowth (Forester)   — closes the Wood→Tree→Wood loop
5. Food chain (Farm→Bread)    — first 4-step production chain
6. Debug Overlay              — parallel to Phases 4-5
7. Strategic economy          — Coal/Iron/Gold/Beer/Weapons
```

### Key invariants for Epoch 2

```
struct EconomyMetrics {
    float woodFlow;
    float plankFlow;
    float stoneFlow;
    float toolFlow;
    int idleWorkers;
    int queuedDemand;
    int activeBuildings;
    int transportTasks;
    int carriersBusy;
    int starvationCount;
    int deadlockCount;
};
```

Invariants to maintain across all soak durations:
- Resource flow never degrades to zero without external cause
- Queued demand is bounded (does not grow unbounded)
- Building count stabilises or grows predictably
- Transport does not enter persistent queue accumulation
- No deadlocks (zero `deadlockCount`)
- No starvations (zero `starvationCount`)

## See also
- `docs/ECONOMY_ARCHITECTURE.md` — architectural contract, freeze invariants
- `docs/LOGISTICS_ARCHITECTURE.md` (FROZEN v1.0) — logistics architecture
- `docs/PHASE7_IMPLEMENTATION_PLAN.md` — Phase 7 detailed plan

---

## Economy Core Freeze — Achieved

**The economic core is no longer under design.** Three relationship types (Renewable, Transformation, Consumption) are each handled by their own data-driven system. `ProductionSystem` contains zero special-case building checks — all behaviour comes from `Definition` tables.

**Invariant — New building checklist:**
No file outside `Definitions/`, `Core/` (enums), or `Testing/` may be modified when adding a new production building. Violation = freeze broken.

## Roadmap — Industry-driven (post-freeze)

The question has changed. From:

> *"How do we build the economic architecture?"* ✅

To:

> *"How do we implement the full Settlers II economy using the unchanging core?"*

### Phase A — Agriculture
```
Farm → Grain → Mill → Flour → Bakery → Bread → mine food (Bread = efficiency 3)
```
**Architectural criterion:** zero new systems, zero changes to Production/Consumption/Renewable. Definitions only.

### Phase B — Mining
```
CoalMine, IronMine, GoldMine — each uses existing ConsumptionSystem
```
**Architectural criterion:** only new Definition rows. ConsumptionDefinition already supports all three. ✅ T47 (Coal/Iron/Gold all produce simultaneously)

### Phase C — Metallurgy
```
IronOre + Coal → Smelter → IronBars → WeaponSmith → Weapons (IronBar + Coal)
```
**Architectural criterion:** first multi-input production (A + B → C) via ProductionDefinition only. Verifies ProductionSystem handles >1 required input without special cases — allDelivered gate, atomic consumption, DemandManager per-slot. No core changes. ✅ T48 (architectural: partial input → no output, both inputs → produces, inputs consumed after cycle)

### Phase D — Military
```
Weapons + Gold + Food → Barracks → Soldiers
```
**Architectural criterion:** single economic graph serving multiple independent subsystems (civilian + military).

### Key tests
- **T46+ Industry integration test** per sector (e.g., Farm→Mill→Bakery→Bread, Coal/Iron/GoldMine, multi-input WeaponSmith)
- **T48 — Multi-input architectural test**: partial input → no output, both inputs → produces, inputs consumed after cycle, no core changes
- **T49 — Economic Reachability Test** (static): every resource reachable from renewables, no orphan cycles, unique producer, no dangling resources
- **T50 — Economic Independence Test** (static): removing any industry degrades only transitive dependencies

## Development model
```
Before freeze: Architecture → Architecture → Architecture
After freeze:  Gameplay → Gameplay → Gameplay
```

New content is added by editing Definition tables and writing tests. Core systems are never touched.

## Architecture Epoch — Complete

All architectural invariants have been proven through integration tests and static analysis.
The Economy Core Freeze is verified across four independent dimensions:

| Dimension | Proof |
|-----------|-------|
| New industry (linear) | T46 Agriculture |
| New mine (consumption) | T47 GoldMine |
| Multi-input production | T48 WeaponSmith |
| Static graph integrity | T49 Reachability |
| Modular independence | T50 Independence |

**No changes to ProductionSystem, ConsumptionSystem, or RenewableResourceSystem**
**were required for any of T46–T50.**

## Current Focus — Simulation Development

The question has changed. From:

> *"Can we prove the architecture?"* ✅

To:

> *"How complex and interesting can we make the simulation?"*

Development now proceeds in three parallel tracks:

1. **Content** — new industries, buildings, resources (Definitions + tests)
2. **AI Behaviour** — decision quality, priorities, economic stability, strategy
3. **Balance** — production rates, costs, efficiencies, regeneration parameters

Architectural changes to core systems are exceptions, not the norm.

### Open items (non-architectural)

- Military industry chain (Weapons → Barracks → Soldiers)
- Food chain extensions (Grain → Mill → Flour → Bakery → Bread already done)
- Tree regrowth → closed Wood loop (Forester + Woodcutter)
- Production balance tuning
- AI development priorities
- Debug overlay for economic observability

## Future — after Complete Economic Graph

The project focus shifts from infrastructure to AI behaviour quality:
- Development priorities (which building first)
- Production chain optimisation (Hunter vs Fisher vs Bread)
- Mine selection (Coal, Iron, or Gold first)
- Worker distribution
- Inventory management
- Military strategy

**Metric changes:**
- Before: "How many architectural PRs completed?"
- Now: "How many economic industries completed?"
- Future: "How good is the simulation behaviour?"

The architecture is no longer a constraint. It has become a foundation.
