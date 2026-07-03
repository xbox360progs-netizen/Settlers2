#pragma once

#include "../Graphics/RenderQueue.h"
#include "Shared/RenderFrame.h"
#include "Buildings/BuildingRenderer.h"
#include "Terrain/TerrainPass.h"
#include "Cursor/CursorPass.h"
#include "Flags/FlagResourcePass.h"
#include "Wildlife/WildlifePass.h"
#include "Placement/PlacementPreviewPass.h"
#include "Roads/RoadPreviewPass.h"
#include "Overlays/GeologistOverlayPass.h"
#include "Overlays/HuntingSpotPass.h"
#include "UI/ConfirmationMenuPass.h"
#include "UI/NotificationPass.h"
#include "UI/TownHallPanelPass.h"
#include "UI/LogisticsDebugPass.h"
#include "UI/ResourceHudPass.h"
#include "UI/BannerPass.h"
#include "UI/MenuPass.h"
#include "BackgroundPass.h"
#include "Roads/RoadConnectionPass.h"
#include "Overlays/WorkSitePass.h"
#include "Resources/GroundResourcePass.h"
#include "Workers/WorkerPass.h"
#include "Buildings/BuildingHighlightPass.h"
#include "Rendering/RenderCommandBuffer.h"
#include "Rendering/RenderGraph.h"

// Forward declarations
class Renderer;
class TileRenderer;
class Camera;
class TextManager;
namespace World {
    class Map;
}
namespace Scene {

// ─── Render pass wrappers (delegate to dedicated renderers) ────────────

class BuildingRenderPass : public RenderPass {
    BuildingRenderer& m_renderer;
public:
    explicit BuildingRenderPass(BuildingRenderer& r) : m_renderer(r) {}
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
        Camera* camera,
        World::Map*       map,
        TextManager*                textManager
    );

    void Render(Graphics::RenderQueue* renderQueue, const RenderFrame& renderFrame);

private:
    // Dependencies (non-owning pointers)
    TileRenderer*     m_tileRenderer;
    Renderer*         m_renderer;
    Camera* m_camera;
    World::Map*       m_map;
    TextManager*                m_textManager;

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
    CursorPass m_cursorPass;
    FlagResourcePass m_flagResourcePass;
    WildlifePass m_wildlifePass;
    PlacementPreviewPass m_placementPreviewPass;
    RoadPreviewPass m_roadPreviewPass;
    GeologistOverlayPass m_geologistOverlayPass;
    HuntingSpotPass m_huntingSpotPass;
    ConfirmationMenuPass m_confirmationMenuPass;
    NotificationPass m_notificationPass;
    TownHallPanelPass m_townHallPanelPass;
    BuildingHighlightPass m_buildingHighlightPass;
    LogisticsDebugPass m_logisticsDebugPass;
    ResourceHudPass m_resourceHudPass;
    BannerPass m_bannerPass;
    MenuPass m_menuPass;
    BackgroundPass m_backgroundPass;
    RoadConnectionPass m_roadConnectionPass;
    WorkSitePass m_workSitePass;
    GroundResourcePass m_groundResourcePass;
    WorkerPass m_workerPass;
};

} // namespace Scene
