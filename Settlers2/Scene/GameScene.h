#pragma once

#include "Scene.h"
#include "../Core/EventBus.h"
#include "../Core/CommandBus.h"
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
#include "WorldRestorer.h"
#include "MenuBootstrap.h"
#include "WorldBootstrap.h"
#include "TextureSlots.h"
#include "../World/Systems/RoadNetworkRelinker.h"
#include "InputController.h"
#include "GameRenderer.h"

namespace Scene {

struct EconomyJobData
{
    Logic::EconomyManager* economy;
    World::CarrierManager* carriers;
};

class GameScene : public Scene, public Core::EventListener, public IInputHost
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

    // IInputHost
    virtual void DeleteFlagAt(int tileX, int tileY);
    virtual void OnMountainTileAction(int tileX, int tileY);
    virtual void CancelGeologist();

private:
    // ─── Simulation system (owns game logic subsystems) ──────
    World::SimulationSystem m_simulation;
    Core::EventBus* m_eventBus;
    Core::CommandBus* m_commandBus;

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

    // Renderer (delegates rendering to GameRenderer)
    GameRendererState m_renderState;
    GameRenderer* m_gameRenderer;

    // Input (delegates to InputController)
    InputController* m_inputController;
    Input::InputManager* m_inputManager;

    // Menus (owned by GameScene, accessed by InputController via pointers)
    GridMenu* m_buildMenu;
    GridMenu* m_roadMenu;
    UIMenu* m_flagMenu;
    UIMenu::ItemData m_flagMenuItemData[3];
    int m_flagMenuItemCount;
    UIMenu* m_geologistMenu;

    // Fixed-pool popup windows
    World::PopupUiData m_popups[World::MAX_UI_POPUPS];
    int m_popupCount;

    // Geologist state machine (state in m_renderState)
    float m_geologistTimer;

    // UI
    TextManager* m_textManager;

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

    // World restorer (load-time building/flag restoration)
    WorldRestorer m_restorer;

    // Worker arrival
    World::WorkerManager* m_workerManager;

    // Road building state
    RoadController m_roadController;
    World::RoadNetworkRelinker m_relinker;

    // Notification banner (bunner_info — slide target, state in m_renderState)
    float m_bannerTargetX;



    // Frame counter for ground resource age checks
    unsigned int m_frameCount;





    void InitBuildMenu();
    void InitRoadMenu();
    void InitFlagMenu();
    void InitGeologistMenu();

    bool CanPlaceBuilding(World::BuildingType type, int buildX, int buildY);
    void ConfirmConstruction(World::Flag* flag);
    World::BuildingType GetBuildingTypeFromSpriteName(const std::string& name) const;

    // Update phase methods
    void UpdateCamera(float dt);
    void UpdateBanner(float dt);
    void UpdateStatusText(float dt);
    void UpdateGeologist(float dt);
    void UpdateWildlife(float dt);
    void CollectGroundResources();
    void CheckConstructionSites();

    // Input wrappers (delegate to InputController)
    void HandleInput();
    void UpdateCursor();

    // Flag/building deletion
    void ConfirmDeleteFlag(World::Flag* flag);

    // Geologist
    void ShowGeologistConfirm(int tx, int ty);
    void StartGeologistSurvey();

    // Console UI
    void SpawnGeologistPopup(int tx, int ty);

    void GetEntranceOffset(const std::string& buildingName, int& outX, int& outY);

    // Wildlife regeneration timer
    float m_wildlifeRegenTimer;
    // Tree growth timer (Sapling > Young > Mature)
    float m_treeGrowthTimer;

    // Job data (reused each frame)
    EconomyJobData m_economyJobData;
};

} // namespace Scene
