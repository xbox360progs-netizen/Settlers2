#pragma once

#include "Scene.h"
#include "../Core/EventBus.h"
#include "../Graphics/RenderQueue.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/TileRenderer.h"
#include "../World/Map.h"
#include "../World/WildlifeSystem.h"
#include "../World/EntityManager.h"
#include "../World/Systems/AnimalSystem.h"
#include "../World/Systems/SimulationSystem.h"
#include "../Logic/EconomyManager.h"
#include "../World/CarrierManager.h"
#include "../World/Systems/CarrierSystem.h"
#include "../Logic/AISystem.h"
#include "../World/Components/Building.h"
#include "../World/Flag.h"
#include "../World/FlagManager.h"
#include "../World/RoadManager.h"
#include "../World/TransportJobManager.h"
#include "../World/CargoManager.h"
#include "../World/DemandManager.h"
#include "../World/StorehouseManager.h"
#include "../World/ConstructionSite.h"
#include "../World/ConstructionManager.h"
#include "../World/ObjectLifecycleManager.h"
#include "../World/WorkerManager.h"
#include "../Logic/ResourceRegistry.h"
#include "../Graphics/Camera.h"
#include "../Graphics/TextManager.h"
#include "../Input/InputManager.h"
#include "../UI/GridMenu.h"
#include "../UI/UIMenu.h"
#include "../Logic/AStar.h"
#include "../Logic/WeightMap.h"
#include <string>
#include <string.h>
#include "../World/UiDefs.h"
#include "BuildingPlacement.h"
#include "ConstructionVisualizer.h"
#include "PlacementController.h"
#include "RoadController.h"

namespace Scene {

struct EconomyJobData
{
    Logic::EconomyManager* economy;
    World::CarrierManager* carriers;
};

class GameScene : public Scene, public Core::EventListener
{
public:
    GameScene();
    virtual ~GameScene();

    virtual void Initialize(IDirect3DDevice9* device, Graphics::SpriteRenderer* spriteRenderer);
    virtual void Load();
    virtual void Unload();
    virtual void Update(float deltaTime);
    virtual void Render(Graphics::RenderQueue* renderQueue);
    
    virtual void OnEvent(Core::EventType type, void* data);

    // Setter for renderer (should be called after Initialize)
    void SetRenderer(Renderer* renderer) { m_renderer = renderer; }
    void SetInputManager(Input::InputManager* inputManager) { m_inputManager = inputManager; }
    void SetTextManager(TextManager* textManager) { m_textManager = textManager; }

private:
    // ─── Simulation system (owns game logic subsystems) ──────
    World::SimulationSystem m_simulation;
    Core::EventBus* m_eventBus;

    // Systems (legacy pointers — gradually migrating into m_simulation)
    World::Map* m_map;
    World::EntityManager* m_entityManager;
    World::AnimalSystem* m_animalSystem;
    World::AnimalManager* m_animalManager;
    World::WildlifeSystem* m_wildlife;
    Logic::EconomyManager* m_economyManager;
    World::CarrierManager* m_carrierManager;
    World::CarrierSystem* m_carrierSystem;
    Logic::AISystem* m_aiSystem;

    // Graphics
    Renderer* m_renderer;
    TileRenderer* m_tileRenderer;
    Camera* m_camera;

    Input::InputManager* m_inputManager;

    // Cursor & interaction
    int m_cursorTileX;
    int m_cursorTileY;
    GridMenu* m_buildMenu;
    GridMenu* m_roadMenu;
    UIMenu* m_flagMenu;
    UIMenu::ItemData m_flagMenuItemData[3];
    int m_flagMenuItemCount;
    UIMenu* m_geologistMenu;
    bool m_geologistMenuActive;
    bool m_menuActive;
    bool m_roadMenuActive;
    bool m_flagMenuActive;
    bool m_cursorOnTownHall;
    bool m_townHallPanelOpen;
    bool m_logisticsDebug;

    // Gamepad cursor (console)
    Vector2i m_gamepadCursor;
    float m_gamepadCursorCooldown;
    bool m_gamepadActive;

    // Fixed-pool popup windows
    World::PopupUiData m_popups[World::MAX_UI_POPUPS];
    int m_popupCount;

    // Geologist state machine
    enum GeologistState {
        GEOLOGIST_NONE,
        GEOLOGIST_CONFIRM,   // showing "Call geologist?" UI
        GEOLOGIST_WORKING,   // timer counting down on a mountain
    };
    GeologistState m_geologistState;
    float m_geologistTimer;     // seconds remaining
    int m_geologistTileX;       // mountain tile being surveyed
    int m_geologistTileY;

    // UI
    TextManager* m_textManager;
    std::string m_statusText;
    float m_statusTextTimer;

    // Placement state machine
    PlacementController m_placement;
    class BuildingPlacementManager* m_placementManager;

    // Flags & Roads
    World::FlagManager* m_flagManager;
    World::RoadManager* m_roadManager;
    World::TransportJobManager* m_transportJobManager;
    World::CargoManager* m_cargoManager;
    World::DemandManager* m_demandManager;

    World::StorehouseManager* m_storehouseManager;

    // Construction sites
    World::ConstructionManager* m_constructionManager;
    ConstructionVisualizer* m_constructionVisualizer;

    // Lifecycle
    World::ObjectLifecycleManager* m_objectLifecycleManager;

    // Worker arrival
    World::WorkerManager* m_workerManager;

    // Road building state
    RoadController m_roadController;

    // Town hall panel data
    int m_townHallPanelBgIdx;
    float m_townHallPanelU0, m_townHallPanelV0, m_townHallPanelU1, m_townHallPanelV1;
    float m_townHallPanelW, m_townHallPanelH;

    // Notification banner (bunner_info)
    float m_bannerSlideX;
    float m_bannerTargetX;
    float m_bannerW;
    float m_bannerH;
    float m_bannerU0, m_bannerV0, m_bannerU1, m_bannerV1;
    bool m_bannerLoaded;

    // Resource HUD
    struct ResourceHudItem {
        World::ResourceType type;
        const char* iconName;
        int iconIdx; // cached atlas index
        int showOrder;
    };
    static const int RESOURCE_HUD_COUNT = 11;
    ResourceHudItem m_resourceHud[RESOURCE_HUD_COUNT];
    bool m_resourceHudLoaded;

    // Frame counter for ground resource age checks
    unsigned int m_frameCount;

    // Ground resource rendering
    int m_groundWoodIconIdx;
    bool m_groundWoodIconLoaded;

    enum {
        SLOT_BUILDINGS_HIGHLIGHT = 18,
        SLOT_BACKGROUND = 19,
        SLOT_UI_CURSOR = 20,
        SLOT_UI_MENU_BG = 21,
        SLOT_UI_MENU_CELL = 22,
        SLOT_UI_MENU_ICON = 23,
        SLOT_STREETS = 24,
        SLOT_UI_TOWNHALL_PANEL = 25,
        SLOT_UI_ROAD_BG = 26,
        SLOT_UI_ROAD_CELL = 27,
        SLOT_UI_ROAD_ICON = 28,
        SLOT_UNITS = 29,
        SLOT_FLAG_RESOURCES = 30,
    };

    void UpdateCursor();
    void RenderCursor(Graphics::RenderQueue* renderQueue);
    void InitBuildMenu();
    void InitRoadMenu();
    void InitFlagMenu();
    void InitGeologistMenu();

    void HandlePlaceAtCursor();
    void HandleConfirmFreeFlag();
    bool CanPlaceBuilding(World::BuildingType type, int buildX, int buildY);
    void ConfirmConstruction(World::Flag* flag);
    void ConfirmDeleteFlag(int tileX, int tileY);
    void ClearRoadTilesForFlag(World::Flag* flag);
    void LinkFlagToRoadNetwork(World::Flag* flag);
    void SyncCarriersForFlag(World::Flag* flag);
    World::BuildingType GetBuildingTypeFromSpriteName(const std::string& name) const;

    void RestoreBuildingsFromLayer();
    void AssignOreDepositsToMountains();

    // Geologist system
    void ShowGeologistConfirm(int tx, int ty);
    void StartGeologistSurvey();
    void CancelGeologistMenu();
    void RenderGeologistOverlay(Graphics::RenderQueue* renderQueue);

    // Update phase methods (extracted from the monolithic Update())
    void UpdateCamera(float dt);
    void UpdateBanner(float dt);
    void UpdateStatusText(float dt);
    void UpdateGeologist(float dt);
    void HandleInput();
    void UpdateWildlife(float dt);
    void CollectGroundResources();
    void CheckConstructionSites();

    // Gamepad input & console UI
    void HandleGamepadInput();
    void OnGamepadButton(uint32_t buttons);
    void SpawnGeologistPopup(int tx, int ty);
    void UpdateGamepadUI(float dt);
    void PushUiToQueue();

    void GetEntranceOffset(const std::string& buildingName, int& outX, int& outY);

    // Wildlife regeneration timer
    float m_wildlifeRegenTimer;
    // Tree growth timer (Sapling > Young > Mature)
    float m_treeGrowthTimer;

    // Job data (reused each frame)
    EconomyJobData m_economyJobData;
};

} // namespace Scene
