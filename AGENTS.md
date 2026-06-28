## Refactoring Plan: GameScene God-Object → Decoupled Systems

### Status
- **Stage A** (WorldRestorer + MenuBootstrap) — ✅ Done
- **Stage B** (WorldBootstrap) — ✅ Done
- **Stage C** (Deferred deletion) — ✅ Done
- **Stage D** (Extract InputController) — ✅ Done
- **Stage E** (Extract GameRenderer) — ✅ Done
- **Stage F** — ⏳ Pending

#### Stage E3: Extract render methods → GameRenderer (this session) ✅
- Created `Scene/GameRenderer.h` — `GameRendererState` struct (geologist state, banner state) + `GameRenderer` class with `Render`, `RenderCursor`, `RenderGeologistOverlay`, `PushUiToQueue`
- Created `Scene/GameRenderer.cpp` — moved all render logic from GameScene, delegates to GameRendererState via `m_state->`
- Updated `GameScene.h` — added `GameRendererState m_renderState`, `GameRenderer* m_gameRenderer`, removed old render-related private members (`m_geologistState`, `m_geologistTileX/Y`, `m_bannerState`, `m_bannerTargetX`)
- Updated `GameScene.cpp` — `Render()` now delegates to `m_gameRenderer->Render()`, removed ~2100 lines of orphaned render code (`RenderCursor`, `RenderGeologistOverlay`, `PushUiToQueue` bodies)
- Removed orphaned `// ─── Cursor & Interaction` section that had been accidentally duplicated
- Added `Scene\GameRenderer.h` / `Scene\GameRenderer.cpp` to vcxproj
- Reduced `GameScene.cpp` from ~3660 → **1563 lines**

#### Stage E2: Extract InputController ✅ (previous session)
- Created `Scene/InputController.h/cpp` with mouse/keyboard/gamepad input handling
- Removed ~250 lines of input code from GameScene

#### Stage E1: Extract initialization mutations → WorldBootstrap ✅ (previous session)
- `WorldBootstrap::CreateStartingHQ()` — moved HQ flag+warehouse creation, carrier wiring, demand setup
- `WorldBootstrap::InitializeMapSprites()` — moved stump/tree sprite initialization
- Removed old HQ creation code (~80 lines) from GameScene::Load

#### Stage E build fixes (previous session) ✅
- Fixed `WorldBootstrap.h` forward declarations
- Fixed `InputController.h/cpp` namespace issues
- Added missing private method declarations to `GameScene.h`
- Fixed `ConfirmDeleteFlag` signature mismatch
- Removed dead `UpdateGamepadUI`, `ClearRoadTilesForFlag`
- Removed unused `#include "../World/Components/BuildingFactory.h"`

#### Stage E remaining calls in GameScene.cpp (non-goal for now):
- `World::CreateBuilding(...)` @ ConfirmConstruction — should go through CommandBus
- `m_map->RemoveGroundResource(gi)` @ Update — resource decay, could go to SimulationSystem
- `m_map->SetWildlifeSystem(NULL)` @ destructor — valid cleanup
- Read-queries via `m_flagManager->Get*`, `m_carrierManager->Get*`, `m_map->Get*` — mostly in Update/input, not render

---

### Stage C: Deferred Deletion

Implemented three-state lifecycle for flag deletion:
1. `OnCommand(Cmd_DeleteFlag)` → sets `flag->state = PendingDelete`, pushes to `m_pendingFlags`
2. `ObjectLifecycleManager::FlushDeletions()` → processes all pending flags at end of frame (releases cargo, removes roads/carriers, calls `FlagManager::RemoveFlag`, posts `Event_FlagDeleted`)
3. `FlagManager::GetFlagAt` / `GetFlagById` / `GetFlagPairs` / `GetFlagData` — skip non-Active flags

**Files changed:**
- `World/ObjectLifecycleManager.h` — added `FlushDeletions()`, `std::vector<Flag*> m_pendingFlags`
- `World/ObjectLifecycleManager.cpp` — `OnCommand` sets PendingDelete instead of immediate ForceDeleteFlag; `FlushDeletions` processes queue
- `World/FlagManager.cpp` — `GetFlagAt`, `GetFlagById`, `GetFlagPairs`, `GetFlagData` skip `PendingDelete` flags
- `Scene/GameScene.cpp` — calls `m_objectLifecycleManager->FlushDeletions()` at end of `Update()`

**Current line counts:**
- `GameScene.cpp`: **1563** lines (was ~3660)
- `GameScene.h`: 219 lines (was 218)
- New: `GameRenderer.cpp`: ~1150 lines
- New: `GameRenderer.h`: 140 lines

### Build Config
- **Platform**: Xbox 360 (C++03, no variadic templates, `std::function`, auto, range-for)
- **SDK**: Not available for local builds — correctness by code review only
