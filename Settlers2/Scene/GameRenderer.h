#pragma once

#include "../Graphics/RenderQueue.h"
#include "../World/Components/Building.h"
#include "../World/UiDefs.h"

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
    class WildlifeSystem;
}
namespace Logic {
    class EconomyManager;
}
namespace Scene {
    class PlacementController;
    class InputController;
    class RoadController;
}
class GridMenu;
class UIMenu;

namespace Scene {

// ─── Shared mutable state between GameScene and GameRenderer ────────────
struct GameRendererState {
    // Geologist overlay
    int geologistState;
    int geologistTileX;
    int geologistTileY;

    static const int GEOLOGIST_NONE    = 0;
    static const int GEOLOGIST_CONFIRM = 1;
    static const int GEOLOGIST_WORKING = 2;

    // Town hall info panel
    int   townHallPanelBgIdx;
    float townHallPanelU0, townHallPanelV0, townHallPanelU1, townHallPanelV1;
    float townHallPanelW, townHallPanelH;

    // Notification banner
    float bannerSlideX;
    float bannerW, bannerH, bannerU0, bannerV0, bannerU1, bannerV1;
    bool  bannerLoaded;

    // Resource HUD
    struct ResourceHudItem {
        World::ResourceType type;
        const char*         iconName;
        int                 iconIdx;
        int                 showOrder;
    };
    static const int RESOURCE_HUD_COUNT = 11;
    ResourceHudItem resourceHud[RESOURCE_HUD_COUNT];
    bool             resourceHudLoaded;

    GameRendererState();
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
        World::RoadManager*         roadManager,
        World::ConstructionManager* constructionManager,
        World::WorkerManager*       workerManager,
        World::WildlifeSystem*      wildlife,
        Logic::EconomyManager*      economyManager,
        PlacementController*        placement,
        InputController*            inputController,
        RoadController*             roadController,
        GridMenu*                   buildMenu,
        UIMenu*                     flagMenu,
        UIMenu*                     geologistMenu,
        TextManager*                textManager,
        GameRendererState*          state
    );

    void Render(Graphics::RenderQueue* renderQueue);

private:
    void RenderCursor(Graphics::RenderQueue* renderQueue);
    void RenderGeologistOverlay(Graphics::RenderQueue* renderQueue);
    void PushUiToQueue(Graphics::RenderQueue* renderQueue);

    // Dependencies (non-owning pointers)
    TileRenderer*     m_tileRenderer;
    Renderer*         m_renderer;
    Camera*           m_camera;
    World::Map*       m_map;
    World::FlagManager*         m_flagManager;
    World::CarrierManager*      m_carrierManager;
    World::RoadManager*         m_roadManager;
    World::ConstructionManager* m_constructionManager;
    World::WorkerManager*       m_workerManager;
    World::WildlifeSystem*      m_wildlife;
    Logic::EconomyManager*      m_economyManager;
    PlacementController*        m_placement;
    InputController*            m_inputController;
    RoadController*             m_roadController;
    GridMenu*                   m_buildMenu;
    UIMenu*                     m_flagMenu;
    UIMenu*                     m_geologistMenu;
    TextManager*                m_textManager;
    GameRendererState*          m_state;

    // Self-contained render state (not shared with GameScene)
    int  m_groundWoodIconIdx;
    bool m_groundWoodIconLoaded;
};

} // namespace Scene
