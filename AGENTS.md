# UI Architecture — Cycle 1 Complete ✅

## Milestone: UI Layer Maturity

```
Gameplay → UiMessageId → NotificationManager/StatusManager → LocalizationService → UiFrameState → GameRenderer
publishes IDs                    stores IDs                    resolves text          DTO              reads only
```

## Completed

- **UI1** — `UiMessageId` enum + `LocalizationService` (2D table, `Get`/`Format`)
- **UI2** — `NotificationManager` (fixed pool, ID-based `Notify`, `FillFrameContext`)
- **UI3** — `StatusManager` (persistent status, decay timer, ID-based)
- **UI4a** — All 6 `UiEventSystem` event handlers migrated to `NotificationManager`; `GetResourceName()`/`GetBuildingName()` → `GetResourceNameId()`/`GetBuildingNameId()` (names are now IDs, not strings)
- **UI4b** — Dead confirm system fully removed: `IUiInputHost`, `ShowConfirm/ResolveConfirm`, `confirmActive/Title/Message` fields, GameRenderer confirm block (~100 lines), `"A = Yes     B = No"` literal eliminated
- **UI5a** — MenuScene refactored: `MenuModel { items[], selected, action }` + `UiAction` + `ICommandDispatcher`; `MenuScene` uses `MenuModel` + `LocalizationService`, zero user-facing string literals; `MenuCommandDispatcher` wires to `SceneManager`
- **UI5b** — GridMenu migrated to `MenuModel` + `UiAction`: `SetMenuItems()`, `GetSelectedAction()`, `LocalizationService`-based label resolution at render time; `SetupBuildMenu`/`SetupRoadMenu` use `MenuItem[]` with `UiMessageId` labels; `InputController` reads `GetSelectedAction().value`; backward compat kept for EditorScene (UI6)

## Current Layer Stack

```
EventBus.Post() → UiEventSystem::OnEvent (6 handlers)
                       ↓
                   NotificationManager::Notify(titleId, nameId, descId, duration)
                       ↓
                   FillFrameContext(UiFrameState&)
                       ↓
                   LocalizationService::Get(id) / Format(id, args, out, cap)
                       ↓
                   UiFrameState (char[32] resolved fields, POD)
                       ↓
                   GameRenderer::PushUiToQueue (read-only consumer)
```

## Component Responsibilities

| Component | Owns | Consumes |
|-----------|------|----------|
| `LocalizationService` | Translation table (enum → string) | `UiMessageId` |
| `NotificationManager` | Transient message pool (IDs + timers) | `UiMessageId` |
| `StatusManager` | Persistent status line (ID + timer) | `UiMessageId` |
| `UiController` | Frame assembly (NM + legacy merge) | — |
| `GameRenderer` | Rendering only | `FrameContext` (resolved `char[]`) |
| `UiEventSystem` | Legacy notification pool (dead) `|` EventBus events |

## Data Model: NotificationManager Slot

```
titleId   = category (MSG_TITLE_CONSTRUCTION_COMPLETE)
line1Id   = entity name (MSG_BUILDING_WOODCUTTER) or formatted text
descId    = description (MSG_CONSTRUCTION_COMPLETED) or MSG_NONE
count     = dedup accumulation (≥2 → "xN" in line2)
tileX/Y   = map position for location-tagged notifications
```

## Architecture Audit (after UI4b)

### `grep '"'` in `Scene/`, `UI/`, `World/Systems/`

**22 files** with string literals (~1117 total). Classification:

| Category | Where | Status |
|----------|-------|--------|
| **UI** (user-facing) | `LocalizationService.cpp` (~188 entries) | ✅ Centralized, clean |
| **Asset** (sprite/atlas names) | `GameRenderer.cpp`, `GameScene.cpp`, `RoadController.cpp`, `MenuBootstrap.cpp`, etc. | ✅ Not UI text |
| **Debug/log** | `SceneManager.cpp`, `InputController.cpp`, `WorldBootstrap.cpp`, `CarrierSystem.cpp` | ✅ Diagnostics only |
| **Editor/Menu UI** | `EditorScene.cpp`, `TilePalette.cpp` | ⏳ Not migrated (UI6) |

### Zero UI string literals in gameplay core

| File | Before UI cycle | After UI cycle |
|------|----------------|----------------|
| `InputController.cpp` | ~15 (raw status text) | **0** |
| `RoadController.cpp` | ~6 (raw road messages) | **0** |
| `GameScene.cpp` | ~5 (raw confirm/banner) | **0** |
| `GeologistController.cpp` | ~15 (raw geologist text) | **0** |
| `GameRenderer.cpp` | ~30 (confirm block + `"A=Yes B=No"`) | **0** |
| `UiEventSystem.cpp` | ~53 (6 handlers + 2 name switches) | **0** (legacy pool dead) |
| `NotificationManager.cpp` | — | Only `"x99+"`/`"x%d"` (format args) |

### `grep UiMessageId` — distribution pattern

| Directory | Role | Pattern |
|-----------|------|---------|
| `Scene/*.cpp` (6 files) | **Publishes IDs** | `StatusManager::SetStatus(MSG_*)`, `NotificationManager::Notify(MSG_*)` |
| `UI/*.cpp` (3 files) | **Resolves IDs → text** | `LocalizationService::Get(id)`, `m_loc->Format(id, args, ...)` |
| `Scene/GameRenderer.cpp` | **Consumes DTO** | `frame.ui.notifications[i].title` (resolved `char[]`) |

**Verified**: gameplay never calls `LocalizationService::Get()` — it publishes `UiMessageId` only.

## Dead Code Status

| Component | Status | Evidence |
|-----------|--------|----------|
| `UiEventSystem::ShowConfirm()` | Removed | 0 callers before removal |
| `UiEventSystem::ResolveConfirm()` | Removed | 0 callers before removal |
| `confirmActive/Title/Message` | Removed from DTO + renderer | `grep confirm` → only unrelated subsystems |
| `IUiInputHost` (interface) | Removed | `IsConfirmActive`, `OnConfirmYes/No` gone |
| `"A = Yes     B = No"` | Removed | Last literal outside LocalizationService |
| `UiEventSystem::ShowNotification()` pool | Dead (kept for compat) | All 6 handlers migrated to NotificationManager |
| `GetResourceName()` / `GetBuildingName()` | Removed | Replaced by `GetResourceNameId/BuildingNameId` |

---

# UI Cycle 2 — Menu Architecture

## Goal
Replace raw-string menu ownership with a `MenuModel`-based architecture:
`MenuItem { labelId, enabled, action }` → `MenuModel { items[], selected }` → resolve via `LocalizationService` at render time

## Motivation
- `MenuScene` currently owns: labels (raw strings), selection state, renderer coupling — God Object pattern
- After UI5, menu labels become `UiMessageId`, actions become typed enums, selection lives in `MenuModel`
- `GridMenu` (build menu) and `RadialMenu` follow the same model
- No string literals, no callbacks-as-strings, no UI commands in menu code

## Planned (in order)

### UI5a — MenuModel + MenuScene
- `MenuItem` struct: `labelId (UiMessageId)`, `enabled (bool)`, `action (enum)`
- `MenuModel`: items array, selected index
- `MenuScene` refactored to use MenuModel; raw string labels → UiMessageId
- Menu state flows through FrameContext (resolved `char[]` DTO)
- `MenuRenderer` reads from DTO only

### UI5b — BuildMenu (GridMenu) ✅
- Convert `GridMenu` to MenuModel + UiAction contract
- Building labels → UiMessageId (via `GetBuildingMessageIdFromSpriteName`)
- Grid navigation (4-direction stick) stays in GridMenu, item data lives in MenuModel
- `ICommandDispatcher` unchanged; caller reads `GetSelectedAction().value`

### UI5c — RadialMenu
- Convert to MenuModel
- Action labels + icon bindings → UiMessageId
- Angular geometry stays in renderer, item data stays in model

### UI6 — EditorScene
- Migrate Editor UI to MenuModel / UiMessageId pattern
- Editor is lower priority — complex debug flows, tools, special cases

## Deferred
- **Stage I (SimulationSystem extraction)** — backlog. Core is healthy after H1–H3; UI infrastructure is newer and benefits more from immediate use.

## Transport Contract (Architectural Invariants)

After Logistic PR 1, these three rules constitute the specification of the resource routing subsystem. PR 1 must satisfy all three. Any future change to logistics is verified against them.

### Rule 1 — Planning

> **`EconomyManager` plans demand; `DemandManager` stores it. `Carrier` only executes tickets. `CargoManager` completes delivery.**

```
EconomyManager  →  DemandManager  →  Carrier  →  CargoManager
   creates demand      stores demand     executes     completes delivery
```

- `EconomyManager` creates demand (Phase 2 — `SetDemand(type, amount, targetFlag, priority)`)
- `DemandManager` stores and tracks demand; `Reserve()` assigns tickets; owns cancellation
- `Carrier` receives a ticket and follows it; never computes a route or destination
- `CargoManager` completes delivery when cargo reaches ticket target flag

**Violation**: any code in `Carrier` that computes `targetFlag` or looks up a destination independently.

### Rule 2 — Ticket

> **Every moving `Cargo` owns exactly one `DemandTicket`.**

```
Cargo moving  ⇒  DemandTicket exists  ⇒  Destination exists
```

This means:
- A cargo without a ticket cannot begin movement.
- A moving cargo always has a single, unambiguous destination.
- Cancellation is `DemandManager` releasing the ticket, not state mutation on `Carrier`.
- Re-routing is `DemandManager` cancelling + re-issuing, not the carrier changing its mind.

**Violation**: a `Cargo` whose `currentFlag != targetFlag` but `ticket == NULL`.

### Rule 3 — Ownership

> **Each `DemandTicket` has at most one owner at any moment.**

```
Free
  ↓ Reserve()
Owned by Cargo(handle)
  ↓ Deliver()
Completed
```
or
```
Owned by Cargo(handle)
  ↓ Cancel()
Cancelled
```

"At most one" is the invariant — the exact states may evolve (e.g. `AssignedToCarrier`, `RetryPending`), but the ownership guarantee must hold.

**Violation**: two `Cargo` objects referencing the same live `DemandTicket`, or a `Delivered`/`Cancelled` ticket still referenced by a moving `Cargo`.

---

## Logistic PR 3 — ResourceSlot Ownership Semantics (Complete ✅)

### Result
`destFlagId` is **not** dead routing code. It is an **ownership/guarantee tag** for stationary `ResourceSlot` entries, orthogonal to `DemandTicket` routing.

### Architectural boundary discovered

```
Production
      ↓
ResourceSlot { destFlagId }     ← ownership tag: who may claim first
      ↓
TakeCargoForRoad()
      ↓
──────────────────────────────── ← state transition (stationary → moving)
      ↓
Cargo { DemandTicket }          ← routing: where to go
      ↓
Delivery
```

`DemandTicket` governs moving resources. `ResourceSlot::destFlagId` governs ownership of stationary resources awaiting pickup. **These responsibilities must never overlap.**

### Three roles of destFlagId (none is routing)

1. **Ownership tag** (EconomyManager Phase 1a, Phase 6) — marks a ResourceSlot as "spoken for" before a carrier converts it to Cargo. Fills the gap between `AddResource` and `TakeCargoForRoad`.

2. **Collision guards** (Warehouse::Update, ConstructionManager::Update, CollectWarehouse) — prevents subsystems from consuming resources tagged for other consumers. Three independent guards, same invariant: *a tagged resource must not be consumed by anyone other than its intended recipient.*

3. **Orphan reroute** (EconomyManager Phase 7) — detects resources whose destination flag was deleted/unreachable. Not covered by DemandManager (which only handles Cargo with tickets, not orphaned ResourceSlots).

### What was removed
- `ResourceDeliveredData::destFlagId` — completely dead: event `Event_ResourceDelivered` is never posted, and the handler (`UiEventSystem::OnResourceDelivered`) only reads `resourceType`.

### What was NOT removed
- `ResourceSlot::destFlagId` — carries independent ownership responsibility at all 30+ live usage points.

### Key invariant (new)
> **`DemandTicket` governs moving resources. `ResourceSlot::destFlagId` governs ownership of stationary resources awaiting pickup. These responsibilities must never overlap.**

---

## Build Config

- **Platform**: Xbox 360 (C++03, no variadic templates, `std::function`, auto, range-for)
- **SDK**: Not available for local builds — correctness by code review only
