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

## Transport v1 — Architecture

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

#### Economy vs Transport
> **Economy requests movement. Transport performs movement.**
> Economy never moves resources directly.
> DemandTicket lives at the boundary as an adapter — it connects DemandManager's request to TransportTask's lifecycle, then drops out of the model.

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

## Key field-level invariants

```
Flag::hasBuilding  == reservation / planned building
Flag::building != NULL == instantiated building object
```

**Never use `hasBuilding` as a check for object existence.** A flag with a planned construction site has `hasBuilding=true` but `building=NULL`. Use `flag->building != NULL` when you need to know whether a `Building` object actually exists.
