# Pipeline Roadmap

## Macro Roadmap

```
Transport Complete
        │
        ▼
RenderFrame becomes the only visual source
        │
        ▼
Old renderer removed
        │
        ▼
Visual World Verification (T1–T8)
        │
        ▼
Remove legacy transport (Phase 8.4)
        │
        ▼
Cycle 3 — Definition Pattern
```

## Pipeline Stages

### Stage 1 — Full RenderFrame ✅
```
RenderFrame { frameId, simTick, settlers }
SettlerPresentationSystem  →  RenderFrame.settlers
SettlerRenderer            ←  RenderFrame.settlers
```

### Stage 2 — RenderBuilding + BuildingPresentationSystem ✅
```
RenderBuilding               structure DTO (kind: 0=flag, 1=building)
BuildingPresentationSystem   reads FlagManager → writes RenderFrame.buildings
BuildingRenderer             resolves sprites (flag/building) from DTO fields
GameRenderer                 removes inline flag rendering → delegates to RenderFrame
```
Pending: building sprites still come from Buildings map layer (TileRenderer). Migration to RenderFrame is deferred until Buildings layer is removed from tile rendering.

### Stage 2.5 — BuildingVisual split ✅
```
RenderBuilding   →   RenderTransform + BuildingVisual
```
Matches the pattern established by RenderSettler. Prevents struct bloat when selecting/highlighting/depleted flags are added.

### Stage 3 — ProjectionSystem ✅
```
ScreenTransform { screenX, screenY, depth }
Pipeline: Simulation → Presentation → Projection → RenderFrame → Renderer
Camera/Zoom/Shake → ProjectionSystem only, never touches simulation.
Renderers use ScreenSprite() with SHADER_UI + LAYER_WORLD (screen coords, no VP transform).
GameRenderer no longer needs camera for entity rendering — only terrain VP remains.
```

**Temporary compromise**: `ScreenSprite` uses `SHADER_UI` because there is no dedicated `SHADER_WORLD_SCREEN` yet. Creating a dedicated shader is deferred until Stage 4.

### Stage 4 — RenderCommandBuffer ✅
```
RenderFrame → Renderer → CommandBuffer → GPU
```
Renderers no longer depend on `Graphics::RenderQueue`, `RenderCommandBuilder`, or shader IDs. The scene-to-graphics boundary is now `CommandBuffer` (scene) → `RenderQueue` (graphics).

### Stage 5 — RenderGraph ✅
```
RenderFrame → RenderGraph → passes → CommandBuffer → RenderQueue
```
Before: GameRenderer orchestrates renderers inline. After: RenderGraph owns pass order; GameRenderer delegates to graph.

### Stage 6 — TerrainPresentation + TerrainPass (next)
```
TileRenderer → TerrainPresentationSystem → RenderFrame.terrain
TileRenderer becomes pure render (reads DTOs only)
```
Goal: Remove the last world→graphics bypass. After Stage 6: `TileRenderer::RenderMap()` disappears from `GameRenderer`.

**Forbidden after Stage 6**:
- `TileRenderer → Camera`
- `TileRenderer → RenderQueue`

### Stage 7 — Kill inline submit
All remaining direct `Submit` calls in `GameRenderer` become passes:
- CursorPass ✅
- PreviewPass ✅
- FlagResourcePass ✅
- WildlifePass ✅
- UiPass (menus, notifications, status) ⏳

Goal: `RenderGraph` owns full frame execution. After Stage 7: render regression test becomes possible — `RenderFrame → RenderGraph → CommandBuffer` without running the game.

### Stage 8 — RenderContext ✅
Introduced `RenderContext` as per-frame readonly context:
```
struct RenderContext {
    const Camera* camera;
    float   time;
    bool    debugOverlay;
};
```
Changed `RenderPass::Execute(frame, buffer)` → `Execute(frame, context, buffer)`.

### Stage 9 — Core0 / Core1 split
```
RenderFrame frames[2];
volatile int front, back;
Core1: Simulation → Presentation → Projection → Build(back) → Publish()
Core0: Read(front) → RenderGraph → RenderContext → CommandBuffer → RenderQueue → GPU
```
No refactoring needed — all contracts already defined by Stage 8. Split is organizational (move files to Core0 project), not architectural.

### Stage 10 — Asset Definition Pipeline
Separate **Definitions** (static metadata) from **Runtime State** (per-frame DTO):
```
BuildingType → BuildingDefinition → BuildingVisual → RenderFrame
WorkerType   → WorkerDefinition   → SettlerVisual   → RenderFrame
```
Eliminates all `if (type == WOODCUTTER)` from render path.

---

## Completed Sub-stages

### Stage 7D2 — RoadPreviewPass ✅
Road preview migrated to DTO+Pass pattern:
- `RenderRoadSegment { worldX0/Y0, worldX1/Y1, screenX0/Y0, screenX1/Y1, valid }`
- `RoadPreviewPresentationSystem` reads `GetPreviewPath()`/`GetAutoPath()`/`GetValidNeighbors()`
- `ProjectionSystem::ProjectRoadPreview()` projects both endpoints
- Removed `m_roadController`, `m_roadManager`, `RoadController.h`, `RoadManager.h` from `GameRenderer`

### Stage 8B1 — ConfirmationMenuPass ✅
First screen-space UI pass. Geologist confirmation dialog migrated to DTO+Pass:
- `RenderConfirmationMenu { visible, selected, style }`
- `ConfirmationMenuPresentationSystem` reads `UIMenu::IsVisible()`
- First pass that reads `RenderFrame.ui` instead of calling a menu object directly

### Stage 8B2 — NotificationPass ✅
Notifications (top-right stacked panels) migrated to DTO+Pass pattern:
- `RenderNotification { isActive, alpha, offsetY, title[32], line1[32], line2[32] }`
- `NotificationPresentationSystem` reads `UiFrameState::notifications[]`
- Pipeline: `NotificationManager::FillFrameContext(uiState)` → `NotificationPresentationSystem::BuildRenderFrame(uiState, renderFrame.ui)` → `NotificationPass::Execute(renderFrame, context, buffer)`

### Stage 7E1 — GeologistOverlayPass ✅
Geologist world-space overlays migrated to DTO+Pass pattern:
- `RenderOverlayMarker { RenderTransform, markerType, resourceType }`
- `GeologistOverlayPresentationSystem` reads `Map` (resource nodes, cursor tile) + `FrameContext.overlay`
- `ProjectionSystem::ProjectOverlays()` — uniform projection, no exceptions
- Removed `RenderGeologistOverlay()` function (~200 lines) from `GameRenderer`

---

## Scene maturity

```
Terrain          ✅  (Stage 6C)
Buildings        ✅  (Stage 5)
Workers          ✅  (Stage 7E3 — replaces Settlers)
Settlers         🗑️  (deprecated — kept for RenderGraph registration only)
Wildlife         ✅  (Stage 7C)
Road Preview     ✅  (Stage 7D2)
Placement        ✅  (Stage 7D1)
Cursor           ✅  (Stage 7A)
Flag Resources   ✅  (Stage 7B)
Overlays         ✅  (Stage 7E1)
Ground Resources ✅  (Stage 7E2)
---              ---
UI               ⏳  (Stage 8B1 — ConfirmationMenuPass)
                 👆  (Stage 8B2 — NotificationPass)
Background       ⏳  (Stage 7F)
```

---

## Status — GameScene RenderFrame pipeline complete ✅

```
GameScene → Presentation → RenderFrame → RenderGraph → Pass → CommandBuffer
```

All auxiliary scenes (EditorScene, LoadingScene, MenuScene) use independent renderers — this is not a bypass of the game pipeline.

## Next Steps

1. **T1–T8 soak tests** (Xbox 360)
2. **Remove legacy transport (Phase 8.4)** — delete TransportJobManager, DemandTicket, kUseTransportJobs
3. **Cycle 3 — Definition Pattern** — BuildingDefinition, WorkerDefinition, ResourceDefinition

## Future (no immediate action)

- Auxiliary scene migration to RenderGraph — decide per scene when maintenance requires it
