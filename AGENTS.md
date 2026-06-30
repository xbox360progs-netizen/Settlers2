# Architecture — Cycle 2 Complete ✅ (tag: `architecture-cycle-2`)

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
| Logistics (routing) | `DemandTicket` | `DemandTicket` |
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

Three rules verified after Logistic PR 1+2+3:

### Rule 1 — Planning
> **`EconomyManager` plans demand; `DemandManager` stores it. `Carrier` only executes tickets. `CargoManager` completes delivery.**

```
EconomyManager  →  DemandManager  →  Carrier  →  CargoManager
   creates demand      stores demand     executes     completes delivery
```

**Violation**: any code in `Carrier` that computes `targetFlag` or looks up a destination independently.

### Rule 2 — Ticket
> **Every moving `Cargo` owns exactly one `DemandTicket`.**
> `Cargo moving  ⇒  DemandTicket exists  ⇒  Destination exists`

**Violation**: a `Cargo` whose `currentFlag != targetFlag` but `ticket == NULL`.

### Rule 3 — Ownership
> **Each `DemandTicket` has at most one owner at any moment.**

```
Free → Reserve() → Owned by Cargo(handle) → Deliver() → Completed
                                           → Cancel() → Cancelled
```

**Violation**: two `Cargo` objects referencing the same live `DemandTicket`, or a delivered/cancelled ticket still referenced by moving `Cargo`.

### Architectural boundary (PR 3)
```
Production → ResourceSlot { destFlagId } → TakeCargoForRoad() → Cargo { DemandTicket } → Delivery
             (ownership tag)                                     (routing)
```

`DemandTicket` governs moving resources. `ResourceSlot::destFlagId` governs ownership of stationary resources awaiting pickup. **These responsibilities must never overlap.**

## Build Config
- **Platform**: Xbox 360 (C++03, no variadic templates, `std::function`, auto, range-for)
- **SDK**: Not available for local builds — correctness by code review only

---

# Stabilization Checklist (current iteration)

Run through the game to verify no regressions after the architecture cycle.

## Logistics
- [ ] Building receives all required resources
- [ ] Production buildings get input resources
- [ ] Warehouses collect only truly free resources
- [ ] Flag deletion leaves no orphaned resources
- [ ] No DemandTicket pool asserts triggered
- [ ] No DemandTicket leaks on map clear / return to menu

## Construction
- [ ] Open build menu
- [ ] Select any building
- [ ] Place building
- [ ] Wood delivery
- [ ] Stone delivery
- [ ] Construction completion

## UI
- [ ] All radial menus open
- [ ] All items display correctly
- [ ] All actions match expectations
- [ ] Editor layer selection works via UiAction

## Editor
- [ ] Layer switching
- [ ] Object placement
- [ ] Save/load (if present)

## Known Pre-existing Bugs (not caused by architecture cycle)
- **Construction completion order**: `ConstructionManager::Update()` deletes completed sites before `PostUpdate()` fires `Event_ConstructionComplete`
- **Building placement**: cursor does not change after selecting building icon from build menu (traced through menu selection → EnterBuildMode chain; cause not in PR 1)

---

# Next Steps

1. **Stabilization** — run checklist above
2. **UI6** — EditorScene migration to MenuModel / UiMessageId pattern (~268 string literals in EditorScene.cpp + TilePalette.cpp)
3. **Cycle 3: Definition Pattern** — `BuildingDefinition` table keyed by `BuildingType`, eliminating `GetBuildingTypeFromSpriteName` / `GetBuildingSpriteName` reverse lookups

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

## Boundary Rules for Future PRs
- Carrier never decides cargo destination — routing decision centralised in DemandManager
- PRs change one architectural aspect each; never combine cleanup with behavioural change
- GameRenderer remains read-only (no mutation of world state)
- UI widgets speak `UiAction`; `ICommandDispatcher` is single execution point
- Dead code removal is consequence, not goal; API surfaces unchanged in architectural PRs
