# Changelog

## Phase 2 — Local Transport Foundation ✅ (2026-07-05)

### Milestone: Production→Transport pipeline fully in SimulationCore

```
TransportNode      ← passive buffer + attachment registry (new)
ResourceBuffer     ← typed 8-slot storage (new)
LocalTransferSystem ← single owner of local distribution (new)
WarehouseSystem    ← migrated from outputBuffer → TransportNode
ScanProductionBuffers ← removed (replaced by ScanTransportBuffers)
```

### Key changes

- **TransportNode** introduced: buffer, attachments, pendingDemand. Passive storage with atomic operations. Does not evaluate deficits in the integrated pipeline.
- **ResourceBuffer** introduced: 8-slot fixed buffer with `Add/Remove/Has/Count/FindEmptySlot` contract.
- **LocalTransferSystem** introduced: Tick order = Export → Supply → Evaluate Deficit → outgoingCount.
- **WarehouseSystem migration**: from direct `ProductionBuilding::outputBuffer` access to `TransportNode.buffer` observation. `ScanProductionBuffers` renamed to `ScanTransportBuffers`. Dual-writer eliminated.
- **outputBuffer single-writer invariant restored**: only ConstructionSystem (init), ProductionSystem (fill), and LocalTransferSystem (drain) write to `outputBuffer`.
- **Tick order enforced**: LocalTransferSystem ticks before WarehouseSystem, so node buffer state is current at observation time.
- **T15/T17 passing**: Warehouse integration and 50k soak confirm production→warehouse pipeline through TransportNode.
- **Test coverage**: 164/164 unit tests pass. ResourceBuffer (15), TransportNode (22), LocalTransferSystem (20).

### Known temporary limitation

Until Carrier ownership is implemented (Milestone 3), `WarehouseSystem::HandleDeliveryEvents` performs an accounting `TransportNode.buffer.Remove()` as a stub bypass. `AcceptingFlagInventory` creates resources from nothing, breaking closed-form conservation.

---

## Cycle 2 — Unified Domain Model ✅ (tag: `architecture-cycle-2`)

### Milestone: Two parallel migration lines converged

```
Logistics:  EconomyManager → DemandManager → DemanTicket → Carrier → CargoManager
            (single source of truth for routing)

UI:         Game → MenuBuilder → MenuModel → RadialMenu → UiAction → Game
            (single source of truth for menu content)
```

### Completed migrations

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

### Architecture audit — Zero UI string literals in gameplay core (post UI4b)

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

---

## Phase 6b — ARCHIVED (Research Prototype)

Phase 6b (`kUseTransportJobs` experiment) successfully identified a fundamental architectural flaw:
> A logistics system needs **one TransportTask per shipment**, not one TransportJob per hop.
> Split responsibility between Job/Cargo/Carrier caused sourceFlag mismatch on intermediate hops.

All lessons and the new architecture are in `docs/LOGISTICS_ARCHITECTURE.md` (FROZEN v1.0).

---

## Phase 8 Post-Merge Debugging Session (2026-07-02)

### Problem

After merging `phase-7` into `main` (fast-forward, commit `436b0cd`), a dump.txt log showed:

1. **Builder doesn't visually come out**
2. **Building itself doesn't construct** — construction site scaffolding appeared but final building sprite never replaced it
3. **Worker doesn't go to work** — no Woodcutter worker spawned after building completed
4. **No tree cutting**
5. **`activeRequests=1` hangs forever** — EconomyManager's construction request count stuck at 1

### Root Cause — Two Bugs

**Bug 1: Premature site deletion in `ConstructionManager::Update()`**

`ConstructionManager::Update()` (`ConstructionManager.cpp:333`) called `RemoveSite(site)` immediately when `IsComplete() && builderState == Builder_None`. This deleted the completed construction site in Phase 1, before `ConstructionSystem::PostUpdate()` (Phase 7) could fire `Event_ConstructionComplete`.

**Fix**: Removed the `RemoveSite(site)` call. Completed site stays in the vector. `PostUpdate()` detects it, fires the event, and the normal cleanup chain runs.

**Bug 2: Wrong guard in `BuildingSystem::HandleConstructionComplete()`**

Originally:
```cpp
if (flag->hasBuilding) return;
```

`flag->hasBuilding` is set to `true` at flag creation time for non-free flags. So the guard **always triggered** for newly-built construction sites, preventing building creation, worker spawning, and tile layer setup.

**Fix**: Changed guard to `if (flag->building != NULL) return;`.

| Fix | Effect |
|-----|--------|
| Bug 1 fix | `PostUpdate` fires `Event_ConstructionComplete` |
| Bug 2 fix | `BuildingSystem` creates `Building`, calls `AddToLayer` (sprite appears), `SpawnWorker` (worker walks to building), `AddBuilding` (registers with economy) |
| Both | EconomyManager clears stale construction requests → `activeRequests` returns to 0 |

### Files Changed
- `Settlers2/World/ConstructionManager.cpp:333-337`
- `Settlers2/World/Systems/BuildingSystem.cpp:143`

### Debugging Maturity Signal

| Layer | Examples | Status |
|-------|----------|--------|
| **Architecture** | double ownership, DemandTicket vs TransportTask, carrier routing | ✅ Resolved |
| **Initialization** | Handle vs FlagId mismatch, two ConstructionManager instances, warehouseFlag wiring | ✅ Resolved |
| **Lifecycle** | object deleted before event consumer runs, state flag used for wrong semantic | ✅ Fixed |

This progression is a strong signal of project maturity — during large refactors, bugs disappear in exactly this order.

### Remaining pre-existing issues (not caused by merge)
- **OVERDELIVER telemetry** (`delivered=3/1`): harmless artifact of `SetDemand(woodMissing)` overwriting `requested` downward each frame
- **Carrier `sprite=0` when idle**: computed from road slope direction when `walkDir=0.0` — correct idle facing
- **Construction completion order**: `ConstructionManager::Update()` deletes completed sites before `PostUpdate()` fires `Event_ConstructionComplete`
- **Building placement**: cursor does not change after selecting building icon from build menu

---

## Build Stabilization Session (2026-07-02)

After committing Stage 7E4 (Building State Presentation), the project had 9+ build errors:

| # | Problem | File(s) |
|---|---------|---------|
| 1 | `class Scene` conflicts with `Scene` namespace | `Scene/Scene.h:16` |
| 2 | `const` mutating cached fields | `BuildingRenderer` |
| 3 | Deleted enum refs (`SettlerType`, `SettlerState`) | `SettlerPresentationSystem.cpp`, `SettlerRenderer.cpp` |
| 4 | Wrong enum names (`Building_Woodcutter` → `Woodcutter`) | `WorkerPass.cpp` |
| 5 | Wrong method name (`GetFlagCount()` → `GetCount()`) | `WorkerPresentationSystem.cpp` |
| 6 | Missing `stdafx.h` | `GroundResourcePresentationSystem.cpp` |
| 7 | Wrong namespace forward decl (`UIMenu`) | `ConfirmationMenuPresentationSystem.h` |
| 8 | `Graphics::SpriteAtlas` vs `::SpriteAtlas` forward decl mismatch | `BuildingRenderer`, `SettlerRenderer` |
| 9 | Missing `Building.h` include | `WorkerPass.cpp` |

### Key pattern: namespace hygiene for forward declarations

All 3rd-party types used in Scene/ headers must be forward-declared in their **real** namespace. Rule: verify the actual `class Foo { ... }` declaration location, not the folder path.
