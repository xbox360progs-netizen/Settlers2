#pragma once

#include "Scene.h"
#include "../Core/JobManager.h"
#include "../Graphics/RenderQueue.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/TileRenderer.h"
#include "../World/Map.h"
#include "../World/WildlifeSystem.h"
#include "../Logic/EconomyManager.h"
#include "../World/CarrierManager.h"
#include "../Logic/AISystem.h"
#include "../World/Components/Building.h"
#include "../World/Flag.h"
#include "../World/FlagManager.h"
#include "../World/ConstructionSite.h"
#include "../World/ConstructionManager.h"
#include "../Logic/ResourceRegistry.h"
#include "../Graphics/Camera.h"
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

struct WildlifeSectorData
{
    World::WildlifeSystem* wildlife;
    int startSpawner;
    int endSpawner;
    std::vector<World::Animal> newAnimals;
};

struct CarrierRangeData
{
    World::CarrierManager* mgr;
    int startCarrier;
    int endCarrier;
    float dt;
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

private:
    JobManager* m_jobManager;

    // Systems
    World::Map* m_map;
    World::WildlifeSystem* m_wildlife;
    Logic::EconomyManager* m_economyManager;
    World::CarrierManager* m_carrierManager;
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
    bool m_menuActive;

    // Build state machine
    enum BuildState {
        BUILDSTATE_NONE,
        BUILDSTATE_PLACE_FLAG,     // selected building → place flag first
        BUILDSTATE_PLACE_ROAD,     // building road between flags
    };
    BuildState m_buildState;
    World::BuildingType m_selectedBuilding;
    int m_placementIconIdx;    // UI atlas sprite index for preview at cursor
    int m_placementConstrIdx;  // Buildings atlas sprite index for construction site
    std::string m_selectedIconName;

    // Flags
    World::FlagManager* m_flagManager;

    // Construction sites
    World::ConstructionManager* m_constructionManager;

    // Road building state
    int m_roadStartX, m_roadStartY;
    std::vector<std::pair<int,int>> m_roadPreviewPath;

    enum {
        SLOT_UI_CURSOR = 20,
        SLOT_UI_MENU_BG = 21,
        SLOT_UI_MENU_CELL = 22,
        SLOT_UI_MENU_ICON = 23,
        SLOT_STREETS = 24,
    };

    static bool IsNodeRoad(int nx, int ny, World::TileLayer* roadsLayer, const std::vector<std::pair<int,int>>& previewPath);
    static int CalcPatternAt(int x, int y, World::TileLayer* roadsLayer, const std::vector<std::pair<int,int>>& previewPath);

    void UpdateCursor();
    void RenderCursor(Graphics::RenderQueue* renderQueue);
    void InitBuildMenu();

    bool CanPlaceBuilding(World::BuildingType type, int buildX, int buildY);
    void PlaceFlag(int tileX, int tileY);
    void CreateConstructionSite(World::Flag* flag, int siteX, int siteY);
    void ConfirmConstruction(World::Flag* flag);
    const char* GetBuildingName(World::BuildingType type) const;

    void StartRoad(int x, int y);
    void UpdateRoadPreview(int cursorX, int cursorY);
    void CommitRoad();
    void CancelRoad();
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

    // Job data (reused each frame)
    EconomyJobData m_economyJobData;
    WildlifeSectorData m_wildlifeSectors[4];
    CarrierRangeData m_carrierRanges[4];
    AIChunkData m_aiChunks[4];
};

} // namespace Scene
