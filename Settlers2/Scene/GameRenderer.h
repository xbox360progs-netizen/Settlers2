#pragma once

#include "../Graphics/RenderQueue.h"
#include "../World/Components/Building.h"
#include "FrameContext.h"
#include "Shared/RenderFrame.h"
#include "Settlers/SettlerRenderer.h"
#include "Buildings/BuildingRenderer.h"
#include "Terrain/TerrainPass.h"
#include "Cursor/CursorPass.h"
#include "Flags/FlagResourcePass.h"
#include "Wildlife/WildlifePass.h"
#include "Placement/PlacementPreviewPass.h"
#include "Roads/RoadPreviewPass.h"
#include "Overlays/GeologistOverlayPass.h"
#include "UI/ConfirmationMenuPass.h"
#include "UI/NotificationPass.h"
#include "Resources/GroundResourcePass.h"
#include "Rendering/RenderCommandBuffer.h"
#include "Rendering/RenderGraph.h"

// Forward declarations
class Renderer;
class TileRenderer;
class Camera;
class TextManager;
namespace World {
    class Map;
    class FlagManager;
    class CarrierManager;
    class RoadManager;
    class ConstructionManager;
    class WorkerManager;
}
namespace Logic {
    class EconomyManager;
}
namespace Scene {
    class PlacementController;
    class RoadController;
}
class GridMenu;
class UIMenu;
struct RenderContext;

namespace Scene {

// ─── Render pass wrappers (delegate to dedicated renderers) ────────────

class BuildingRenderPass : public RenderPass {
    BuildingRenderer& m_renderer;
public:
    explicit BuildingRenderPass(BuildingRenderer& r) : m_renderer(r) {}
    void Execute(const RenderFrame& frame, const RenderContext& context,
                 RenderCommandBuffer& buffer);
};

class SettlerRenderPass : public RenderPass {
    SettlerRenderer& m_renderer;
public:
    explicit SettlerRenderPass(SettlerRenderer& r) : m_renderer(r) {}
    void Execute(const RenderFrame& frame, const RenderContext& context,
                 RenderCommandBuffer& buffer);
};

// ─── Render-only class (no game-logic knowledge) ───────────────────────
class GameRenderer
{
public:
    GameRenderer(
        TileRenderer*     tileRenderer,
        Renderer*         renderer,
        Camera*           camera,
        World::Map*       map,
        World::FlagManager*         flagManager,
        World::CarrierManager*      carrierManager,
        World::ConstructionManager* constructionManager,
        World::WorkerManager*       workerManager,
        Logic::EconomyManager*      economyManager,
        PlacementController*        placement,
        GridMenu*                   buildMenu,
        UIMenu*                     flagMenu,
        TextManager*                textManager
    );

    void Render(Graphics::RenderQueue* renderQueue, const FrameContext& frame, const RenderFrame& renderFrame);

private:
    void PushUiToQueue(Graphics::RenderQueue* renderQueue, const FrameContext& frame);

    // Dependencies (non-owning pointers)
    TileRenderer*     m_tileRenderer;
    Renderer*         m_renderer;
    Camera*           m_camera;
    World::Map*       m_map;
    World::FlagManager*         m_flagManager;
    World::CarrierManager*      m_carrierManager;
    World::ConstructionManager* m_constructionManager;
    World::WorkerManager*       m_workerManager;
    Logic::EconomyManager*      m_economyManager;
    PlacementController*        m_placement;
    GridMenu*                   m_buildMenu;
    UIMenu*                     m_flagMenu;
    TextManager*                m_textManager;


    // Settler renderer (reads DTOs only — no simulation access)
    SettlerRenderer m_settlerRenderer;

    // Building renderer (reads DTOs only — renders flags + buildings)
    BuildingRenderer m_buildingRenderer;

    // Self-contained render state (not shared with GameScene)

    // Scene-side command buffer: collects projected entity commands
    // before batch submission to the graphics RenderQueue.
    RenderCommandBuffer m_commandBuffer;

    // Render graph: ordered pass list that replaces inline entity rendering.
    RenderGraph m_renderGraph;
    TerrainPass m_terrainPass;
    BuildingRenderPass m_buildingRenderPass;
    SettlerRenderPass m_settlerRenderPass;
    CursorPass m_cursorPass;
    FlagResourcePass m_flagResourcePass;
    WildlifePass m_wildlifePass;
    PlacementPreviewPass m_placementPreviewPass;
    RoadPreviewPass m_roadPreviewPass;
    GeologistOverlayPass m_geologistOverlayPass;
    ConfirmationMenuPass m_confirmationMenuPass;
    NotificationPass m_notificationPass;
    GroundResourcePass m_groundResourcePass;
};

} // namespace Scene
