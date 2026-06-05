#include "stdafx.h"
#include "GameScene.h"
#include "../Graphics/RenderQueue.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/SpriteAtlas.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/TextureRegistry.h"
#include "../World/Map.h"
#include "../World/MapSerializer.h"
#include "../Logic/EconomyManager.h"
#include "../World/CarrierManager.h"
#include "../World/Warehouse.h"
#include "../World/Components/BuildingFactory.h"
#include "../Logic/CoordinateSystem.h"

namespace Scene {

    // Construction sprite UV fix — pixel rect (1022,1883,196,139) in 2048x2048 atlas
    const float GameScene::CONSTRUCTION_U0 = 1022.0f / 2048.0f;
    const float GameScene::CONSTRUCTION_V0 = 1883.0f / 2048.0f;
    const float GameScene::CONSTRUCTION_U1 = (1022.0f + 196.0f) / 2048.0f;
    const float GameScene::CONSTRUCTION_V1 = (1883.0f + 139.0f) / 2048.0f;
    const uint32_t GameScene::CONSTRUCTION_ATLAS_W = 2048;
    const uint32_t GameScene::CONSTRUCTION_ATLAS_H = 2048;
    const uint32_t GameScene::CONSTRUCTION_PIXEL_X = 1022;
    const uint32_t GameScene::CONSTRUCTION_PIXEL_Y = 1883;
    const uint32_t GameScene::CONSTRUCTION_PIXEL_W = 196;
    const uint32_t GameScene::CONSTRUCTION_PIXEL_H = 139;

    static void EconomyJobFunc(void* data);
    static void WildlifeSectorFunc(void* data);
    static void CarrierAssignFunc(void* data);
    static void CarrierUpdateFunc(void* data);
    static void AIChunkJobFunc(void* data);

    GameScene::GameScene()
        : Scene("Game")
        , m_jobManager(NULL)
        , m_map(NULL)
        , m_wildlife(NULL)
        , m_economyManager(NULL)
        , m_carrierManager(NULL)
        , m_aiSystem(NULL)
        , m_renderer(NULL)
        , m_tileRenderer(NULL)
        , m_camera(NULL)
        , m_inputManager(NULL)
        , m_cursorTileX(0)
        , m_cursorTileY(0)
        , m_buildMenu(NULL)
        , m_menuActive(false)
        , m_buildState(BUILDSTATE_NONE)
        , m_selectedBuilding(World::Building_None)
        , m_placementIconIdx(-1)
        , m_placementConstrIdx(-1)
        , m_flagManager(NULL)
        , m_constructionManager(NULL)
        , m_roadStartX(-1)
        , m_roadStartY(-1)
    {
    }

    GameScene::~GameScene()
    {
        if (m_constructionManager) {
            delete m_constructionManager;
            m_constructionManager = NULL;
        }
        if (m_buildMenu) {
            delete m_buildMenu;
            m_buildMenu = NULL;
        }
        if (m_flagManager) {
            delete m_flagManager;
            m_flagManager = NULL;
        }
    }

    void GameScene::Initialize(IDirect3DDevice9* device, Graphics::SpriteRenderer* spriteRenderer)
    {
        OutputDebugStringA("[GameScene::Initialize] START\n");
        (void)device;
        (void)spriteRenderer;
        
        OutputDebugStringA("[GameScene::Initialize] Creating JobManager\n");
        m_jobManager = new JobManager();
        int processors[] = { 1, 2 };
        m_jobManager->Initialize(2, processors);
        OutputDebugStringA("[GameScene::Initialize] JobManager initialized\n");

        // NOTE: Do NOT create a new Renderer here - it will conflict with the main game renderer
        // The renderer should be set via SetRenderer() method or passed from caller
        OutputDebugStringA("[GameScene::Initialize] NOT creating new Renderer (will use external one)\n");
        m_renderer = NULL;  // Will be set later

        // Tile renderer will be created after map is loaded
        m_tileRenderer = NULL;
        OutputDebugStringA("[GameScene::Initialize] DONE\n");
    }

    void GameScene::Load()
    {
        OutputDebugStringA("[GameScene::Load] START\n");
        
        // Create job manager first (already created in Initialize, but just in case)
        OutputDebugStringA("[GameScene::Load] Creating JobManager if needed\n");
        if (!m_jobManager) {
            m_jobManager = new JobManager();
            int processors[] = { 1, 2 };
            m_jobManager->Initialize(2, processors);
        }
        OutputDebugStringA("[GameScene::Load] JobManager ready\n");

        // Load or create map
        OutputDebugStringA("[GameScene::Load] Loading or creating Map\n");

        // Register atlas texture paths from manifest (same as EditorScene)
        OutputDebugStringA("[GameScene::Load] Loading atlas texture manifest\n");
        TextureRegistry::instance().initializeFromManifest("game:\\Media\\Config\\textures.ini", "AtlasTextures");
        // Manually register Buildings atlas path (may not be in manifest)
        TextureRegistry::instance().registerTexturePath("Buildings", "AtlasTextures\\Buildings.png");
        TextureRegistry::instance().registerTexturePath("streets", "UI\\Streets.png");

        // Peek at file header to get map dimensions before creating the Map object.
        // If loading fails entirely, fall back to a default 20x20 map.
        HANDLE hMapFile = CreateFileA("game:\\Media\\Maps\\slot_01.bin", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        bool fileExists = (hMapFile != INVALID_HANDLE_VALUE);

        int groundW = 20, groundH = 20, otherW = 40, otherH = 80;

        if (fileExists) {
            char magic[4];
            DWORD readBytes;
            int version;
            ReadFile(hMapFile, magic, 4, &readBytes, NULL);
            ReadFile(hMapFile, &version, sizeof(version), &readBytes, NULL);
            if (memcmp(magic, "SMAP", 4) == 0 && version >= 1) {
                ReadFile(hMapFile, &groundW, sizeof(groundW), &readBytes, NULL);
                ReadFile(hMapFile, &groundH, sizeof(groundH), &readBytes, NULL);
                ReadFile(hMapFile, &otherW, sizeof(otherW), &readBytes, NULL);
                ReadFile(hMapFile, &otherH, sizeof(otherH), &readBytes, NULL);
            }
            CloseHandle(hMapFile);
        }

        {
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[GameScene::Load] Creating Map(%d,%d,%d,%d)\n", groundW, groundH, otherW, otherH);
            OutputDebugStringA(dbg);
        }
        m_map = new World::Map(groundW, groundH, otherW, otherH);
        std::vector<std::pair<int,int>> loadedFlags;
        if (fileExists) {
            bool loadOk = MapSerializer::Load(*m_map, "game:\\Media\\Maps\\slot_01.bin", &loadedFlags);
            {
                char dbg[256];
                _snprintf(dbg, sizeof(dbg), "[GameScene::Load] MapSerializer::Load returned %d (flags: %u)\n", loadOk, (unsigned)loadedFlags.size());
                OutputDebugStringA(dbg);
            }
            if (!loadOk)
            {
                OutputDebugStringA("[GameScene::Load] Loading failed, creating default map\n");
                delete m_map;
                m_map = new World::Map(20, 20, 40, 80);
            } else {
                // ─── Initialize unpainted ground tiles ───────────────────────
                World::TileLayer* groundLayer = m_map->GetLayer(World::Ground);
                if (groundLayer) {
                    int gw = groundLayer->GetWidth();
                    int gh = groundLayer->GetHeight();
                    int filled = 0;
                    for (int gy = 0; gy < gh; ++gy) {
                        for (int gx = 0; gx < gw; ++gx) {
                            World::Tile& tile = groundLayer->GetTile(gx, gy);
                            bool needInit = false;
                            if (tile.atlasName.empty()) {
                                tile.atlasName = "ground";
                                needInit = true;
                            }
                            if (tile.type == World::Tile_None) {
                                tile.type = World::Tree;
                                needInit = true;
                            }
                            if (needInit) {
                                if (tile.regionIndex < 0) {
                                    tile.regionIndex = 0;
                                }
                                // Get proper UVs from ground atlas region 0
                                TextureRegistry::instance().getTextureOrLoad("ground");
                                std::tr1::shared_ptr<SpriteAtlas> ga = TextureRegistry::instance().getAtlas("ground");
                                if (ga) {
                                    const SpriteRegion* r = ga->GetRegion(tile.regionIndex >= 0 ? tile.regionIndex : 0);
                                    if (r) {
                                        tile.u0 = r->u0;
                                        tile.v0 = r->v0;
                                        tile.u1 = r->u1;
                                        tile.v1 = r->v1;
                                    } else {
                                        tile.u0 = 0.0f; tile.v0 = 0.0f;
                                        tile.u1 = 1.0f; tile.v1 = 1.0f;
                                    }
                                } else {
                                    tile.u0 = 0.0f; tile.v0 = 0.0f;
                                    tile.u1 = 1.0f; tile.v1 = 1.0f;
                                }
                                filled++;
                            }
                        }
                    }
                    char buf[128];
                    _snprintf(buf, sizeof(buf), "[GameScene::Load] Initialized %d unpainted ground tiles\n", filled);
                    OutputDebugStringA(buf);
                }
            }
        } else {
            OutputDebugStringA("[GameScene::Load] No save file found, creating default map\n");
        }
        OutputDebugStringA("[GameScene::Load] Map ready\n");

        // Set up wildlife system
        OutputDebugStringA("[GameScene::Load] Creating WildlifeSystem\n");
        m_wildlife = new World::WildlifeSystem(m_map);
        m_map->SetWildlifeSystem(m_wildlife);
        OutputDebugStringA("[GameScene::Load] WildlifeSystem ready\n");

        // Set up economy manager and link resource registry to map
        OutputDebugStringA("[GameScene::Load] Creating EconomyManager\n");
        m_economyManager = new Logic::EconomyManager();
        m_map->SetResourceRegistry(&m_economyManager->GetRegistry());
        m_economyManager->GetRegistry().BuildWorldResourceCache(m_map);
        OutputDebugStringA("[GameScene::Load] EconomyManager ready\n");

        // Set up carrier manager
        OutputDebugStringA("[GameScene::Load] Creating CarrierManager\n");
        m_carrierManager = new World::CarrierManager();
        OutputDebugStringA("[GameScene::Load] CarrierManager ready\n");

        // Set up AI system
        OutputDebugStringA("[GameScene::Load] Creating AISystem\n");
        m_aiSystem = new Logic::AISystem(0, m_map, m_economyManager);
        OutputDebugStringA("[GameScene::Load] AISystem ready\n");

        // Create flag manager and load flags from save
        OutputDebugStringA("[GameScene::Load] Creating FlagManager\n");
        m_flagManager = new World::FlagManager();
        if (!loadedFlags.empty()) {
            m_flagManager->LoadFromPairs(loadedFlags);
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[GameScene::Load] Loaded %u flags from save\n", (unsigned)loadedFlags.size());
            OutputDebugStringA(dbg);
        }
        OutputDebugStringA("[GameScene::Load] FlagManager ready\n");

        // Create construction manager
        OutputDebugStringA("[GameScene::Load] Creating ConstructionManager\n");
        m_constructionManager = new World::ConstructionManager();
        OutputDebugStringA("[GameScene::Load] ConstructionManager ready\n");

        // Initialize tile renderer
        OutputDebugStringA("[GameScene::Load] Creating TileRenderer\n");
        if (m_tileRenderer) {
            delete m_tileRenderer;
            m_tileRenderer = NULL;
        }
        m_tileRenderer = new TileRenderer(m_renderer, m_map->GetWidth(), m_map->GetHeight());
        m_tileRenderer->SetMap(m_map);
        // Initialize coordinate system for world↔node conversions (needed before first render)
        CoordinateSystem::GetInstance().Initialize(m_map->GetWidth(), m_map->GetHeight());
        // Set the tile renderer's render queue to the renderer's render queue
        if (m_renderer && m_tileRenderer) {
            m_tileRenderer->SetRenderQueue(m_renderer->GetRenderQueue());
        }
        OutputDebugStringA("[GameScene::Load] TileRenderer ready\n");

        // Create camera
        OutputDebugStringA("[GameScene::Load] Creating Camera\n");
        m_camera = new Camera();
        m_camera->Initialize(1280.0f, 720.0f, m_renderer ? m_renderer->GetShaderManager() : NULL);
        float cx = m_map->GetWidth() * 119.0f;
        float cy = m_map->GetHeight() * 74.0f;
        m_camera->SetPosition(cx, cy);
        m_camera->Update();
        OutputDebugStringA("[GameScene::Load] Camera ready\n");

        // Initialize build menu (must be after UI atlas is loaded)
        OutputDebugStringA("[GameScene::Load] Initializing build menu\n");
        InitBuildMenu();
        OutputDebugStringA("[GameScene::Load] Build menu initialized\n");

        // ─── Fix construction sprite UV on existing tiles ────────────────
        {
            TextureRegistry& reg = TextureRegistry::instance();
            reg.getTextureOrLoad("Buildings");
            std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
            if (buildingsAtlas) {
                uint32_t cIdx = buildingsAtlas->GetIndex("construction");
                if (cIdx == 0xFFFFFFFF) cIdx = buildingsAtlas->GetIndex("Construction");
                if (cIdx == 0xFFFFFFFF) cIdx = buildingsAtlas->GetIndex("ConstructionSite");
                if (cIdx != 0xFFFFFFFF) {
                    World::TileLayer* buildingsLayer = m_map->GetLayer(World::Buildings);
                    if (buildingsLayer) {
                        int fixed = 0;
                        for (int y = 0; y < buildingsLayer->GetHeight(); ++y) {
                            for (int x = 0; x < buildingsLayer->GetWidth(); ++x) {
                                World::Tile& tile = buildingsLayer->GetTile(x, y);
                                if (tile.atlasName == "Buildings" && tile.type != World::Tile_None &&
                                    (static_cast<uint32_t>(tile.regionIndex) == cIdx ||
                                     (tile.u0 == 0.0f && tile.v0 == 0.0f && tile.u1 == 1.0f && tile.v1 == 1.0f)))
                                {
                                    tile.u0 = CONSTRUCTION_U0;
                                    tile.v0 = CONSTRUCTION_V0;
                                    tile.u1 = CONSTRUCTION_U1;
                                    tile.v1 = CONSTRUCTION_V1;
                                    fixed++;
                                }
                            }
                        }
                        {
                            char dbg[256];
                            _snprintf(dbg, sizeof(dbg), "[GameScene] Fixed %d existing construction tile UVs\n", fixed);
                            OutputDebugStringA(dbg);
                        }
                    }
                } else {
                    OutputDebugStringA("[GameScene] WARNING: construction sprite NOT FOUND in Buildings atlas\n");
                }
            }
        }

        // ─── Create starting warehouse + carriers ────────────────────────
        if (m_flagManager && m_economyManager) {
            int hqFlagX = 10, hqFlagY = 10;  // node grid position for warehouse flag
            int hqBuildX = 10, hqBuildY = 8; // building footprint (entrance offset 0,2)

            // Mark the building tile on Buildings layer
            World::TileLayer* buildingsLayer = m_map->GetLayer(World::Buildings);
            if (buildingsLayer && hqBuildX >= 0 && hqBuildX < buildingsLayer->GetWidth() &&
                hqBuildY >= 0 && hqBuildY < buildingsLayer->GetHeight())
            {
                World::Tile& bt = buildingsLayer->GetTile(hqBuildX, hqBuildY);
                bt.type = World::Decoration;
                bt.atlasName = "Buildings";
                bt.walkable = false;
            }

            // Create warehouse flag if no existing flag at position
            World::Flag* hqFlag = m_flagManager->GetFlagAt(hqFlagX, hqFlagY);
            if (!hqFlag) {
                hqFlag = m_flagManager->CreateFlag(hqFlagX, hqFlagY);
            }
            hqFlag->type = World::FLAG_WAREHOUSE;
            hqFlag->hasBuilding = true;

            // Create warehouse and link it
            World::Warehouse* warehouse = new World::Warehouse(hqBuildX, hqBuildY, 0);
            warehouse->state = World::State_Finished;
            warehouse->connectedFlag = hqFlag;
            warehouse->map = m_map;

            // Seed warehouse with starting resources
            warehouse->AddResource(World::ResourceType_Wood, 50);
            warehouse->AddResource(World::ResourceType_Stone, 50);

            m_economyManager->SetWarehouse(warehouse);

            // Spawn a few carriers at the warehouse flag
            for (int ci = 0; ci < 5; ++ci) {
                World::Carrier* carrier = new World::Carrier(hqFlag);
                m_carrierManager->AddCarrier(carrier);
            }

            OutputDebugStringA("[GameScene::Load] Warehouse + carriers created\n");
        }

        OutputDebugStringA("[GameScene::Load] DONE\n");
        m_loaded = true;
    }

void GameScene::Unload()
{
    // Save the map before unloading
    if (m_map) {
        std::vector<std::pair<int,int>> flagPairs;
        if (m_flagManager) {
            flagPairs = m_flagManager->GetFlagPairs();
        }
        MapSerializer::Save(*m_map, "game:\\Media\\Maps\\slot_01.bin", &flagPairs);
    }

    if (m_constructionManager) {
        delete m_constructionManager;
        m_constructionManager = NULL;
    }

    if (m_flagManager) {
        delete m_flagManager;
        m_flagManager = NULL;
    }

    if (m_buildMenu) {
        delete m_buildMenu;
        m_buildMenu = NULL;
    }

    if (m_aiSystem) {
        delete m_aiSystem;
        m_aiSystem = NULL;
    }
    if (m_carrierManager) {
        delete m_carrierManager;
        m_carrierManager = NULL;
    }
    if (m_economyManager) {
        delete m_economyManager;
        m_economyManager = NULL;
    }
    if (m_wildlife) {
        delete m_wildlife;
        m_wildlife = NULL;
    }
    if (m_map) {
        delete m_map;
        m_map = NULL;
    }
    if (m_camera) {
        delete m_camera;
        m_camera = NULL;
    }
    if (m_tileRenderer) {
        delete m_tileRenderer;
        m_tileRenderer = NULL;
    }
    if (m_renderer) {
        delete m_renderer;
        m_renderer = NULL;
    }
    if (m_jobManager) {
        m_jobManager->Shutdown();
        delete m_jobManager;
        m_jobManager = NULL;
    }
    m_loaded = false;
}

void GameScene::Update(float deltaTime)
{
    OutputDebugStringA("[GameScene::Update] START\n");
    if (!m_loaded) {
        OutputDebugStringA("[GameScene::Update] Not loaded, returning\n");
        return;
    }

    // ─── Camera movement and zoom (disabled during menu) ──
    if (!m_menuActive && m_camera && m_inputManager) {
        Input::Gamepad* gamepad = m_inputManager->GetGamepad();
        if (gamepad) {
            float moveSpeed = 500.0f * deltaTime;
            float stickX, stickY;
            gamepad->GetLeftStick(stickX, stickY);
            if (fabsf(stickX) > 0.1f || fabsf(stickY) > 0.1f) {
                m_camera->Move(stickX * moveSpeed, stickY * moveSpeed);
            }
            float rightX, rightY;
            gamepad->GetRightStick(rightX, rightY);
            if (fabsf(rightY) > 0.1f) {
                m_camera->Zoom(rightY * 1.0f * deltaTime);
            }
        }
    }

    // ─── Cursor update ────────────────────────────────────────────────
    UpdateCursor();

    // Auto-update A* road preview when cursor moves during road building
    if (m_buildState == BUILDSTATE_PLACE_ROAD) {
        UpdateRoadPreview(m_cursorTileX, m_cursorTileY);
    }

    // ─── Input handling ──────────────────────────────────────────────
    if (m_inputManager) {
        Input::Gamepad* pad = m_inputManager->GetGamepad();
        if (pad) {
            bool rbPressed = pad->IsButtonPressed(Input::GP_RB);
            bool bPressed = pad->IsButtonPressed(Input::GP_B);
            bool aPressed = pad->IsButtonPressed(Input::GP_A);

            if (m_buildState == BUILDSTATE_PLACE_FLAG) {
                // Flag placement mode: A to place flag with pending building, B to cancel
                if (aPressed) {
                    PlaceFlag(m_cursorTileX, m_cursorTileY);
                } else if (bPressed) {
                    m_buildState = BUILDSTATE_NONE;
                    m_placementIconIdx = -1;
                    m_placementConstrIdx = -1;
                    m_selectedBuilding = World::Building_None;
                    OutputDebugStringA("[GameScene] Flag placement cancelled\n");
                }
            } else if (m_buildState == BUILDSTATE_PLACE_ROAD) {
                // Road building mode: A to commit, B to cancel
                if (aPressed) {
                    CommitRoad();
                    OutputDebugStringA("[GameScene] Road committed\n");
                } else if (bPressed) {
                    CancelRoad();
                    OutputDebugStringA("[GameScene] Road cancelled\n");
                }
            } else if (m_menuActive) {
                // Menu is open
                if (m_buildMenu) {
                    m_buildMenu->Update(pad, deltaTime);

                    if (bPressed) {
                        m_menuActive = false;
                        m_buildMenu->Hide();
                        OutputDebugStringA("[GameScene] Build menu closed\n");
                    }

                    if (m_buildMenu->HasSelection()) {
                        int selIdx = m_buildMenu->GetSelectedSpriteIndex();
                        if (selIdx >= 0) {
                            std::tr1::shared_ptr<SpriteAtlas> uiAtlas = TextureRegistry::instance().getAtlas("ui");
                            if (uiAtlas) {
                                const SpriteRegion* reg = uiAtlas->GetRegion(selIdx);
                                if (reg) {
                                    m_selectedIconName = reg->name;
                                    m_placementIconIdx = selIdx;
                                    // Extract building name from icon name
                                    std::string buildingName = reg->name;
                                    if (buildingName.compare(0, 5, "icon_") == 0) {
                                        buildingName = buildingName.substr(5);
                                    }
                                    // Map icon name to BuildingType
                                    World::BuildingType bt = World::Building_None;
                                    if (buildingName == "Woodcutter") bt = World::Woodcutter;
                                    else if (buildingName == "Sawmill") bt = World::Sawmill;
                                    else if (buildingName == "CoalMine") bt = World::CoalMine;
                                    else if (buildingName == "IronMine") bt = World::IronMine;
                                    else if (buildingName == "GoldMine") bt = World::GoldMine;
                                    else if (buildingName == "IronSmelter") bt = World::IronSmelter;
                                    else if (buildingName == "GoldSmelter") bt = World::GoldSmelter;
                                    else if (buildingName == "Farm") bt = World::Farm;
                                    else if (buildingName == "Mill") bt = World::Mill;
                                    else if (buildingName == "Bakery") bt = World::Bakery;
                                    else if (buildingName == "Fisher") bt = World::Fisher;
                                    else if (buildingName == "Hunter") bt = World::Hunter;
                                    else if (buildingName == "ToolWorkshop") bt = World::ToolWorkshop;
                                    m_selectedBuilding = bt;

                                    // Force-load Buildings atlas for construction sprite lookup
                                    TextureRegistry& tr = TextureRegistry::instance();
                                    tr.getTextureOrLoad("Buildings");
                                    std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = tr.getAtlas("Buildings");
                                    if (buildingsAtlas) {
                                        uint32_t cIdx = buildingsAtlas->GetIndex("construction");
                                        if (cIdx == 0xFFFFFFFF) cIdx = buildingsAtlas->GetIndex("Construction");
                                        if (cIdx == 0xFFFFFFFF) cIdx = buildingsAtlas->GetIndex("ConstructionSite");
                                        if (cIdx != 0xFFFFFFFF) {
                                            m_placementConstrIdx = (int)cIdx;
                                        } else {
                                            m_placementConstrIdx = 0;
                                            OutputDebugStringA("[GameScene] WARNING: 'construction' not found in Buildings atlas, using index 0\n");
                                        }
                                    }
                                    // Enter flag-placement mode
                                    m_buildState = BUILDSTATE_PLACE_FLAG;
                                    OutputDebugStringA("[GameScene] Entered flag-placement mode\n");
                                }
                            }
                        }
                        m_menuActive = false;
                        m_buildMenu->Hide();
                        m_buildMenu->ResetSelection();
                    }
                }
            } else {
                // Normal mode: RB to open build menu, A on flag to start road
                if (aPressed) {
                    if (m_flagManager) {
                        World::Flag* flag = m_flagManager->GetFlagAt(m_cursorTileX, m_cursorTileY);
                        if (flag) {
                            StartRoad(m_cursorTileX, m_cursorTileY);
                        }
                    }
                } else if (rbPressed && m_buildMenu) {
                    m_menuActive = true;
                    m_buildMenu->ResetSelection();
                    m_buildMenu->Show(640.0f, 360.0f);
                    OutputDebugStringA("[GameScene] Build menu opened\n");
                }
            }
        }
    }

    // ─── Phase A: Economy ∥ Wildlife sectors ───────────────────────────
    m_economyJobData.economy = m_economyManager;
    m_economyJobData.carriers = m_carrierManager;

    // Generate construction resource requests before economy processes them
    if (m_constructionManager && m_economyManager) {
        m_constructionManager->GenerateRequests(m_economyManager);
    }

    m_jobManager->Submit(EconomyJobFunc, &m_economyJobData);

    {
        bool doWildlifeSpawn = m_wildlife && m_wildlife->ShouldSpawn(deltaTime);
        if (doWildlifeSpawn)
        {
            int totalSpawners = m_wildlife ? m_wildlife->GetSpawnerCount() : 0;
            if (totalSpawners > 0)
            {
                int spawnersPerSector = totalSpawners / 4;
                int sectorStart = 0;
                for (int i = 0; i < 4; ++i)
                {
                    m_wildlifeSectors[i].wildlife = m_wildlife;
                    m_wildlifeSectors[i].startSpawner = sectorStart;
                    m_wildlifeSectors[i].endSpawner = (i == 3) ? totalSpawners : sectorStart + spawnersPerSector;
                    m_wildlifeSectors[i].newAnimals.clear();
                    m_jobManager->Submit(WildlifeSectorFunc, &m_wildlifeSectors[i]);
                    sectorStart = m_wildlifeSectors[i].endSpawner;
                }
            }
        }

        m_jobManager->WaitAll();

        // Merge wildlife spawns from sector buffers
        if (doWildlifeSpawn && m_wildlife)
        {
            for (int i = 0; i < 4; ++i)
                m_wildlife->AddAnimals(m_wildlifeSectors[i].newAnimals);
        }

        // ─── Phase B1: Carrier sort + assign (single-threaded) ──────────────
        m_jobManager->Submit(CarrierAssignFunc, m_carrierManager);
        m_jobManager->WaitAll();

        // ─── Phase B2: Carrier updates ∥ AI analysis ────────────────────────
        int numCarriers = m_carrierManager ? m_carrierManager->GetCarrierCount() : 0;
        if (numCarriers > 0)
        {
            int carriersPerRange = numCarriers / 4;
            if (carriersPerRange < 1) carriersPerRange = 1;
            int rangeStart = 0;
            for (int i = 0; i < 4; ++i)
            {
                m_carrierRanges[i].mgr = m_carrierManager;
                m_carrierRanges[i].startCarrier = rangeStart;
                m_carrierRanges[i].endCarrier = (i == 3) ? numCarriers : rangeStart + carriersPerRange;
                m_carrierRanges[i].dt = deltaTime;
                m_jobManager->Submit(CarrierUpdateFunc, &m_carrierRanges[i]);
                rangeStart = m_carrierRanges[i].endCarrier;
                if (rangeStart >= numCarriers) break;
            }
        }

        // ─── Construction manager update ───────────────────────────────
        if (m_constructionManager) {
            m_constructionManager->Update(deltaTime);
            // Check for completed construction sites
            const std::vector<World::ConstructionSite*>& sites = m_constructionManager->GetAllSites();
            for (size_t ci = 0; ci < sites.size(); ++ci) {
                if (sites[ci]->IsComplete()) {
                    ConfirmConstruction(sites[ci]->flag);
                }
            }
        }

        // ─── AI chunks — read-only PlanBuild, use reservations + cache ───────
        m_aiSystem->ClearReservations();

        m_aiChunks[0].ai = m_aiSystem;
        m_aiChunks[0].types[0] = World::Woodcutter;
        m_aiChunks[0].types[1] = World::Sawmill;
        m_aiChunks[0].types[2] = World::CoalMine;
        m_aiChunks[0].numTypes = 3;
        m_aiChunks[0].numRequests = 0;

        m_aiChunks[1].ai = m_aiSystem;
        m_aiChunks[1].types[0] = World::IronMine;
        m_aiChunks[1].types[1] = World::IronSmelter;
        m_aiChunks[1].types[2] = World::ToolWorkshop;
        m_aiChunks[1].numTypes = 3;
        m_aiChunks[1].numRequests = 0;

        m_aiChunks[2].ai = m_aiSystem;
        m_aiChunks[2].types[0] = World::Farm;
        m_aiChunks[2].types[1] = World::Mill;
        m_aiChunks[2].types[2] = World::Bakery;
        m_aiChunks[2].numTypes = 3;
        m_aiChunks[2].numRequests = 0;

        m_aiChunks[3].ai = m_aiSystem;
        m_aiChunks[3].types[0] = World::Hunter;
        m_aiChunks[3].types[1] = World::Fisher;
        m_aiChunks[3].types[2] = World::GoldMine;
        m_aiChunks[3].types[3] = World::GoldSmelter;
        m_aiChunks[3].numTypes = 4;
        m_aiChunks[3].numRequests = 0;

        m_jobManager->Submit(AIChunkJobFunc, &m_aiChunks[0]);
        m_jobManager->Submit(AIChunkJobFunc, &m_aiChunks[1]);
        m_jobManager->Submit(AIChunkJobFunc, &m_aiChunks[2]);
        m_jobManager->Submit(AIChunkJobFunc, &m_aiChunks[3]);
        m_jobManager->WaitAll();

        // ─── Phase C: Apply Build Commands (single-threaded) ───────────────────
        for (int c = 0; c < 4; ++c)
            m_aiSystem->ApplyBuildRequests(m_aiChunks[c].requests, m_aiChunks[c].numRequests);
        
        OutputDebugStringA("[GameScene::Update] DONE\n");
    }
}

void GameScene::Render(Graphics::RenderQueue* renderQueue)
{
//    OutputDebugStringA("[GameScene::Render] START\n");
    if (!m_loaded || !m_tileRenderer || !m_map) {
        OutputDebugStringA("[GameScene::Render] Not ready, returning\n");
        return;
    }

     if (m_renderer) {
         m_renderer->Clear(0xFF000000); // Black
     }

    // Set the render queue for the tile renderer (optional, if it caches it)
    m_tileRenderer->SetRenderQueue(renderQueue);

    // ─── Set up atlas texture slots ────────────────────────────────────────
//    OutputDebugStringA("[GameScene::Render] Setting up texture slots\n");
    m_tileRenderer->ClearAtlasSlots();
    TextureRegistry& reg = TextureRegistry::instance();
    SpriteRenderer* spriteRenderer = m_renderer ? m_renderer->GetSpriteRenderer() : NULL;

    if (spriteRenderer) {
        WORD nextSlot = 1;
        // Scan all layers for unique atlas names
        for (int lt = 0; lt < static_cast<int>(World::LayerCount); ++lt) {
            World::TileLayer* layer = m_map->GetLayer(static_cast<World::LayerType>(lt));
            if (!layer) continue;
            int w = layer->GetWidth();
            int h = layer->GetHeight();
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    const World::Tile& tile = layer->GetTile(x, y);
                    if (tile.atlasName.empty()) continue;
                    if (m_tileRenderer->HasAtlasSlot(tile.atlasName)) continue;
                    // Try to load atlas via texture registry (loads .bin file on demand)
                    LPDIRECT3DTEXTURE9 tex = reg.getTextureOrLoad(tile.atlasName);
                    if (!tex) continue;
                    // Use atlas texture if available, otherwise raw texture
                    LPDIRECT3DTEXTURE9 slotTex = tex;
                    std::tr1::shared_ptr<SpriteAtlas> atlas = reg.getAtlas(tile.atlasName);
                    if (atlas && atlas->GetTexture()) {
                        slotTex = atlas->GetTexture();
                    }
                    spriteRenderer->SetTextureSlot(nextSlot, slotTex);
                    m_tileRenderer->SetAtlasSlot(tile.atlasName, nextSlot);
                    nextSlot++;
                }
            }
        }
    }
//    OutputDebugStringA("[GameScene::Render] Texture slots ready\n");

    // Set up camera view-projection matrices for world-space rendering
    if (m_camera) {
        m_camera->Update();
        D3DXMATRIX viewProj = m_camera->GetViewMatrix() * m_camera->GetProjectionMatrix();
        if (m_renderer) {
            Graphics::ShaderManager* sm = m_renderer->GetShaderManager();
            if (sm) {
                sm->UpdateGlobalMatrices(&m_camera->GetViewMatrix(), &m_camera->GetProjectionMatrix());
                sm->SetShaderMatrix(SHADER_TERRAIN, &viewProj);
                sm->SetShaderMatrix(SHADER_WORLD, &viewProj);
            }
        }
    }

    // Re-assert UI atlas texture slots for cursor & menu (after TileRenderer slot assignment)
    if (spriteRenderer) {
        std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
        if (uiAtlas && uiAtlas->GetTexture()) {
            LPDIRECT3DTEXTURE9 uiTex = uiAtlas->GetTexture();
            spriteRenderer->SetTextureSlot(SLOT_UI_CURSOR, uiTex);
            spriteRenderer->SetTextureSlot(SLOT_UI_MENU_BG, uiTex);
            spriteRenderer->SetTextureSlot(SLOT_UI_MENU_CELL, uiTex);
            spriteRenderer->SetTextureSlot(SLOT_UI_MENU_ICON, uiTex);
        }
    }

//    OutputDebugStringA("[GameScene::Render] Rendering map\n");
    m_tileRenderer->RenderMap();
//    OutputDebugStringA("[GameScene::Render] Map rendered\n");

    // ─── Render cursor or placement preview ─────────────────────────────
    if (m_buildState == BUILDSTATE_PLACE_FLAG && m_placementIconIdx >= 0) {
        // Calculate building footprint position (offset from cursor using entrance offset)
        std::string buildingName = m_selectedIconName;
        if (buildingName.compare(0, 5, "icon_") == 0) {
            buildingName = buildingName.substr(5);
        }
        int entranceX = 0, entranceY = 0;
        GetEntranceOffset(buildingName, entranceX, entranceY);
        int buildX = m_cursorTileX - entranceX;
        int buildY = m_cursorTileY - entranceY;

        std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
        if (uiAtlas) {
            const SpriteRegion* iconRegion = uiAtlas->GetRegion(m_placementIconIdx);
            if (iconRegion) {
                float wx, wy;
                CoordinateSystem::GetInstance().NodeTileToWorld(buildX, buildY, wx, wy);

                if (spriteRenderer) {
                    LPDIRECT3DTEXTURE9 uiTex = uiAtlas->GetTexture();
                    if (uiTex) spriteRenderer->SetTextureSlot(SLOT_UI_CURSOR, uiTex);
                }

                Graphics::RenderCommand pcmd = {};
                pcmd.x = wx;
                pcmd.y = wy;
                pcmd.width = (float)iconRegion->width;
                pcmd.height = (float)iconRegion->height;
                pcmd.u0 = iconRegion->u0;
                pcmd.v0 = iconRegion->v0;
                pcmd.u1 = iconRegion->u1;
                pcmd.v1 = iconRegion->v1;
                pcmd.color = 0xAAFFFFFF;
                pcmd.textureID = SLOT_UI_CURSOR;
                pcmd.shaderID = SHADER_TERRAIN;
                pcmd.blendMode = 1;
                pcmd.layer = LAYER_FOREGROUND;
                pcmd.depth = static_cast<WORD>(0.98f * 65535.0f);
                renderQueue->Submit(pcmd);
            }
        }
//        OutputDebugStringA("[GameScene::Render] Placement preview rendered\n");
    } else if (!m_menuActive) {
        RenderCursor(renderQueue);
//        OutputDebugStringA("[GameScene::Render] Cursor rendered\n");
    }

    // ─── Render flags ───────────────────────────────────────────────────
    if (m_flagManager && !m_flagManager->GetFlagPairs().empty() && spriteRenderer) {
        std::tr1::shared_ptr<SpriteAtlas> streetsAtlas = reg.getAtlas("streets");
        if (streetsAtlas && streetsAtlas->GetTexture()) {
            LPDIRECT3DTEXTURE9 streetsTex = streetsAtlas->GetTexture();
            spriteRenderer->SetTextureSlot(SLOT_STREETS, streetsTex);

            const std::vector<uint32_t>* flagGroup = streetsAtlas->GetGroup("FlagStreets");
            if (flagGroup && !flagGroup->empty()) {
                uint32_t flagIdx = (*flagGroup)[0];
                const SpriteRegion* flagRegion = streetsAtlas->GetRegion(flagIdx);
                if (flagRegion) {
                    CoordinateSystem& coords = CoordinateSystem::GetInstance();
                    const std::vector<std::pair<int,int>>& pairs = m_flagManager->GetFlagPairs();
                    for (size_t fi = 0; fi < pairs.size(); ++fi) {
                        int fx = pairs[fi].first;
                        int fy = pairs[fi].second;
                        float wx, wy;
                        coords.NodeTileToWorld(fx, fy, wx, wy);
                        Graphics::RenderCommand cmd = {};
                        cmd.x = wx - flagRegion->pivotX;
                        cmd.y = wy - flagRegion->pivotY;
                        cmd.width = (float)flagRegion->width;
                        cmd.height = (float)flagRegion->height;
                        cmd.u0 = flagRegion->u0;
                        cmd.v0 = flagRegion->v0;
                        cmd.u1 = flagRegion->u1;
                        cmd.v1 = flagRegion->v1;
                        cmd.color = 0xFFFFFFFF;
                        cmd.textureID = SLOT_STREETS;
                        cmd.shaderID = SHADER_TERRAIN;
                        cmd.blendMode = 1;
                        cmd.layer = LAYER_WORLD;
                        cmd.depth = static_cast<WORD>(30010 + fy * 400);
                        renderQueue->Submit(cmd);
                    }
                }
            }
        }
    }

    // ─── Render road preview (A* path) ─────────────────────────────────
    if (m_buildState == BUILDSTATE_PLACE_ROAD && !m_roadPreviewPath.empty()) {
        std::tr1::shared_ptr<SpriteAtlas> streetsAtlas = reg.getAtlas("streets");
        if (streetsAtlas) {
            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
            for (size_t i = 1; i < m_roadPreviewPath.size(); ++i) {
                int px = m_roadPreviewPath[i].first;
                int py = m_roadPreviewPath[i].second;
                int pattern = CalcPatternAt(px, py, roadsLayer, m_roadPreviewPath);

                char groupBuf[16];
                const char* groupName = groupBuf;
                switch (pattern) {
                    case 0:  groupName = "street_1"; break;
                    case 1:  groupName = "street_1"; break;
                    case 2:  groupName = "street_2"; break;
                    case 3:  _snprintf(groupBuf, sizeof(groupBuf), "street_%d", 3); break;
                    case 4:  groupName = "street_1"; break;
                    case 5:  groupName = "street_5"; break;
                    case 6:  _snprintf(groupBuf, sizeof(groupBuf), "street_%d", 6); break;
                    case 7:  _snprintf(groupBuf, sizeof(groupBuf), "street_%d", 7); break;
                    case 8:  groupName = "street_2"; break;
                    case 9:  _snprintf(groupBuf, sizeof(groupBuf), "street_%d", 9); break;
                    case 10: groupName = "street_2"; break;
                    case 11: _snprintf(groupBuf, sizeof(groupBuf), "street_%d", 11); break;
                    case 12: _snprintf(groupBuf, sizeof(groupBuf), "street_%d", 12); break;
                    case 13: _snprintf(groupBuf, sizeof(groupBuf), "street_%d", 13); break;
                    case 14: _snprintf(groupBuf, sizeof(groupBuf), "street_%d", 14); break;
                    case 15: _snprintf(groupBuf, sizeof(groupBuf), "street_%d", 15); break;
                }

                const std::vector<uint32_t>* group = streetsAtlas->GetGroup(groupName);
                if (!group || group->empty()) {
                    group = streetsAtlas->GetGroup("street_1");
                    if (!group || group->empty()) continue;
                }

                uint32_t regionIdx = (*group)[0];
                const SpriteRegion* region = streetsAtlas->GetRegion(regionIdx);
                if (!region) continue;

                float wx, wy;
                coords.NodeTileToWorld(px, py, wx, wy);

                Graphics::RenderCommand cmd = {};
                cmd.x = wx - region->pivotX;
                cmd.y = wy - region->pivotY;
                cmd.width = (float)region->width;
                cmd.height = (float)region->height;
                cmd.u0 = region->u0;
                cmd.v0 = region->v0;
                cmd.u1 = region->u1;
                cmd.v1 = region->v1;
                cmd.color = D3DCOLOR_ARGB(160, 255, 255, 255);
                cmd.textureID = SLOT_STREETS;
                cmd.shaderID = SHADER_TERRAIN;
                cmd.blendMode = 1;
                cmd.layer = LAYER_WORLD;
                cmd.depth = static_cast<WORD>(30000 + py * 400);
                renderQueue->Submit(cmd);
            }
        }
    }

    // ─── Render build menu (if active) ───────────────────────────────────
    if (m_buildMenu && m_menuActive) {
        m_buildMenu->Render();
//        OutputDebugStringA("[GameScene::Render] Build menu rendered\n");
    }

//    OutputDebugStringA("[GameScene::Render] DONE\n");
}

    // ─── Cursor & Interaction ────────────────────────────────────────────────

    void GameScene::UpdateCursor()
    {
        if (!m_camera || !m_map) return;

        float worldCX, worldCY;
        m_camera->GetWorldCenter(worldCX, worldCY);

        CoordinateSystem::GetInstance().WorldToNodeTile(worldCX, worldCY, m_cursorTileX, m_cursorTileY);

        int nodesW = CoordinateSystem::GetInstance().GetNodesWidth();
        int nodesH = CoordinateSystem::GetInstance().GetNodesHeight();
        if (m_cursorTileX < 0) m_cursorTileX = 0;
        if (m_cursorTileX >= nodesW) m_cursorTileX = nodesW - 1;
        if (m_cursorTileY < 0) m_cursorTileY = 0;
        if (m_cursorTileY >= nodesH) m_cursorTileY = nodesH - 1;
    }

    void GameScene::RenderCursor(Graphics::RenderQueue* renderQueue)
    {
        TextureRegistry& reg = TextureRegistry::instance();
        std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
        if (!uiAtlas) return;

        uint32_t cursorIdx = uiAtlas->GetIndex("cursor");
        if (cursorIdx == 0xFFFFFFFF) return;

        const SpriteRegion* cursorRegion = uiAtlas->GetRegion(cursorIdx);
        if (!cursorRegion) return;

        // Re-assert UI atlas texture on cursor slot (in case TileRenderer overwrote it)
        SpriteRenderer* spriteRenderer = m_renderer ? m_renderer->GetSpriteRenderer() : NULL;
        if (spriteRenderer) {
            LPDIRECT3DTEXTURE9 uiTex = uiAtlas->GetTexture();
            if (uiTex) spriteRenderer->SetTextureSlot(SLOT_UI_CURSOR, uiTex);
        }

        float worldX, worldY;
        CoordinateSystem::GetInstance().NodeTileToWorld(m_cursorTileX, m_cursorTileY, worldX, worldY);

        float cursorW = CoordinateSystem::GetInstance().GetNodeWidth();
        float cursorH = CoordinateSystem::GetInstance().GetNodeHeight();

        Graphics::RenderCommand cmd = {};
        cmd.x = worldX;
        cmd.y = worldY;
        cmd.width = cursorW;
        cmd.height = cursorH;
        cmd.u0 = cursorRegion->u0;
        cmd.v0 = cursorRegion->v0;
        cmd.u1 = cursorRegion->u1;
        cmd.v1 = cursorRegion->v1;
        cmd.color = 0xFFFFFFFF;
        cmd.textureID = SLOT_UI_CURSOR;
        cmd.shaderID = SHADER_TERRAIN;
        cmd.blendMode = 1;
        cmd.layer = LAYER_FOREGROUND;
        cmd.depth = static_cast<WORD>(0.99f * 65535.0f);
        renderQueue->Submit(cmd);
    }

    void GameScene::InitBuildMenu()
    {
        OutputDebugStringA("[GameScene::InitBuildMenu] START\n");

        m_buildMenu = new GridMenu();
        m_buildMenu->Initialize();
        m_buildMenu->SetRenderQueue(m_renderer ? m_renderer->GetRenderQueue() : NULL);
        SpriteRenderer* sr = m_renderer ? m_renderer->GetSpriteRenderer() : NULL;
        m_buildMenu->SetSpriteRenderer(sr);
        m_buildMenu->SetRenderer(m_renderer);

        TextureRegistry& reg = TextureRegistry::instance();
        std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
        if (!uiAtlas) {
            OutputDebugStringA("[GameScene] WARNING: UI atlas not found for build menu\n");
            return;
        }

        LPDIRECT3DTEXTURE9 uiTex = uiAtlas->GetTexture();
        if (!uiTex) {
            OutputDebugStringA("[GameScene] WARNING: UI atlas has no texture\n");
            return;
        }

        if (sr) {
            sr->SetTextureSlot(SLOT_UI_MENU_BG, uiTex);
            sr->SetTextureSlot(SLOT_UI_MENU_CELL, uiTex);
            sr->SetTextureSlot(SLOT_UI_MENU_ICON, uiTex);
        }

        m_buildMenu->SetTextureSlots(SLOT_UI_MENU_BG, SLOT_UI_MENU_CELL, SLOT_UI_MENU_ICON);
        m_buildMenu->SetTextures(uiTex, uiTex, uiTex);

        GridMenu::TileUV bgUV = {0,0,1,1}, cellUV = {0,0,1,1};
        uint32_t bgIdx = uiAtlas->GetIndex("menu_Grid");
        if (bgIdx != 0xFFFFFFFF) {
            const SpriteRegion* r = uiAtlas->GetRegion(bgIdx);
            if (r) { bgUV.u0 = r->u0; bgUV.v0 = r->v0; bgUV.u1 = r->u1; bgUV.v1 = r->v1; }
        } else {
            OutputDebugStringA("[GameScene] WARNING: 'menu_Grid' NOT FOUND in UI atlas\n");
        }
        uint32_t cellIdx = uiAtlas->GetIndex("menu_cell");
        if (cellIdx != 0xFFFFFFFF) {
            const SpriteRegion* r = uiAtlas->GetRegion(cellIdx);
            if (r) { cellUV.u0 = r->u0; cellUV.v0 = r->v0; cellUV.u1 = r->u1; cellUV.v1 = r->v1; }
        } else {
            OutputDebugStringA("[GameScene] WARNING: 'menu_cell' NOT FOUND in UI atlas\n");
        }
        m_buildMenu->SetBackgroundUV(bgUV);
        m_buildMenu->SetCellUV(cellUV);

        // Load IconBasicBuilding group from UI atlas
        std::vector<GridMenu::TileUV> tileUVs;
        std::vector<int> spriteIndices;
        std::vector<std::string> cellLabels;

        const std::vector<uint32_t>* groupIndices = uiAtlas->GetGroup("IconBasicBuilding");
        if (groupIndices && !groupIndices->empty()) {
            for (size_t gi = 0; gi < groupIndices->size(); ++gi) {
                uint32_t spriteIdx = (*groupIndices)[gi];
                const SpriteRegion* reg = uiAtlas->GetRegion(spriteIdx);
                if (!reg) continue;
                GridMenu::TileUV uv;
                uv.u0 = reg->u0; uv.v0 = reg->v0;
                uv.u1 = reg->u1; uv.v1 = reg->v1;
                tileUVs.push_back(uv);
                spriteIndices.push_back((int)spriteIdx);
                // Use part after "icon_" as label, or full name
                std::string label = reg->name;
                if (label.compare(0, 5, "icon_") == 0) label = label.substr(5);
                cellLabels.push_back(label);
            }
        }

        {
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[GameScene] Found %d sprites in IconBasicBuilding group\n", (int)tileUVs.size());
            OutputDebugStringA(dbg);
        }

        if (tileUVs.empty()) {
            OutputDebugStringA("[GameScene] IconBasicBuilding group empty, skipping menu setup\n");
            return;
        }

        m_buildMenu->SetCellLabels(cellLabels);
        m_buildMenu->SetCellSpacing(80.0f, 80.0f);
        m_buildMenu->SetCellPadding(4.0f);
        m_buildMenu->SetCellVisualSize(64.0f, 64.0f);

        m_buildMenu->SetTileData(tileUVs, spriteIndices);

        OutputDebugStringA("[GameScene::InitBuildMenu] DONE\n");
    }

    void GameScene::GetEntranceOffset(const std::string& buildingName, int& outX, int& outY)
    {
        outX = 0; outY = 0;
        TextureRegistry& reg = TextureRegistry::instance();
        reg.getTextureOrLoad("Buildings");
        std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
        if (!buildingsAtlas) return;

        uint32_t idx = buildingsAtlas->GetIndex(buildingName.c_str());
        if (idx == 0xFFFFFFFF) {
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[GameScene] GetEntranceOffset: '%s' not found in Buildings atlas\n", buildingName.c_str());
            OutputDebugStringA(dbg);
            return;
        }
        const SpriteRegion* r = buildingsAtlas->GetRegion(idx);
        if (r) {
            outX = r->entranceX;
            outY = r->entranceY;
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[GameScene] GetEntranceOffset: '%s' entrance=(%d,%d)\n",
                buildingName.c_str(), outX, outY);
            OutputDebugStringA(dbg);
        }
    }

    bool GameScene::CanPlaceBuilding(World::BuildingType type, int buildX, int buildY)
    {
        if (!m_map) return false;

        // Check Buildings layer — must be empty
        World::TileLayer* buildingsLayer = m_map->GetLayer(World::Buildings);
        if (buildingsLayer) {
            if (buildX < 0 || buildX >= buildingsLayer->GetWidth() || buildY < 0 || buildY >= buildingsLayer->GetHeight())
                return false;
            const World::Tile& bt = buildingsLayer->GetTile(buildX, buildY);
            if (bt.type != World::Tile_None)
                return false;
        }

        // Check Roads layer — must not have a road (actual road has regionIndex>=0 + atlasName=="streets")
        World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
        if (roadsLayer) {
            const World::Tile& rt = roadsLayer->GetTile(buildX, buildY);
            if (rt.regionIndex >= 0 && rt.atlasName == "streets")
                return false;
        }

        // Check Objects layer — must be empty
        World::TileLayer* objectsLayer = m_map->GetLayer(World::Objects);
        if (objectsLayer) {
            const World::Tile& ot = objectsLayer->GetTile(buildX, buildY);
            if (ot.u1 > ot.u0 && ot.v1 > ot.v0)
                return false;
        }

        // Check for existing flag at footprint
        if (m_flagManager) {
            if (m_flagManager->GetFlagAt(buildX, buildY))
                return false;
        }

        (void)type; // for future multi-tile footprint checks
        return true;
    }

    void GameScene::PlaceFlag(int tileX, int tileY)
    {
        if (!m_flagManager || !m_map) return;
        if (m_selectedBuilding == World::Building_None) return;

        char dbg[256];
        _snprintf(dbg, sizeof(dbg), "[GameScene] PlaceFlag at (%d,%d) for building type %d\n",
            tileX, tileY, (int)m_selectedBuilding);
        OutputDebugStringA(dbg);

        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        int nodesW = coords.GetNodesWidth();
        int nodesH = coords.GetNodesHeight();
        if (tileX < 0 || tileX >= nodesW || tileY < 0 || tileY >= nodesH) return;

        // Get entrance offset for the building
        std::string buildingName = m_selectedIconName;
        if (buildingName.compare(0, 5, "icon_") == 0) {
            buildingName = buildingName.substr(5);
        }
        int entranceX = 0, entranceY = 0;
        GetEntranceOffset(buildingName, entranceX, entranceY);

        // Calculate building footprint position (flag position minus entrance offset)
        int buildX = tileX - entranceX;
        int buildY = tileY - entranceY;

        // Validate building footprint with CanPlaceBuilding
        if (!CanPlaceBuilding(m_selectedBuilding, buildX, buildY)) {
            OutputDebugStringA("[GameScene] PlaceFlag: cannot place building at footprint\n");
            return;
        }

        // Check flag position node weight (not building position — the flag goes on the road/entrance)
        BYTE flagWeight = m_map->GetNodeWeight(tileX, tileY);
        if (flagWeight == World::Weight_Deep) {
            OutputDebugStringA("[GameScene] PlaceFlag: flag position is deep water\n");
            return;
        }

        // Prevent placing on existing flag
        if (m_flagManager->GetFlagAt(tileX, tileY)) {
            OutputDebugStringA("[GameScene] PlaceFlag: flag already exists at position\n");
            return;
        }

        // Create the flag with Building type
        World::Flag* flag = m_flagManager->CreateFlag(tileX, tileY);
        flag->type = World::FLAG_BUILDING;
        flag->pendingBuilding = m_selectedBuilding;
        flag->hasBuilding = false;

        // Create the construction site object and tile
        CreateConstructionSite(flag, buildX, buildY);

        // Reset build state
        m_buildState = BUILDSTATE_NONE;
        m_placementIconIdx = -1;
        m_placementConstrIdx = -1;
        m_selectedBuilding = World::Building_None;

        _snprintf(dbg, sizeof(dbg), "[GameScene] Flag placed at (%d,%d) -> building at (%d,%d), entrance offset (%d,%d)\n",
            tileX, tileY, buildX, buildY, entranceX, entranceY);
        OutputDebugStringA(dbg);
    }

    void GameScene::CreateConstructionSite(World::Flag* flag, int siteX, int siteY)
    {
        if (!flag || !m_map) return;

        char dbg[256];
        _snprintf(dbg, sizeof(dbg), "[GameScene] CreateConstructionSite at (%d,%d) for flag at (%d,%d)\n",
            siteX, siteY, flag->pos.x, flag->pos.y);
        OutputDebugStringA(dbg);

        World::TileLayer* buildingsLayer = m_map->GetLayer(World::Buildings);
        if (!buildingsLayer || siteX < 0 || siteX >= buildingsLayer->GetWidth() || siteY < 0 || siteY >= buildingsLayer->GetHeight()) {
            OutputDebugStringA("[GameScene] CreateConstructionSite: invalid coordinates\n");
            return;
        }

        // Create the ConstructionSite object
        World::ConstructionSite* site = new World::ConstructionSite(siteX, siteY, flag->pendingBuilding, flag);
        if (m_constructionManager) {
            m_constructionManager->AddSite(site);
        }

        // Set the sprite tile on the Buildings layer
        World::Tile& tile = buildingsLayer->GetTile(siteX, siteY);
        tile.atlasName = "Buildings";
        tile.type = World::Decoration;
        tile.regionIndex = m_placementConstrIdx >= 0 ? m_placementConstrIdx : 0;
        tile.walkable = true;

        TextureRegistry& reg = TextureRegistry::instance();
        reg.getTextureOrLoad("Buildings");
        std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");

        if (m_placementConstrIdx >= 0 && buildingsAtlas) {
            const SpriteRegion* r = buildingsAtlas->GetRegion((uint32_t)m_placementConstrIdx);
            if (r) {
                tile.u0 = r->u0; tile.v0 = r->v0;
                tile.u1 = r->u1; tile.v1 = r->v1;
                tile.u0 = CONSTRUCTION_U0;
                tile.v0 = CONSTRUCTION_V0;
                tile.u1 = CONSTRUCTION_U1;
                tile.v1 = CONSTRUCTION_V1;
            } else {
                tile.u0 = CONSTRUCTION_U0; tile.v0 = CONSTRUCTION_V0;
                tile.u1 = CONSTRUCTION_U1; tile.v1 = CONSTRUCTION_V1;
            }
        } else {
            tile.u0 = CONSTRUCTION_U0; tile.v0 = CONSTRUCTION_V0;
            tile.u1 = CONSTRUCTION_U1; tile.v1 = CONSTRUCTION_V1;
        }

        _snprintf(dbg, sizeof(dbg), "[GameScene] ConstructionSite created at (%d,%d) type=%d wood=%d stone=%d\n",
            siteX, siteY, (int)flag->pendingBuilding, site->woodNeeded, site->stoneNeeded);
        OutputDebugStringA(dbg);
    }

    const char* GameScene::GetBuildingName(World::BuildingType type) const
    {
        switch (type) {
            case World::BuildingType::Woodcutter:     return "Woodcutter";
            case World::BuildingType::Sawmill:        return "Sawmill";
            case World::BuildingType::CoalMine:       return "CoalMine";
            case World::BuildingType::IronMine:       return "IronMine";
            case World::BuildingType::GoldMine:       return "GoldMine";
            case World::BuildingType::IronSmelter:    return "IronSmelter";
            case World::BuildingType::GoldSmelter:    return "GoldSmelter";
            case World::BuildingType::Farm:           return "Farm";
            case World::BuildingType::Mill:           return "Mill";
            case World::BuildingType::Bakery:         return "Bakery";
            case World::BuildingType::Fisher:         return "Fisher";
            case World::BuildingType::Hunter:         return "Hunter";
            case World::BuildingType::ToolWorkshop:   return "ToolWorkshop";
            default:                    return "";
        }
    }

    void GameScene::ConfirmConstruction(World::Flag* flag)
    {
        if (!flag || !m_map) return;

        World::ConstructionSite* site = m_constructionManager ? m_constructionManager->GetSiteForFlag(flag) : NULL;
        if (!site) return;

        World::TileLayer* buildingsLayer = m_map->GetLayer(World::Buildings);
        if (buildingsLayer) {
            const char* buildingName = GetBuildingName(site->buildingType);

            TextureRegistry& reg = TextureRegistry::instance();
            reg.getTextureOrLoad("Buildings");
            std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");

            if (buildingsAtlas && buildingName[0] != '\0') {
                uint32_t spriteIdx = buildingsAtlas->GetIndex(buildingName);
                if (spriteIdx != 0xFFFFFFFF) {
                    const SpriteRegion* r = buildingsAtlas->GetRegion(spriteIdx);
                    if (r) {
                        World::Tile& tile = buildingsLayer->GetTile(site->x, site->y);
                        tile.atlasName = "Buildings";
                        tile.type = World::Decoration;
                        tile.regionIndex = (int)spriteIdx;
                        tile.u0 = r->u0; tile.v0 = r->v0;
                        tile.u1 = r->u1; tile.v1 = r->v1;
                        tile.walkable = true;
                    }
                } else {
                    World::Tile& tile = buildingsLayer->GetTile(site->x, site->y);
                    tile.regionIndex = -1;
                    tile.type = World::Tile_None;
                }
            } else {
                World::Tile& tile = buildingsLayer->GetTile(site->x, site->y);
                tile.regionIndex = -1;
                tile.type = World::Tile_None;
            }
        }

        // Create concrete building object via factory (avoids name collision with enum values on XDK)
        World::Building* building = World::CreateBuilding(site->buildingType, site->x, site->y, 0, m_map);

        if (building) {
            building->state = World::State_Finished;
            building->connectedFlag = flag;
            flag->hasBuilding = true;
            flag->pendingBuilding = World::Building_None;

            if (m_economyManager) {
                m_economyManager->AddBuilding(building);
            }
        }

        m_constructionManager->RemoveSite(site);

        OutputDebugStringA("[GameScene] ConfirmConstruction - building completed\n");
    }

    // ─── Road building helpers ──────────────────────────────────────────────

    bool GameScene::IsNodeRoad(int nx, int ny, World::TileLayer* roadsLayer, const std::vector<std::pair<int,int>>& previewPath)
    {
        if (nx < 0 || ny < 0) return false;
        if (roadsLayer && roadsLayer->GetTile(nx, ny).regionIndex >= 0) return true;
        for (size_t k = 0; k < previewPath.size(); ++k) {
            if (previewPath[k].first == nx && previewPath[k].second == ny) return true;
        }
        return false;
    }

    int GameScene::CalcPatternAt(int x, int y, World::TileLayer* roadsLayer, const std::vector<std::pair<int,int>>& previewPath)
    {
        int pattern = 0;
        bool evenRow = (y % 2 == 0);
        if (evenRow) {
            if (IsNodeRoad(x+1, y-1, roadsLayer, previewPath)) pattern |= 1;
            if (IsNodeRoad(x+1, y+1, roadsLayer, previewPath)) pattern |= 2;
            if (IsNodeRoad(x, y+1, roadsLayer, previewPath))   pattern |= 4;
            if (IsNodeRoad(x, y-1, roadsLayer, previewPath))   pattern |= 8;
        } else {
            if (IsNodeRoad(x, y-1, roadsLayer, previewPath))   pattern |= 1;
            if (IsNodeRoad(x, y+1, roadsLayer, previewPath))   pattern |= 2;
            if (IsNodeRoad(x-1, y+1, roadsLayer, previewPath)) pattern |= 4;
            if (IsNodeRoad(x-1, y-1, roadsLayer, previewPath)) pattern |= 8;
        }
        return pattern;
    }

    void GameScene::StartRoad(int x, int y)
    {
        if (!m_map) return;
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        int nodesW = coords.GetNodesWidth();
        int nodesH = coords.GetNodesHeight();
        if (x < 0 || x >= nodesW || y < 0 || y >= nodesH) return;
        BYTE weight = m_map->GetNodeWeight(x, y);
        if (weight == World::Weight_Deep || weight == World::Weight_Block) return;

        if (!m_flagManager || !m_flagManager->GetFlagAt(x, y)) return;

        m_buildState = BUILDSTATE_PLACE_ROAD;
        m_roadStartX = x;
        m_roadStartY = y;
        m_roadPreviewPath.clear();
        m_roadPreviewPath.push_back(std::make_pair(x, y));
        OutputDebugStringA("[GameScene] Road building started\n");
    }

    void GameScene::UpdateRoadPreview(int cursorX, int cursorY)
    {
        if (m_buildState != BUILDSTATE_PLACE_ROAD) return;
        if (!m_map) return;
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        int nodesW = coords.GetNodesWidth();
        int nodesH = coords.GetNodesHeight();
        if (cursorX < 0 || cursorX >= nodesW || cursorY < 0 || cursorY >= nodesH) return;

        int endX = cursorX, endY = cursorY;
        {
            BYTE w = m_map->GetNodeWeight(endX, endY);
            if (w == World::Weight_Deep || w == World::Weight_Block) {
                const int dx[8] = {0,1,1,1,0,-1,-1,-1};
                const int dy[8] = {-1,-1,0,1,1,1,0,-1};
                bool found = false;
                for (int d = 0; d < 8; ++d) {
                    int nx = cursorX + dx[d];
                    int ny = cursorY + dy[d];
                    if (nx < 0 || nx >= nodesW || ny < 0 || ny >= nodesH) continue;
                    BYTE nw = m_map->GetNodeWeight(nx, ny);
                    if (nw != World::Weight_Deep && nw != World::Weight_Block) {
                        endX = nx; endY = ny; found = true; break;
                    }
                }
                if (!found) return;
            }
        }

        BYTE startW = m_map->GetNodeWeight(m_roadStartX, m_roadStartY);
        if (startW == World::Weight_Deep || startW == World::Weight_Block) {
            CancelRoad();
            return;
        }

        if (m_roadStartX == endX && m_roadStartY == endY) {
            m_roadPreviewPath.clear();
            m_roadPreviewPath.push_back(std::make_pair(m_roadStartX, m_roadStartY));
            return;
        }

        struct RoadPassable {
            World::Map* map;
            RoadPassable(World::Map* m) : map(m) {}
            bool operator()(int x, int y) {
                BYTE w = map->GetNodeWeight(x, y);
                if (w == World::Weight_Deep || w == World::Weight_Block) return false;
                World::TileLayer* placementLayer = map->GetLayer(World::Placement);
                World::TileLayer* objectsLayer = map->GetLayer(World::Objects);
                if (objectsLayer) {
                    const World::Tile& ot = objectsLayer->GetTile(x, y);
                    if (ot.u1 > ot.u0 && ot.v1 > ot.v0) return false;
                }
                if (placementLayer) {
                    const World::Tile& pt = placementLayer->GetTile(x, y);
                    if (pt.regionIndex >= 0 && pt.atlasName != "streets") return false;
                }
                return true;
            }
        };

        struct RoadCost {
            World::Map* map;
            RoadCost(World::Map* m) : map(m) {}
            float operator()(int x, int y) {
                World::TileLayer* roadsLayer = map->GetLayer(World::Roads);
                if (roadsLayer) {
                    const World::Tile& t = roadsLayer->GetTile(x, y);
                    if (t.regionIndex >= 0) return 0.3f;
                }
                return 1.0f;
            }
        };

        Logic::IsoNeighbors isoNeighbors;
        Logic::AStar::FindPath(
            m_roadStartX, m_roadStartY, endX, endY,
            nodesW, nodesH,
            RoadPassable(m_map),
            RoadCost(m_map),
            isoNeighbors,
            m_roadPreviewPath
        );

        if (!m_roadPreviewPath.empty() &&
            (m_roadPreviewPath[0].first != m_roadStartX || m_roadPreviewPath[0].second != m_roadStartY)) {
            m_roadPreviewPath.insert(m_roadPreviewPath.begin(), std::make_pair(m_roadStartX, m_roadStartY));
        }
    }

    void GameScene::CommitRoad()
    {
        if (m_buildState != BUILDSTATE_PLACE_ROAD) return;
        if (!m_map) return;

        World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
        if (!roadsLayer) return;

        World::TileLayer* placementLayer = m_map->GetLayer(World::Placement);

        for (size_t i = 0; i < m_roadPreviewPath.size(); ++i) {
            int px = m_roadPreviewPath[i].first;
            int py = m_roadPreviewPath[i].second;
            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            int nodesW = coords.GetNodesWidth();
            int nodesH = coords.GetNodesHeight();
            if (px < 0 || px >= nodesW || py < 0 || py >= nodesH) continue;

            World::TileLayer* objectsLayer = m_map->GetLayer(World::Objects);
            if (objectsLayer) {
                const World::Tile& ot = objectsLayer->GetTile(px, py);
                if (ot.u1 > ot.u0 && ot.v1 > ot.v0) continue;
            }
            if (placementLayer) {
                const World::Tile& pt = placementLayer->GetTile(px, py);
                if (pt.regionIndex >= 0 && pt.atlasName != "streets") continue;
            }

            // In Settlers 2, roads pass through intermediate flags without removing them.
            // Flags are network nodes — only skip the start position itself.
            (void)px; (void)py;

            World::Tile& tile = roadsLayer->GetTile(px, py);
            if (tile.regionIndex < 0) {
                tile.regionIndex = 0;
                tile.atlasName = "streets";
                tile.walkable = true;
                tile.buildable = false;
                m_map->SetNodeWeight(px, py, World::Weight_Land);
                if (placementLayer) {
                    World::Tile& pt = placementLayer->GetTile(px, py);
                    pt.regionIndex = 0;
                    pt.type = World::Tile_None;
                    pt.atlasName = "streets";
                    pt.walkable = true;
                    pt.buildable = false;
                }
            } else {
                if (placementLayer) {
                    World::Tile& pt = placementLayer->GetTile(px, py);
                    if (pt.regionIndex < 0) {
                        pt.regionIndex = 0;
                        pt.type = World::Tile_None;
                        pt.atlasName = "streets";
                        pt.walkable = true;
                        pt.buildable = false;
                    }
                }
            }
        }

        for (size_t i = 0; i < m_roadPreviewPath.size(); ++i) {
            int px = m_roadPreviewPath[i].first;
            int py = m_roadPreviewPath[i].second;
            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            int nodesW = coords.GetNodesWidth();
            int nodesH = coords.GetNodesHeight();
            if (px < 0 || px >= nodesW || py < 0 || py >= nodesH) continue;

            World::TileLayer* objectsLayer = m_map->GetLayer(World::Objects);
            if (objectsLayer) {
                const World::Tile& ot = objectsLayer->GetTile(px, py);
                if (ot.u1 > ot.u0 && ot.v1 > ot.v0) continue;
            }
            if (placementLayer) {
                const World::Tile& pt = placementLayer->GetTile(px, py);
                if (pt.regionIndex >= 0 && pt.atlasName != "streets") continue;
            }

            RebuildRoadSprite(px, py);
            UpdateRoadNeighbors(px, py);
        }

        if (!m_roadPreviewPath.empty()) {
            int endX = m_roadPreviewPath.back().first;
            int endY = m_roadPreviewPath.back().second;
            World::Flag* endFlag = NULL;
            if (m_flagManager) {
                endFlag = m_flagManager->GetFlagAt(endX, endY);
                if (!endFlag) {
                    endFlag = m_flagManager->CreateFlag(endX, endY);
                    endFlag->type = World::FLAG_NORMAL;
                    char buf[128];
                    _snprintf(buf, sizeof(buf), "[GameScene] Auto-flag placed at (%d,%d)\n", endX, endY);
                    OutputDebugStringA(buf);
                }
            }

            // Link start and end flags as neighbors
            World::Flag* startFlag = m_flagManager ? m_flagManager->GetFlagAt(m_roadStartX, m_roadStartY) : NULL;
            if (startFlag && endFlag && startFlag != endFlag) {
                // Add bidirectional neighbor links
                bool hasStart = false, hasEnd = false;
                for (size_t ni = 0; ni < startFlag->neighbors.size(); ++ni) {
                    if (startFlag->neighbors[ni] == endFlag) { hasStart = true; break; }
                }
                for (size_t ni = 0; ni < endFlag->neighbors.size(); ++ni) {
                    if (endFlag->neighbors[ni] == startFlag) { hasEnd = true; break; }
                }
                if (!hasStart) startFlag->neighbors.push_back(endFlag);
                if (!hasEnd) endFlag->neighbors.push_back(startFlag);
            }
        }

        CancelRoad();
    }

    void GameScene::CancelRoad()
    {
        m_buildState = BUILDSTATE_NONE;
        m_roadStartX = -1;
        m_roadStartY = -1;
        m_roadPreviewPath.clear();
    }

    void GameScene::RebuildRoadSprite(int x, int y)
    {
        TextureRegistry& reg = TextureRegistry::instance();
        std::tr1::shared_ptr<SpriteAtlas> roadsAtlas = reg.getAtlas("streets");
        if (!roadsAtlas) return;
        World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
        if (!roadsLayer) return;
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        int nodesW = coords.GetNodesWidth();
        int nodesH = coords.GetNodesHeight();
        if (x < 0 || x >= nodesW || y < 0 || y >= nodesH) return;

        World::Tile& tile = roadsLayer->GetTile(x, y);
        bool hasRoad = (tile.regionIndex >= 0);
        if (!hasRoad) return;

        int pattern = CalcPatternAt(x, y, roadsLayer, m_roadPreviewPath);

        int connectionCount = 0;
        int temp = pattern;
        while (temp) {
            connectionCount += temp & 1;
            temp >>= 1;
        }

        const char* groupName = NULL;
        if (connectionCount == 1) {
            switch (pattern) {
                case 1:  groupName = "street_end_s"; break;
                case 2:  groupName = "street_end_w"; break;
                case 4:  groupName = "street_end_n"; break;
                case 8:  groupName = "street_end_e"; break;
                default: groupName = "street_1"; break;
            }
        } else {
            switch (pattern) {
                case 0:  groupName = "street_1"; break;
                case 1:  groupName = "street_1"; break;
                case 2:  groupName = "street_2"; break;
                case 3:  groupName = "street_3"; break;
                case 4:  groupName = "street_1"; break;
                case 5:  groupName = "street_5"; break;
                case 6:  groupName = "street_6"; break;
                case 7:  groupName = "street_7"; break;
                case 8:  groupName = "street_2"; break;
                case 9:  groupName = "street_9"; break;
                case 10: groupName = "street_2"; break;
                case 11: groupName = "street_11"; break;
                case 12: groupName = "street_12"; break;
                case 13: groupName = "street_13"; break;
                case 14: groupName = "street_14"; break;
                case 15: groupName = "street_15"; break;
                default: groupName = "street_1"; break;
            }
        }

        if (!groupName) {
            if (pattern == 2 || pattern == 8 || pattern == 10) {
                groupName = "street_2";
            } else {
                groupName = "street_1";
            }
        }

        const std::vector<uint32_t>* group = roadsAtlas->GetGroup(groupName);
        if (!group || group->empty()) {
            group = roadsAtlas->GetGroup("street_1");
            if (!group || group->empty()) return;
        }

        uint32_t regionIdx = (*group)[rand() % group->size()];
        const SpriteRegion* region = roadsAtlas->GetRegion(regionIdx);
        if (!region) return;

        tile.u0 = region->u0;
        tile.v0 = region->v0;
        tile.u1 = region->u1;
        tile.v1 = region->v1;
        tile.regionIndex = (int)regionIdx;
        tile.atlasName = "streets";
        tile.walkable = true;
        tile.buildable = false;
    }

    void GameScene::UpdateRoadNeighbors(int x, int y)
    {
        World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
        if (!roadsLayer) return;
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        int nodesH = coords.GetNodesHeight();
        bool evenRow = (y % 2 == 0);

        if (y - 1 >= 0) {
            RebuildRoadSprite(evenRow ? x : (x - 1), y - 1);
            RebuildRoadSprite(evenRow ? (x + 1) : x, y - 1);
        }
        if (y + 1 < nodesH) {
            RebuildRoadSprite(evenRow ? x : (x - 1), y + 1);
            RebuildRoadSprite(evenRow ? (x + 1) : x, y + 1);
        }
    }

    // ─── Job function implementations ───────────────────────────────────────
    static void EconomyJobFunc(void* data)
    {
        EconomyJobData* d = (EconomyJobData*)data;
        d->economy->Update(d->carriers);
    }

    static void WildlifeSectorFunc(void* data)
    {
        WildlifeSectorData* d = (WildlifeSectorData*)data;
        if (d->wildlife)
            d->wildlife->ProcessSpawnerRange(d->startSpawner, d->endSpawner, d->newAnimals);
    }

    static void CarrierAssignFunc(void* data)
    {
        World::CarrierManager* mgr = (World::CarrierManager*)data;
        mgr->SortAndAssign();
    }

    static void CarrierUpdateFunc(void* data)
    {
        CarrierRangeData* d = (CarrierRangeData*)data;
        if (d->mgr)
            d->mgr->UpdateCarrierRange(d->startCarrier, d->endCarrier, d->dt);
    }

    static void AIChunkJobFunc(void* data)
    {
        AIChunkData* d = (AIChunkData*)data;
        for (int i = 0; i < d->numTypes; ++i)
        {
            if (d->numRequests >= MAX_REQUESTS_PER_CHUNK) break;
            Logic::BuildRequest req;
            if (d->ai->PlanBuild(d->types[i], req))
                d->requests[d->numRequests++] = req;
        }
    }

} // namespace Scene