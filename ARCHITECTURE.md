# Architecture

## Domain First

Domain types and entities are the only stable source of truth. Rendering, UI, localization, metadata and serialization are projections of the domain model and must never become the primary source of game state.

```
Gameplay → UiMessageId → NotificationManager/StatusManager → LocalizationService → UiFrameState → GameRenderer
publishes IDs                    stores IDs                    resolves text          DTO              reads only
```

### Component Responsibility Map

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

---

## Transport v2 — Architecture

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

Route planner (`FindPath`) builds an immutable route at `CreateTask` time.
No code changes route after creation (except `RetryBlockedTasks` rebuilds it).
Controller owns all state transitions via `SetTaskState` (single point, `transitionCount` guard).
Dispatcher (`PickNextTask`) selects among waiting tasks; never touches route/hopIndex/targetFlag.
Carrier moves spatially toward `targetFlag`; never reads route, never modifies task.
Telemetry (`LogTelemetry`) scans state every 600 ticks, `assert oldestWaitingAge < 10000`.

### Key Decisions

1. **Route = immutable execution plan** — built once at `CreateTask`, never mutated.
2. **One task = one physical unit** — no `amount` field, no batching at task level.
3. **Event-driven lifecycle** — no per-frame scanning, all state transitions from `Notify*` callbacks.
4. **Carrier is a dumb executor** — knows only `targetFlag`, never `route[]` or task state.
5. **Age-based anti-starvation** — `ageBonus = min(tick - createdTick, 200)` computed on selection.
6. **Telemetry = passive observation** — `LogTelemetry()` never modifies state.
7. **`transitionCount < 64`** — catches infinite state loops.
8. **`Update()` is NOT a decision loop** — exists only for telemetry (`LogTelemetry` every 600 ticks) and monotonic tick increment (`m_currentTick++` for age bonus). Assignment, route mutation, retries, and state transitions remain event-driven from `Notify*` callbacks only.

---

## Transport Contract

### Rule 1 — TransportTask owns the shipment lifecycle

> **TransportTask owns lifecycle. No other object may create, route, deliver or cancel a shipment.**
> Economy observes shipment completion through `observerTicketId` and never participates in transport execution.

```
CreateTask → Assign → PickUp → AdvanceHop → Delivered → DemandManager::Deliver()
                                                      → Cancel → DemandManager::CancelTicket()
```

**Violation**: any code outside `TransportController::CreateTask`, `SetTaskState`, or `DeliverTask` that modifies task state or completes/cancels a shipment without going through the Controller.

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

**Violation**: modifying a shipment's route, state, or destination from anywhere other than `TransportController` member functions.

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

**Violation**: a `Cargo` containing a `DemandTicket*`, or any transport code calling `DemandManager::Reserve()`/`ReleaseTicket()` directly.

### Acceptable transitional state — `Ground|Task`

After `CreateTask()` and before `PickUp()`, a resource is physically on the ground/flag but logically owned by a TransportTask. The telemetry mask `Ground|Task` is **correct** — ownership precedes physical pickup.

### Architectural boundaries

#### Route vs Dispatch
> **Route planner decides WHERE cargo moves.**
> **Priority dispatcher decides WHEN cargo moves.**
> The dispatcher (`PickNextTask`) selects among waiting tasks but never modifies route, hopIndex, or targetFlag. These responsibilities must never be mixed.

#### DemandManager vs Transport
> **DemandManager requests movement (exclusive publisher). Transport performs movement.**
> Transport never inspects reason, owner, or domain origin — only `task.basePriority`.
> DemandManager receives delivery notification through `observerTicketId`.
> EconomySystem observes passively — never requests movement.

---

## Render Pipeline — Invariants

### Seven rules

```
1. Simulation  →  never knows about Rendering.
2. Presentation  →  never knows about Graphics API.
3. Projection  →  the only place world becomes screen.
4. swap()  →  the only frame publication boundary.
5. After swap()  →  RenderFrame is immutable.
6. RenderGraph  →  the sole owner of pass execution order.
7. Pass  →  only receives RenderFrame, RenderContext, CommandBuffer.
```

### Derived invariants

```
Presentation reads Simulation, writes RenderFrame.
After swap(), only RenderGraph reads RenderFrame.
No Pass may reference a Simulation Manager, Controller, Map, or FrameContext.
Infrastructure services (TextRenderer, FontService) are permitted in Pass.
```

### Projection invariant

```
Projection is deterministic.
Given the same RenderFrame + Camera snapshot → bit-identical projected frame.
No dependency on time, GPU state, or pass order.
```

### Core assignment (Xbox 360 target)

```
Core2      AI
Core1      Simulation  →  Presentation  →  Projection  →  Publish(RenderFrame)
Core0      Acquire(RenderFrame)  →  RenderGraph  →  CommandBuffer  →  GPU
```

The `swap()` call is the sole publish point. Everything before it is Core1 work; everything after it is Core0 work. No shared mutable state crosses this boundary.

---

## RenderFrame Pipeline — Core0/Core1 Contract

### Layer Stack

```
Simulation state   →   Presentation   →   RenderFrame   →   Renderer
(position/tile)        (world coords,      (POD DTOs,        (sprite indices,
                        depth,             no pointers,       atlas lookups,
                        direction)         no sim deps)       render commands)
```

### Key Decisions

1. **Renderer reads `RenderFrame`, never simulation** — zero simulation manager access in render path.
2. **Double-buffer swap** (`RenderFrame next; Build(next); m_renderFrame.swap(next)`) — prepares for Core0/Core1 split where Core1 writes into `frames[back]` and Core0 reads `frames[front]`.
3. **No sprite logic in Presentation** — `SettlerVisual` stores only enum values (type, state, dx/dy). `ResolveSpriteIndex()` lives in SettlerRenderer.
4. **Depth pre-computed in Presentation** — `RenderTransform::depthLayer` stores the final draw order value (e.g., `30020 + tileY * 400`). Renderer casts to WORD directly.

### Data Structures

```
Scene/Shared/
├── RenderTransform.h    worldX, worldY, depthLayer
├── SettlerVisual.h      type, state, dx, dy, carrying, cargoType, buildingType
└── RenderFrame.h        frameId, simulationTick, vector<RenderSettler>

Scene/Settlers/
├── RenderSettler.h      RenderTransform + SettlerVisual
├── SettlerRenderer.h/.cpp     pure render (no simulation includes)
└── SettlerPresentationSystem.h/.cpp  pure presentation (no graphics includes)
```

### RenderFrame lifecycle invariant

```
Presentation
    ↓  BuildRenderFrame(next)
Projection(next)
    ↓  swap()
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
No code modifies RenderFrame
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    ↓
RenderGraph.Execute(frame, context, buffer)
```

`RenderFrame` is **immutable after swap()**. This single invariant eliminates the entire class of Core0/Core1 races and makes the transition to parallel threading a linear-ownership problem, not a mutex problem.

After `swap()`:
- Presentation + Projection own the **next** (back-buffer) frame.
- RenderGraph reads the **current** (front-buffer) frame.
- No atomics, no locks, no dirty flags — just data generation followed by data consumption.

### GameRenderer invariant

```
Forbidden:
  GameRenderer → Controller  (no m_roadController, m_placement is transitional)
  GameRenderer → Manager     (no m_roadManager)

Permitted:
  RenderGraph           (m_renderGraph)
  RenderContext         (future — Stage 8)
  CommandBuffer         (m_commandBuffer — sole output path)
```

### RenderPass invariant

```
Forbidden:
  RenderPass → FrameContext
  RenderPass → Map
  RenderPass → Controller
  RenderPass → Simulation Manager   (FlagManager, EconomyManager, etc.)

Permitted:
  RenderFrame
  RenderContext
  CommandBuffer
  *Renderer / *FontService         (infrastructure, not game managers)
```

Pass input is strictly bounded to `(frame, context, buffer)`.

---

## Definition Pattern

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

---

## Visual Completeness

**RenderFrame is the sole visual representation of Simulation.**

Every gameplay-visible state transition must be observable through RenderFrame. Renderer never queries Simulation Managers or Controllers.

If a gameplay event cannot be verified visually through RenderFrame, the rendering pipeline is incomplete.

```
Simulation
    ↓
Presentation (transforms state → DTOs)
    ↓
RenderFrame (immutable snapshot)
    ↓
RenderGraph (Passes → CommandBuffer)
    ↓
GPU
```

**Violation**: any `#include` of a Simulation Manager header in a Renderer, or any direct call to `FlagManager/RoadManager/Map` from a RenderPass.

---

## Platform Milestones v1

Four stable subsystems form the simulation platform. Dependencies flow in one direction:
domain systems → DemandManager → Transport → DeliveryEvents → domain systems.
EconomySystem observes ProductionBuilding::totalOutput — never writes.

### Transport v2 — Stable

Responsibility: Move resources between flags via carrier lifecycle. Knows nothing about domains,
reasons, or owners.

```
Public contract:
  CreateTask(resource, origin, destination, reason)  →  TransportTask*
  SetTaskState(taskId, newState)                      →  state transition (single point)
  PickNextTask()                                      →  dispatcher (priority+age, FIFO)
  DeliverTask(taskId)                                 →  completion + DeliveryEvent
  Update(dt)                                          →  carrier movement
```

Invariants:
- `score = basePriority + min(tick - createdTick, 200)` — dispatcher never inspects reason/owner.
- `enqueueOrder` is the deterministic FIFO tiebreaker for equal scores.
- `transitionCount < 64` catches infinite loops.
- Carrier reads only `targetFlag` — never reads `route[]` or task state.

Clients: DemandManager (via `CreateTask`), SimpleTransportDriver (via `Tick`), Telemetry.

Change rule: Requires a failing integration or soak test.

### Production v1 — Stable

Responsibility: Convert input resources to output resources on a cycle timer.
Output accumulates in `outputBuffer[]`. ProductionSystem never publishes transport requests
for completed output — that is the responsibility of external consumers (WarehouseSystem, etc.).

```
Public contract:
  ProductionBuilding::totalOutput[p]      — monotonically increasing
  ProductionBuilding::outputBuffer[p]     — finished goods pending collection
  ProcessProduction()                     — cycle logic, input gate
```

Invariants:
- `totalOutput` is the canonical source for output tracking (read by EconomySystem).
- `outputBuffer` is written only by ProductionSystem, read by WarehouseSystem.
- Cycle consumes inputs atomically — `inputDelivered` resets simultaneously.
- `inputsRequested` guard prevents duplicate `SetDemand` per cycle.

Clients: EconomySystem (reads `totalOutput`), WarehouseSystem (reads `outputBuffer`).

Change rule: Requires a failing integration or soak test.

### Economy v1 — Stable

Responsibility: Observe and aggregate resource flow metrics. Never drives gameplay.

```
Public contract:
  GetTotalProduced(type)         — cumulative from productionBuilding.totalOutput[]
  GetTotalConsumed(type)         — derived from ProductionDefinition (e.g. 2 Wood per Plank)
```

Invariants:
- `ProductionBuilding::totalOutput` is the canonical source — EconomySystem computes deltas.
- Consumption is derived from ProductionDefinition, never from direct counters.
- Every new building type (ProductionDefinition entry) automatically extends coverage.
- EconomySystem never mutates ProductionBuilding, Transport, or DemandManager.

Clients: Telemetry, tests, future UI (resource flow display).

Change rule: If EconomySystem is removed, gameplay is unaffected — only telemetry is lost.

### Warehouse v1 — Stable

Responsibility: Store finished goods and create transport demand for them via DemandManager.

```
Public contract:
  GetStockpileAmount(type)       — current stockpile in warehouse
  GetStockpileCount()            — number of distinct resource types tracked
```

Flow:
```
Production → outputBuffer → WarehouseSystem (SetDemand) → Transport → Warehouse inventory
```

Invariants:
- WarehouseSystem monitors `outputBuffer`, never writes it — Production is the sole writer.
- Warehouse decrements `outputBuffer` on delivery receipt, one unit at a time.
- `Stockpile = delivered - consumed` — verified monotonic in soak.
- Transport never knows about Warehouse — `TTR_WarehouseBalance` routed through existing Dispatcher.
- Zero changes to Production, Transport, or Economy.

Clients: EconomySystem (future), Settlement AI (future), Market (future).

Change rule: Requires a failing integration or soak test.

### Dependency graph

```
                Domain Systems
                      │
        ┌─────────────┼─────────────┐
        │             │             │
 Production      Warehouse     Construction
        │             │             │
        └──────► DemandManager ◄────┘
                      │
               Transport v2
                      │
               DeliveryEvents
                      │
        ┌─────────────┴─────────────┐
        │                           │
   Production                 Warehouse
        │
   totalOutput
        │
   Economy (read-only)
```

No subsystem below the line knows about any subsystem above the line.

---

## Platform Architectural Patterns

### Pattern 1 — Intent → Manager → Executor → Event

```
Domain Intent
       │
       ▼
    Manager
       │
       ▼
   Executor
       │
       ▼
Immutable Event
       │
       ▼
0..N Domain Subscribers
```

Independently validated across two subsystems:

| Intent | Manager | Executor | Event |
|--------|---------|----------|-------|
| Demand | DemandManager | TransportController | DeliveryEvent |
| Job | JobManager | WorkerSystem | JobEvent |

#### Invariants

1. **Domain publishes intent, does not execute it.**
   `ConstructionSystem → CreateJob()`, `ProductionSystem → SetDemand()`.
   Neither invokes WorkerSystem or TransportController directly.

2. **Manager owns the lifecycle of the intent.**
   `JobManager` owns Job state/ownership. `DemandManager` owns Demand state/fulfillment.
   Neither delegates lifecycle to the executor.

3. **Executor executes intent, does not know its business meaning.**
   `Carrier` knows only `targetFlag`, never `TransportTaskReason`.
   `WorkerSystem` knows only `Job::duration`, never `JobType` semantics.
   There is no `switch(job.type)` in WorkerSystem, no `switch(task.reason)` in Carrier.

4. **Executor publishes an immutable event, does not interpret it.**
   `WorkerSystem::CaptureJobEvents()` publishes `JobEvent` to WorldModel.
   `SimpleTransportDriver::CaptureDeliveryEvents()` publishes `DeliveryEvent`.
   Neither executor reads or reacts to events it published.

5. **Any number of domain systems may subscribe to the event.**
   Construction, Economy, Achievement, Telemetry — all receive the same event.
   Adding a new subscriber requires zero changes to the executor.

6. **Adding a new subscriber does not require changes to the executor.**
   The executor only publishes events. The WorldModel is the distribution point.

#### Verification (cross-system)

- T18–T19: WorkerSystem acquires/releases jobs through JobManager, never modifies them.
- T20–T21: WorkerSystem completes jobs through `JobManager::CompleteJob()`, captures `JobEvent`.
- T8–T14: TransportController delivers through DemandManager, captures `DeliveryEvent`.
- Both pipelines use identical event lifecycle: publish at end of Tick N, consume at start of Tick N+1,
  `Clear*Events()` after all consumers.

#### Pattern recognition criterion

A pattern becomes a platform primitive only after meeting all of the following:

```
1. One implementation   →  a design decision (might be specific to the subsystem)
2. Two independent      →  a candidate pattern (reproduced without changing the first)
   implementations
3. Reproduced without   →  a platform primitive (the contract is stable enough
   changing the contract     that new domains conform to it, not the other way around)
```

Intent → Manager → Executor → Event crossed from step 2 to step 3 when WorkerSystem
adopted the same lifecycle as TransportController without modifying DeliveryEvent,
ClearDeliveryEvents, or the Simulation tick order. The contract proved stable under
a second, independent implementation.

This criterion prevents premature abstraction: a single successful subsystem is not
sufficient to document a platform pattern. The pattern must survive at least one
independent reproduction.

#### Next epoch — Platform validation

The next development epoch (Settlement AI, resource allocation, trade) has a specific
architectural goal — not "add Settlement AI", but:

> Implement Settlement AI without modifying any existing Platform Architectural Pattern.

Success criteria:
- DemandManager unchanged
- JobManager unchanged
- TransportController unchanged
- WorkerSystem unchanged
- Intent → Manager → Executor → Event unchanged
- Settlement AI publishes only Demand and Job; subscribes only to DeliveryEvent and JobEvent

If Settlement AI can be built entirely within these constraints, the platform is not
merely well-designed — it is capable of supporting game evolution without rebuilding
its own foundation. This is the characteristic that distinguishes a long-lived simulation
platform from a collection of coupled systems.

---

## Settlement AI — Contract

### Responsibility

Settlement AI decides what to do next. It never executes decisions itself.

### Permitted operations (exhaustive)

1. **Publish Job** — via `JobManager::CreateJob()`
   (e.g. Build Sawmill, Repair Building, Harvest Forest, Explore Area)
2. **Publish Demand** — via `DemandManager::SetDemand()`
   (e.g. Need Wood, Need Stone, Rebalance Warehouse, Emergency Supply)
3. **Read world state** — input for decision-making only:
   - `EconomySystem::GetTotalProduced()`, `GetTotalConsumed()`
   - `WarehouseSystem::GetStockpileAmount()`
   - `ProductionBuilding::outputBuffer[]`, `totalOutput[]`
   - `JobEvent[]`, `DeliveryEvent[]`

### Prohibited operations (invariants)

Settlement AI must not:
- Create `TransportTask`
- Assign a `Worker` to a `Job`
- Change `Job` state or ownership
- Change `Demand` state or fulfillment
- Write to `ProductionBuilding`
- Write to `WarehouseSystem` stockpile
- Change `Carrier` state
- Change `Worker` state

In one sentence: **Settlement AI never executes decisions. It only publishes intent.**

### Data flow

```
            Settlement AI
                  │
      ┌───────────┴───────────┐
      │                       │
Publish Job             Publish Demand
      │                       │
      ▼                       ▼
 JobManager            DemandManager
      │                       │
      ▼                       ▼
 WorkerSystem        TransportController
      │                       │
      ▼                       ▼
 JobEvent            DeliveryEvent
      └───────────┬───────────┘
                  ▼
            Settlement AI
```

Settlement AI is another event subscriber, not a central coordinator.
It manages the flow of intent, not the executors themselves.

### Architecture Audit (PR 0) — Existing integration points

#### Existing Job publishers

No domain system currently publishes Jobs. `JobManager::CreateJob()` is called only from tests (T18–T21).
Settlement AI will be the first system to use JobManager in production.

#### Existing Demand publishers

| System | Method | Reason | Owner |
|--------|--------|--------|-------|
| `ConstructionSystem` | `SetDemand(resource, need, flag, TBP_Normal)` | `TTR_Construction` (default) | `DemandOwner_Construction` (default) |
| `ProductionSystem` | `SetDemand(resource, need, flag, TBP_Normal, DemandOwner_Production, TTR_Production)` | `TTR_Production` | `DemandOwner_Production` |
| `WarehouseSystem` | `SetDemand(resource, need, flag, priority, ...)` | `TTR_WarehouseBalance` | — |

DemandManager is the exclusive publisher of `TransportRequest[]` into WorldModel.
All three systems use it, Settlement AI will be the fourth.

#### Existing Events

**DeliveryEvent** (WorldModel, max 64 per tick):
```
type        = DET_Completed
resource    = ResourceType
amount      = int (always 1)
destinationFlag = FlagId
reason      = TransportTaskReason
```

**JobEvent** (WorldModel, max 64 per tick):
```
type    = JET_Completed
jobId   = JobId
jobType = JobType
worker  = WorkerId
```

Both follow identical lifecycle: published at end of Tick N via `Capture*Events()`,
consumed at start of Tick N+1, cleared via `Clear*Events()`.

#### Existing Observables

| API | Returns | Source |
|-----|---------|--------|
| `EconomySystem::GetTotalProduced(type)` | `int` | cumulative from `totalOutput[]` |
| `EconomySystem::GetTotalConsumed(type)` | `int` | derived from `ProductionDefinition` |
| `WarehouseSystem::GetStockpileAmount(type)` | `int` | internal stockpile |
| `WarehouseSystem::GetStockpileCount()` | `int` | distinct resource types |
| `JobManager::GetWaitingJobCount()` | `int` | jobs not yet assigned |
| `JobManager::GetAssignedJobCount()` | `int` | jobs in progress |
| `JobManager::GetCompletedJobCount()` | `int` | finished jobs |
| `SimulationState::activeTransportTasks` | `int` | in-flight tasks |
| `SimulationState::economyPendingRequests` | `uint32` | unfulfilled demands |
| `WorldModel::pendingRequestCount` | `int` | transport requests |
| `WorldModel::productionBuildings[]` | array | per-building outputBuffer, totalOutput, type |
| `WorldModel::deliveryEvents[]` | array | last tick's deliveries |
| `WorldModel::jobEvents[]` | array | last tick's completed jobs |
| `WorldModel::workers[]` | array | worker states |
| `WorldModel::workerCount` | `int` | number of workers |

#### What Settlement AI needs (not yet exposed)

- **List of active Jobs by type** — `JobManager` does not expose `GetJobsByType(JobType)`.
  Settlement will need to iterate `GetJobCount()` and read each `GetJob(i)`.
- **Per-job status** — `GetJob(id)` takes index, not JobId. For now, iterating by index is sufficient.
- **Building type queries** — `ConstructionSystem` does not expose "what is being built where."
  Settlement would need to scan `WorldModel::activeSites[]` directly.
- **Worker availability** — `WorkerSystem` does not expose "idle worker count."
  Settlement can scan `WorldModel::workers[]` states directly.

None of these require new infrastructure — they are direct reads of existing WorldModel state.
Settlement AI will use the same data that tests and telemetry already read.

### Settlement v1 — Decision Loop (PR 1 target)

```
Observe → Decide → Publish Intent → Wait for Events → Observe
```

Concrete example:
- Read world state
- If no Woodcutter exists → publish Job for Woodcutter construction
- If Wood stockpile is low → publish Demand for Wood
- Wait for JobEvent / DeliveryEvent
- On event → re-evaluate

No scheduler, no strategies, no development priorities. Settlement AI v1 only
proves the cycle exists and stays within the permitted operations.

### Architectural role

Each platform subsystem answers one fundamental question:

| Subsystem | Question |
|-----------|----------|
| Transport | How to move? |
| Worker | How to execute work? |
| Production | How to convert resources? |
| Warehouse | Where to store? |
| Economy | How to measure? |
| Settlement AI | **What to do next?** |

Settlement AI is the first system whose answer must be expressed entirely through
existing platform primitives (Job, Demand, Event) — without new infrastructure.
If it succeeds, it validates the platform's architectural closure.

After Settlement AI, a further criterion becomes testable:

> **Platform Closure Criterion:** The platform is architecturally closed if two
> consecutive major domain systems were implemented using only existing Platform
> Architectural Patterns, without introducing new platform primitives.

If both Settlement AI and a subsequent system (e.g. Market/Trading) satisfy this,
the architectural vocabulary of the project is stable — all future game development
proceeds through composition of known primitives, not invention of new fundamental
mechanisms.

---

## Key field-level invariants

```
Flag::hasBuilding  == reservation / planned building
Flag::building != NULL == instantiated building object
```

**Never use `hasBuilding` as a check for object existence.** A flag with a planned construction site has `hasBuilding=true` but `building=NULL`. Use `flag->building != NULL` when you need to know whether a `Building` object actually exists.
