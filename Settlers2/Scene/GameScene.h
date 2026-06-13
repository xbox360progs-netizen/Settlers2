#pragma once

#include "Scene.h"
#include "../Core/JobManager.h"
#include "../Graphics/RenderQueue.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/TileRenderer.h"
#include "../World/Map.h"
#include "../World/WildlifeSystem.h"
#include "../World/EntityManager.h"
#include "../World/Systems/AnimalSystem.h"
#include "../Logic/EconomyManager.h"
#include "../World/CarrierManager.h"
#include "../World/Systems/CarrierSystem.h"
#include "../Logic/AISystem.h"
#include "../World/Components/Building.h"
#include "../World/Flag.h"
#include "../World/FlagManager.h"
#include "../World/RoadManager.h"
#include "../World/TransportJobManager.h"
#include "../World/ConstructionSite.h"
#include "../World/ConstructionManager.h"
#include "../World/ObjectLifecycleManager.h"
#include "../Logic/ResourceRegistry.h"
#include "../Graphics/Camera.h"
#include "../Graphics/TextManager.h"
#include "../Input/InputManager.h"
#include "../UI/GridMenu.h"
#include "../Logic/AStar.h"
#include "../Logic/WeightMap.h"
#include <string>

namespace Scene {

struct EconomyJobData
{
    Logic::EconomyManager* economy;
    World::CarrierManager* carriers;
};

static const int MAX_REQUESTS_PER_CHUNK = 8;

struct AIChunkData
{
    Logic::AISystem* ai;
    World::BuildingType types[4];
    int numTypes;
    Logic::BuildRequest requests[MAX_REQUESTS_PER_CHUNK];
    int numRequests;
};

class GameScene : public Scene
{
public:
    GameScene();
    virtual ~GameScene();

    virtual void Initialize(IDirect3DDevice9* device, Graphics::SpriteRenderer* spriteRenderer);
    virtual void Load();
    virtual void Unload();
    virtual void Update(float deltaTime);
    virtual void Render(Graphics::RenderQueue* renderQueue);
    
    // Setter for renderer (should be called after Initialize)
    void SetRenderer(Renderer* renderer) { m_renderer = renderer; }
    void SetInputManager(Input::InputManager* inputManager) { m_inputManager = inputManager; }
    void SetTextManager(TextManager* textManager) { m_textManager = textManager; }

private:
    JobManager* m_jobManager;

    // Systems
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
    bool m_menuActive;
    bool m_roadMenuActive;
    bool m_cursorOnTownHall;
    bool m_townHallPanelOpen;
    bool m_logisticsDebug;

    // UI
    TextManager* m_textManager;
    std::string m_statusText;
    float m_statusTextTimer;

    // Build state machine
    enum BuildState {
        BUILDSTATE_NONE,
        BUILDSTATE_PLACE_FLAG,     // selected building → place flag first
        BUILDSTATE_PLACE_ROAD,     // building road between flags
        BUILDSTATE_CONFIRM,        // A on ground/flag → confirm action
    };
    enum ConfirmAction {
        CONFIRM_NONE,
        CONFIRM_PLACE_FLAG,
        CONFIRM_START_ROAD,
        CONFIRM_DELETE_FLAG,    // confirmation before deleting a flag with building
    };
    BuildState m_buildState;
    ConfirmAction m_confirmAction;
    int m_confirmTargetX;
    int m_confirmTargetY;
    World::BuildingType m_selectedBuilding;
    int m_placementIconIdx;    // UI atlas sprite index for preview at cursor
    int m_placementConstrIdx;  // Buildings atlas sprite index for construction site
    std::string m_selectedIconName;

    // Flags & Roads
    World::FlagManager* m_flagManager;
    World::RoadManager* m_roadManager;
    World::TransportJobManager* m_transportJobManager;

    // Construction sites
    World::ConstructionManager* m_constructionManager;

    // Lifecycle
    World::ObjectLifecycleManager* m_objectLifecycleManager;

    // Road building state
    int m_roadStartX, m_roadStartY;
    std::vector<std::pair<int,int>> m_roadPreviewPath;
    std::vector<std::pair<int,int>> m_roadValidNeighbors;
    std::vector<std::pair<int,int>> m_roadAutoPath;

    // Town hall panel data
    int m_townHallPanelBgIdx;
    float m_townHallPanelU0, m_townHallPanelV0, m_townHallPanelU1, m_townHallPanelV1;
    float m_townHallPanelW, m_townHallPanelH;

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

    enum {
        SLOT_BUILDINGS_HIGHLIGHT = 18,
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

    static bool IsNodeRoad(int nx, int ny, World::TileLayer* roadsLayer, const std::vector<std::pair<int,int>>& previewPath);
    static int CalcPatternAt(int x, int y, World::TileLayer* roadsLayer, const std::vector<std::pair<int,int>>& previewPath);

    void UpdateCursor();
    void RenderCursor(Graphics::RenderQueue* renderQueue);
    void InitBuildMenu();
    void InitRoadMenu();

    bool CanPlaceBuilding(World::BuildingType type, int buildX, int buildY);
    void PlaceFlag(int tileX, int tileY);
    void PlaceFreeFlag(int tileX, int tileY);
    void CreateConstructionSite(World::Flag* flag, int siteX, int siteY);
    void ConfirmConstruction(World::Flag* flag);
    void ConfirmDeleteFlag(int tileX, int tileY);
    void ClearRoadTilesForFlag(World::Flag* flag);
    const char* GetBuildingName(World::BuildingType type) const;
    const char* GetBuildingSpriteName(World::BuildingType type) const;
    World::BuildingType GetBuildingTypeFromSpriteName(const std::string& name) const;

    void RestoreBuildingsFromLayer();

    void StartRoad(int x, int y);
    void UpdateRoadPreview(int cursorX, int cursorY);
    void TryAddRoadTile(int x, int y);
    void CommitRoad();
    void CancelRoad();
    void LinkFlagToRoadNetwork(World::Flag* flag);
    void SplitRoadAtFlag(World::Flag* flag);
    void SyncCarriersForFlag(World::Flag* flag);
    void RebuildRoadSprite(int x, int y);
    void UpdateRoadNeighbors(int x, int y);
    void GetEntranceOffset(const std::string& buildingName, int& outX, int& outY);

    // Construction sprite known-good UV (pixel rect 1022,1883,196,139 in 2048x2048 atlas)
    static const float CONSTRUCTION_U0;
    static const float CONSTRUCTION_V0;
    static const float CONSTRUCTION_U1;
    static const float CONSTRUCTION_V1;
    static const uint32_t CONSTRUCTION_ATLAS_W;
    static const uint32_t CONSTRUCTION_ATLAS_H;
    static const uint32_t CONSTRUCTION_PIXEL_X;
    static const uint32_t CONSTRUCTION_PIXEL_Y;
    static const uint32_t CONSTRUCTION_PIXEL_W;
    static const uint32_t CONSTRUCTION_PIXEL_H;

    // Wildlife regeneration timer
    float m_wildlifeRegenTimer;

    // Job data (reused each frame)
    EconomyJobData m_economyJobData;
    AIChunkData m_aiChunks[4];
};

} // namespace Scene
