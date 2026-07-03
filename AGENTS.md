# AGENTS.md — Architectural Invariants & Cross-References

## Transport v1 — Complete ✅ (tag: `transport-v1-complete`)

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
Controller owns all state transitions via `SetTaskState` (single point, `transitionCount` guard, max 64).
Dispatcher(`PickNextTask`) selects among waiting tasks; never touches route/hopIndex/targetFlag.
Carrier moves spatially toward `targetFlag`; never reads route, never modifies task.
Telemetry(`LogTelemetry`) scans state every 600 ticks, `assert oldestWaitingAge < 10000`.

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
TransportRequest
        │
        ▼  (coordinator routes)
TransportController
        │
        ▼  (inbound)
DeliveryEvent
        │
        ▼
ConstructionSystem
```

EconomySystem publishes `TransportRequest[]` to WorldModel;
Simulation reads and converts; TransportController executes.
Neither system knows the other's internals.

## SimulationCore — Adding New Systems

New `ISimulationSystem` implementations are plugged in via public `AddSystem()` — `Simulation.cpp` is never modified:

```cpp
sim.AddSystem(new EconomySystem());
sim.AddSystem(new ConstructionSystem());
```

```
Simulation::Tick():
    for each system: system.Tick(world)       → writes requests
    ProcessTransportRequests()                 → Convert to transport tasks
    Telemetry from WorldModel
    transport.Update(dt)                       → Execute movement
```

## Three Development Tracks

After Cycle 2 (PR18) and Simulation Validation (PR19), development splits into independent tracks:

**A. Simulation Engine** — new domain systems:
Worker AI, Production, Economy expansion, Save/Load, Building lifecycle.

**B. Definition Pattern** — data-driven definitions:
BuildingDefinition, ResourceDefinition, WorkerDefinition. Replace switch-based tables. Does not change simulation behavior.

**C. Simulation Validation** — scenarios + invariants:
`IScenario` interface, T1–T8 regression suite, categorized assertions (Transport, Construction, Economy, World), AI fuzzing.

These tracks are independent: Definition Pattern is tested by existing scenarios, new Engine systems add their own scenarios, Validation expands coverage without touching domain code.

## Build Config

- **Platform**: Xbox 360 (C++03, no variadic templates, `std::function`, auto, range-for)
- **SDK**: Not available for local builds — correctness by code review only

---

## Documentation Split

| File | Content |
|------|---------|
| `ARCHITECTURE.md` | Full architecture documentation (Transport Contract, Render Pipeline, Component Responsibility Map, Architecture Audit) |
| `ROADMAP.md` | Pipeline stages, Scene maturity, PR sequence, Cycle 2 (Domain Systems) plan |
| `CHANGELOG.md` | Cycle history, Phase 6b, Post-merge stabilization, Cycle 1 completion |
| `MIGRATION.md` | Phase 8 checklist, Current status, Stabilization checklist, Verification criteria |
| This file | Essential invariants, cross-references, build config |

## See also
- `docs/LOGISTICS_ARCHITECTURE.md` (FROZEN v1.0) — logistics architecture
- `docs/PHASE7_IMPLEMENTATION_PLAN.md` — Phase 7 detailed plan
