#pragma once

#include "../Graphics/RenderQueue.h"
#include "../World/Components/Building.h"
#include "FrameContext.h"

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
    class RoadController;
}
class GridMenu;
class UIMenu;

namespace Scene {

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
        RoadController*             roadController,
        GridMenu*                   buildMenu,
        UIMenu*                     flagMenu,
        UIMenu*                     geologistMenu,
        TextManager*                textManager
    );

    void Render(Graphics::RenderQueue* renderQueue, const FrameContext& frame);

private:
    void RenderCursor(Graphics::RenderQueue* renderQueue, const FrameContext& frame);
    void RenderGeologistOverlay(Graphics::RenderQueue* renderQueue, const FrameContext& frame);
    void PushUiToQueue(Graphics::RenderQueue* renderQueue, const FrameContext& frame);

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
    RoadController*             m_roadController;
    GridMenu*                   m_buildMenu;
    UIMenu*                     m_flagMenu;
    UIMenu*                     m_geologistMenu;
    TextManager*                m_textManager;


    // Self-contained render state (not shared with GameScene)
    int  m_groundWoodIconIdx;
    bool m_groundWoodIconLoaded;
};

} // namespace Scene
