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

## Definition Pattern (Cycle 3)

Domain types are the only stable identifiers. UI assets, render assets, metadata and gameplay properties must be obtained from definition tables/services keyed by the domain type. Systems must not derive domain types from resource names.

**Permitted:** `BuildingType → sprite name`, `BuildingType → icon name`, `BuildingType → cost`
**Forbidden:** `sprite name → BuildingType`, `icon name → BuildingType`

## Boundary Rules for PRs

- Carrier never decides cargo destination
- PRs change one architectural aspect each; never combine cleanup with behavioural change
- GameRenderer remains read-only (no mutation of world state)
- UI widgets speak `UiAction`; `ICommandDispatcher` is single execution point
- Dead code removal is consequence, not goal
- All 3rd-party types in Scene/ headers must be forward-declared in their **real** namespace

## Build Config

- **Platform**: Xbox 360 (C++03, no variadic templates, `std::function`, auto, range-for)
- **SDK**: Not available for local builds — correctness by code review only

---

## Documentation Split

| File | Content |
|------|---------|
| `ARCHITECTURE.md` | Full architecture documentation (Transport Contract, Render Pipeline, Cycle 2, Component Responsibility Map, Architecture Audit) |
| `ROADMAP.md` | Pipeline stages, Scene maturity, PR sequence, Next Steps |
| `CHANGELOG.md` | Cycle history, Phase 6b, Post-merge debugging, Build stabilization |
| `MIGRATION.md` | Phase 8 checklist, Current status, Stabilization checklist, Verification criteria |
| This file | Essential invariants, cross-references, build config |

## See also
- `docs/LOGISTICS_ARCHITECTURE.md` (FROZEN v1.0) — logistics architecture
- `docs/PHASE7_IMPLEMENTATION_PLAN.md` — Phase 7 detailed plan
