#include "stdafx.h"
#include "GameScene.h"
#include "BuildingPlacement.h"
#include "../Graphics/RenderQueue.h"
#include "../Graphics/RenderCommandBuilder.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/SpriteAtlas.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/TextureRegistry.h"
#include "../World/Map.h"
#include "../World/MapSerializer.h"
#include "../Logic/EconomyManager.h"
#include "../World/CarrierManager.h"
#include "../World/ObjectDeletionRules.h"
#include "../World/Warehouse.h"
#include "../World/Components/BuildingFactory.h"
#include "../Logic/CoordinateSystem.h"
#include "../Logic/AStar.h"
#include <queue>
#include <cassert>

namespace Scene {

    // Construction sprite UV fix — pixel rect (1022,1883,196,139) in 2048x2048 atlas
    const float GameScene::CONSTRUCTION_U0 = 0.199f;
    const float GameScene::CONSTRUCTION_V0 = 0.239f;
    const float GameScene::CONSTRUCTION_U1 = 0.295f;
    const float GameScene::CONSTRUCTION_V1 = 0.307f;
    const uint32_t GameScene::CONSTRUCTION_ATLAS_W = 2048;
    const uint32_t GameScene::CONSTRUCTION_ATLAS_H = 2048;
    const uint32_t GameScene::CONSTRUCTION_PIXEL_X = 1022;
    const uint32_t GameScene::CONSTRUCTION_PIXEL_Y = 1883;
    const uint32_t GameScene::CONSTRUCTION_PIXEL_W = 196;
    const uint32_t GameScene::CONSTRUCTION_PIXEL_H = 139;

    static void AIChunkJobFunc(void* data);
    static void AdjustEntranceForParity(bool buildingEvenY, int& entranceX, int entranceY);
    static void AdjustEntranceForParity(bool buildingEvenY, int& entranceX, int entranceY);

    GameScene::GameScene()
        : Scene("Game")
        , m_jobManager(NULL)
        , m_transportJobManager(NULL)
        , m_map(NULL)
        , m_entityManager(NULL)
        , m_animalSystem(NULL)
        , m_animalManager(NULL)
        , m_wildlife(NULL)
        , m_economyManager(NULL)
        , m_carrierManager(NULL)
        , m_carrierSystem(NULL)
        , m_aiSystem(NULL)
        , m_renderer(NULL)
        , m_tileRenderer(NULL)
        , m_camera(NULL)
        , m_inputManager(NULL)
        , m_cursorTileX(0)
        , m_cursorTileY(0)
        , m_buildMenu(NULL)
        , m_roadMenu(NULL)
        , m_flagMenu(NULL)
        , m_flagMenuItemCount(0)
        , m_geologistMenu(NULL)
        , m_geologistMenuActive(false)
        , m_menuActive(false)
        , m_roadMenuActive(false)
        , m_flagMenuActive(false)
        , m_cursorOnTownHall(false)
        , m_townHallPanelOpen(false)
        , m_logisticsDebug(false)
        , m_gamepadCursor(10, 10)
        , m_gamepadCursorCooldown(0.0f)
        , m_gamepadActive(false)
        , m_popupCount(0)
        , m_buildState(BUILDSTATE_NONE)
        , m_selectedBuilding(World::Building_None)
        , m_placementIconIdx(-1)
        , m_placementConstrIdx(-1)
        , m_placementManager(NULL)
        , m_storehouseManager(NULL)
        , m_flagManager(NULL)
        , m_roadManager(NULL)
        , m_constructionManager(NULL)
        , m_objectLifecycleManager(NULL)
        , m_workerManager(NULL)
        , m_roadStartX(-1)
        , m_roadStartY(-1)
        , m_textManager(NULL)
        , m_statusText("")
        , m_statusTextTimer(0.0f)
        , m_townHallPanelBgIdx(-1)
        , m_townHallPanelU0(0.0f), m_townHallPanelV0(0.0f), m_townHallPanelU1(0.0f), m_townHallPanelV1(0.0f)
        , m_townHallPanelW(0.0f), m_townHallPanelH(0.0f)
        , m_confirmAction(CONFIRM_NONE)
        , m_confirmTargetX(-1)
        , m_confirmTargetY(-1)
        , m_resourceHudLoaded(false)
        , m_frameCount(0)
        , m_groundWoodIconIdx(-1)
        , m_groundWoodIconLoaded(false)
        , m_bannerSlideX(1280.0f)
        , m_bannerTargetX(1280.0f)
        , m_bannerW(0.0f)
        , m_bannerH(0.0f)
        , m_bannerU0(0.0f), m_bannerV0(0.0f), m_bannerU1(0.0f), m_bannerV1(0.0f)
        , m_bannerLoaded(false)
        , m_wildlifeRegenTimer(0.0f)
        , m_treeGrowthTimer(0.0f)
        , m_geologistState(GEOLOGIST_NONE)
        , m_geologistTimer(0.0f)
        , m_geologistTileX(-1)
        , m_geologistTileY(-1)
    {
        for (int i = 0; i < RESOURCE_HUD_COUNT; ++i) {
            m_resourceHud[i].type = World::ResourceType_None;
            m_resourceHud[i].iconName = NULL;
            m_resourceHud[i].iconIdx = -1;
            m_resourceHud[i].showOrder = i;
        }
    }

    GameScene::~GameScene()
    {
        if (m_placementManager) {
            delete m_placementManager;
            m_placementManager = NULL;
        }
        if (m_objectLifecycleManager) {
            delete m_objectLifecycleManager;
            m_objectLifecycleManager = NULL;
        }
        if (m_constructionManager) {
            delete m_constructionManager;
            m_constructionManager = NULL;
        }
        if (m_buildMenu) {
            delete m_buildMenu;
            m_buildMenu = NULL;
        }
        if (m_roadMenu) {
            delete m_roadMenu;
            m_roadMenu = NULL;
        }
        if (m_flagMenu) {
            delete m_flagMenu;
            m_flagMenu = NULL;
        }
        if (m_geologistMenu) {
            delete m_geologistMenu;
            m_geologistMenu = NULL;
        }
        if (m_roadManager) {
            delete m_roadManager;
            m_roadManager = NULL;
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
        TextureRegistry::instance().registerTexturePath("background_game", "Background\\BackgroundGameScene.png");
        TextureRegistry::instance().getTextureOrLoad("background_game");

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
        std::vector<World::FlagData> loadedFlagData;
        std::vector<World::RoadData> loadedRoadData;
        if (fileExists) {
            bool loadOk = MapSerializer::LoadV4(*m_map, "game:\\Media\\Maps\\slot_01.bin", &loadedFlagData, &loadedRoadData);
            {
                char dbg[256];
                _snprintf(dbg, sizeof(dbg), "[GameScene::Load] MapSerializer::LoadV4 returned %d (flags: %u, roads: %u)\n", loadOk, (unsigned)loadedFlagData.size(), (unsigned)loadedRoadData.size());
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

        // Set up ECS + wildlife system
        OutputDebugStringA("[GameScene::Load] Creating ECS wildlife\n");
        m_entityManager = new World::EntityManager();
        m_animalSystem = new World::AnimalSystem(m_entityManager, m_map);
        m_animalManager = new World::AnimalManager(m_entityManager, m_animalSystem);
        m_animalManager->Init(&m_map->GetHabitatRegistry());
        m_wildlife = new World::WildlifeSystem(m_map, m_animalManager, m_animalSystem);
        m_map->SetWildlifeSystem(m_wildlife);
        OutputDebugStringA("[GameScene::Load] ECS wildlife ready\n");

        // Set up economy manager and link resource registry to map
        OutputDebugStringA("[GameScene::Load] Creating EconomyManager\n");
        m_economyManager = new Logic::EconomyManager();
        m_map->SetResourceRegistry(&m_economyManager->GetRegistry());
        m_map->GenerateWildlife();

        // Backfill ResourceNodes for existing tree tiles without resources
        {
            World::TileLayer* objLayer = m_map->GetLayer(World::Objects);
            int count = 0;
            if (objLayer) {
                for (int y = 0; y < objLayer->GetHeight(); ++y) {
                    for (int x = 0; x < objLayer->GetWidth(); ++x) {
                        const World::Tile& tile = objLayer->GetTile(x, y);
                        World::ResourceNode& rn = m_map->GetResourceNode(x, y);
                        if (rn.type != World::ResourceType_None) continue;
                        World::ResourceType rt = World::TileTypeToResourceType(tile.type);
                        if (rt != World::ResourceType_None) {
                            rn.type = rt;
                            rn.amount = World::TreeState_Mature;
                            rn.isVisible = true;
                            m_economyManager->GetRegistry().RegisterWorldResource(rt, x, y);
                            count++;
                        }
                    }
                }
            }
            if (count > 0) {
                char dbg[128];
                _snprintf(dbg, sizeof(dbg), "[GameScene::Load] Backfilled %d tree resource nodes\n", count);
                OutputDebugStringA(dbg);
            }
        }

        AssignOreDepositsToMountains();

        OutputDebugStringA("[GameScene::Load] EconomyManager ready\n");

        // Set up ECS carrier system and carrier manager
        OutputDebugStringA("[GameScene::Load] Creating CarrierSystem\n");
        m_carrierSystem = new World::CarrierSystem(m_entityManager);
        OutputDebugStringA("[GameScene::Load] Creating CarrierManager\n");
        m_carrierManager = new World::CarrierManager();
        m_carrierManager->SetCarrierSystem(m_carrierSystem);
        OutputDebugStringA("[GameScene::Load] CarrierManager ready\n");

        // Set up WorkerManager for worker arrivals
        OutputDebugStringA("[GameScene::Load] Creating WorkerManager\n");
        m_workerManager = new World::WorkerManager();
        OutputDebugStringA("[GameScene::Load] WorkerManager ready\n");

        // Set up AI system
        OutputDebugStringA("[GameScene::Load] Creating AISystem\n");
        m_aiSystem = new Logic::AISystem(0, m_map, m_economyManager);
        OutputDebugStringA("[GameScene::Load] AISystem ready\n");

        // Create flag manager and load flags from save
        OutputDebugStringA("[GameScene::Load] Creating FlagManager\n");
        m_flagManager = new World::FlagManager();
        if (!loadedFlagData.empty()) {
            m_flagManager->LoadFromData(loadedFlagData);
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[GameScene::Load] Loaded %u flags from save (v4)\n", (unsigned)loadedFlagData.size());
            OutputDebugStringA(dbg);
            for (size_t fi = 0; fi < m_flagManager->GetCount(); ++fi) {
                World::Flag* ff = m_flagManager->GetFlag(fi);
                if (ff) {
                    _snprintf(dbg, sizeof(dbg), "[FlagLoaded] idx=%zu id=%u type=%d pos=(%d,%d) handle=(%u,%u) hasBuilding=%d\n",
                        fi, ff->id, (int)ff->type, ff->pos.x, ff->pos.y,
                        ff->handle.index, ff->handle.generation, (int)ff->hasBuilding);
                    OutputDebugStringA(dbg);
                }
            }
        }
        m_carrierManager->SetFlagManager(m_flagManager);
        // Create RoadManager
        OutputDebugStringA("[GameScene::Load] Creating RoadManager\n");
        m_roadManager = new World::RoadManager();
        m_roadManager->SetFlagManager(m_flagManager);

        if (!loadedRoadData.empty()) {
            // New save format — load roads directly from saved data
            m_roadManager->LoadFromData(loadedRoadData, m_flagManager);
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[GameScene::Load] Loaded %u roads from save\n", (unsigned)loadedRoadData.size());
            OutputDebugStringA(dbg);
        } else if (m_flagManager->GetCount() > 0) {
            // Old save format — reconstruct roads via BFS on road tiles (fallback)
            World::TileLayer* roadsLayer = m_map ? m_map->GetLayer(World::Roads) : NULL;
            if (roadsLayer) {
                int rw = roadsLayer->GetWidth();
                int rh = roadsLayer->GetHeight();
                int roadsCreated = 0;
                for (size_t fi = 0; fi < m_flagManager->GetCount(); ++fi) {
                    World::Flag* f = m_flagManager->GetFlag(fi);
                    if (!f) continue;
                    std::vector<bool> visited(rw * rh, false);
                    std::queue<std::pair<int,int>> q;
                    std::vector<int> parent(rw * rh, -1);
                    q.push(std::make_pair(f->pos.x, f->pos.y));
                    visited[f->pos.y * rw + f->pos.x] = true;
                    parent[f->pos.y * rw + f->pos.x] = -2;
                    while (!q.empty()) {
                        int cx = q.front().first;
                        int cy = q.front().second;
                        q.pop();
                        World::Flag* other = (cx == f->pos.x && cy == f->pos.y) ? NULL : m_flagManager->GetFlagAt(cx, cy);
                        if (other) {
                            if (!m_roadManager->GetRoadBetween(f, other)) {
                                std::vector<Vector2i> tilePath;
                                int px = cx, py = cy;
                                while (px != f->pos.x || py != f->pos.y) {
                                    Vector2i v; v.x = px; v.y = py;
                                    tilePath.push_back(v);
                                    int p = parent[py * rw + px];
                                    px = p & 0xFFFF;
                                    py = (p >> 16) & 0xFFFF;
                                }
                                Vector2i sv; sv.x = f->pos.x; sv.y = f->pos.y;
                                tilePath.push_back(sv);
                                std::reverse(tilePath.begin(), tilePath.end());
                                m_roadManager->CreateRoad(f, other, tilePath);
                                roadsCreated++;
                            }
                            continue;
                        }
                        bool evenRow = (cy % 2 == 0);
                        int nx[6], ny[6];
                        if (evenRow) {
                            int eNX[] = {cx-1, cx+1, cx-1, cx, cx-1, cx};
                            int eNY[] = {cy, cy, cy-1, cy-1, cy+1, cy+1};
                            memcpy(nx, eNX, sizeof(nx)); memcpy(ny, eNY, sizeof(ny));
                        } else {
                            int oNX[] = {cx-1, cx+1, cx, cx+1, cx, cx+1};
                            int oNY[] = {cy, cy, cy-1, cy-1, cy+1, cy+1};
                            memcpy(nx, oNX, sizeof(nx)); memcpy(ny, oNY, sizeof(ny));
                        }
                        for (int di = 0; di < 6; ++di) {
                            int tx = nx[di], ty = ny[di];
                            if (tx < 0 || tx >= rw || ty < 0 || ty >= rh) continue;
                            if (visited[ty * rw + tx]) continue;
                            const World::Tile& rt = roadsLayer->GetTile(tx, ty);
                            if (rt.atlasName != "streets") continue;
                            visited[ty * rw + tx] = true;
                            parent[ty * rw + tx] = cx | (cy << 16);
                            q.push(std::make_pair(tx, ty));
                        }
                    }
                }
                if (roadsCreated > 0) {
                    char dbg[256];
                    _snprintf(dbg, sizeof(dbg), "[Graph] Reconstructed %d roads from road tile BFS (old save fallback)\n", roadsCreated);
                    OutputDebugStringA(dbg);
                }
            }
        }

        OutputDebugStringA("[GameScene::Load] RoadManager ready\n");
        m_economyManager->SetFlagManager(m_flagManager);
        m_economyManager->SetRoadManager(m_roadManager);
        m_carrierManager->SetRoadManager(m_roadManager);

        // Create TransportJobManager and wire up manager references
        OutputDebugStringA("[GameScene::Load] Creating TransportJobManager\n");
        m_transportJobManager = new World::TransportJobManager();
        m_transportJobManager->SetFlagManager(m_flagManager);
        m_transportJobManager->SetRoadManager(m_roadManager);
        m_transportJobManager->SetCarrierManager(m_carrierManager);
        if (m_economyManager && m_economyManager->GetWarehouse()) {
            m_transportJobManager->SetWarehouse(m_economyManager->GetWarehouse());
        }
        m_carrierManager->SetJobManager(m_transportJobManager);
        OutputDebugStringA("[GameScene::Load] TransportJobManager ready\n");

        // Create CargoManager and DemandManager (new transport system)
        OutputDebugStringA("[GameScene::Load] Creating CargoManager\n");
        m_cargoManager = new World::CargoManager();
        OutputDebugStringA("[GameScene::Load] CargoManager ready\n");
        m_demandManager = new World::DemandManager();
        OutputDebugStringA("[GameScene::Load] DemandManager ready\n");
        if (m_map) {
            m_map->SetCargoManager(m_cargoManager);
            m_map->SetDemandManager(m_demandManager);
        }

        // Create StorehouseManager for O(1) resource tracking
        OutputDebugStringA("[GameScene::Load] Creating StorehouseManager\n");
        m_storehouseManager = new World::StorehouseManager();
        m_storehouseManager->Init();
        if (m_economyManager) {
            m_economyManager->SetStorehouseManager(m_storehouseManager);
        }
        if (m_cargoManager) {
            m_cargoManager->SetStorehouseManager(m_storehouseManager);
        }
        OutputDebugStringA("[GameScene::Load] StorehouseManager ready\n");

        // Create construction manager
        OutputDebugStringA("[GameScene::Load] Creating ConstructionManager\n");
        m_constructionManager = new World::ConstructionManager();
        m_constructionManager->SetFlagManager(m_flagManager);
        m_constructionManager->SetRoadManager(m_roadManager);
        if (m_economyManager && m_economyManager->GetWarehouse() && m_economyManager->GetWarehouse()->connectedFlag) {
            m_constructionManager->SetWarehouseFlag(m_economyManager->GetWarehouse()->connectedFlag);
            m_carrierManager->SetWarehouseFlag(m_economyManager->GetWarehouse()->connectedFlag);
        }
        if (m_demandManager) {
            m_constructionManager->SetDemandManager(m_demandManager);
            m_carrierManager->SetDemandManager(m_demandManager);
        }
        if (m_cargoManager) {
            m_carrierManager->SetCargoManager(m_cargoManager);
            if (m_economyManager) {
                m_economyManager->SetCargoManager(m_cargoManager);
            }
        }
        if (m_workerManager) {
            m_workerManager->SetRoadManager(m_roadManager);
        }
        OutputDebugStringA("[GameScene::Load] ConstructionManager ready\n");

        // Create lifecycle manager
        m_objectLifecycleManager = new World::ObjectLifecycleManager(
            m_flagManager, m_roadManager, m_carrierManager, m_cargoManager,
            m_transportJobManager, m_constructionManager, m_economyManager);
        OutputDebugStringA("[GameScene::Load] ObjectLifecycleManager ready\n");

        // Create building placement manager
        m_placementManager = new BuildingPlacementManager(
            m_map, m_flagManager, m_roadManager, m_carrierManager,
            m_economyManager, m_demandManager);
        OutputDebugStringA("[GameScene::Load] BuildingPlacementManager ready\n");

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
        if (!m_buildMenu) {
            m_buildMenu = new GridMenu();
            m_buildMenu->SetTextManager(m_textManager);
            m_buildMenu->SetSpriteRenderer(m_renderer ? m_renderer->GetSpriteRenderer() : NULL);
            m_buildMenu->SetRenderer(m_renderer);
            if (m_renderer) m_buildMenu->SetRenderQueue(m_renderer->GetRenderQueue());
        }
        InitBuildMenu();
        OutputDebugStringA("[GameScene::Load] Build menu initialized\n");

        // Initialize road/flag menu
        OutputDebugStringA("[GameScene::Load] Initializing road menu\n");
        if (!m_roadMenu) {
            m_roadMenu = new GridMenu();
            m_roadMenu->SetTextManager(m_textManager);
            m_roadMenu->SetSpriteRenderer(m_renderer ? m_renderer->GetSpriteRenderer() : NULL);
            m_roadMenu->SetRenderer(m_renderer);
            if (m_renderer) m_roadMenu->SetRenderQueue(m_renderer->GetRenderQueue());
        }
        InitRoadMenu();
        OutputDebugStringA("[GameScene::Load] Road menu initialized\n");

        // Initialize flag menu (UIMenu-based)
        if (!m_flagMenu) {
            m_flagMenu = new UIMenu();
            m_flagMenu->SetTextManager(m_textManager);
            m_flagMenu->SetRenderer(m_renderer ? m_renderer->GetSpriteRenderer() : NULL,
                                   m_renderer ? m_renderer->GetRenderQueue() : NULL);
        }
        InitFlagMenu();
        OutputDebugStringA("[GameScene::Load] Flag menu initialized\n");

        // Initialize geologist menu (UIMenu-based)
        if (!m_geologistMenu) {
            m_geologistMenu = new UIMenu();
            m_geologistMenu->SetTextManager(m_textManager);
            m_geologistMenu->SetRenderer(m_renderer ? m_renderer->GetSpriteRenderer() : NULL,
                                          m_renderer ? m_renderer->GetRenderQueue() : NULL);
        }
        InitGeologistMenu();
        OutputDebugStringA("[GameScene::Load] Geologist menu initialized\n");

        // ─── Cache bunner_info sprite from UI atlas ────────────────────
        {
            TextureRegistry& reg = TextureRegistry::instance();
            std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
            if (uiAtlas) {
                uint32_t bIdx = uiAtlas->GetIndex("bunner_info");
                if (bIdx != 0xFFFFFFFF) {
                    const SpriteRegion* r = uiAtlas->GetRegion(bIdx);
                    if (r) {
                        m_bannerU0 = r->u0; m_bannerV0 = r->v0;
                        m_bannerU1 = r->u1; m_bannerV1 = r->v1;
                        m_bannerW = (float)r->width;
                        m_bannerH = (float)r->height;
                        m_bannerLoaded = true;
                        m_bannerSlideX = 1280.0f;
                        m_bannerTargetX = 1280.0f;
                        OutputDebugStringA("[GameScene::Load] bunner_info cached\n");
                    }
                }
            }
        }

        // ─── Look up town hall panel sprite from UI atlas ──────────
        {
            TextureRegistry& reg = TextureRegistry::instance();
            std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
            if (uiAtlas) {
                uint32_t panelIdx = uiAtlas->GetIndex("menu_townhall_resource");
                if (panelIdx != 0xFFFFFFFF) {
                    m_townHallPanelBgIdx = (int)panelIdx;
                    const SpriteRegion* r = uiAtlas->GetRegion(panelIdx);
                    if (r) {
                        m_townHallPanelU0 = r->u0;
                        m_townHallPanelV0 = r->v0;
                        m_townHallPanelU1 = r->u1;
                        m_townHallPanelV1 = r->v1;
                        m_townHallPanelW = (float)r->width;
                        m_townHallPanelH = (float)r->height;
                    }
                }
            }
        }

        // ─── Initialize stump sprite indices ────────────────────────────
        {
            TextureRegistry& reg = TextureRegistry::instance();
            std::tr1::shared_ptr<SpriteAtlas> maptiles = reg.getAtlas("maptiles");
            if (maptiles && m_map) {
                uint32_t s1 = maptiles->GetIndex("stump_01");
                uint32_t s2 = maptiles->GetIndex("stump_02");
                uint32_t s3 = maptiles->GetIndex("stump_03");
                const SpriteRegion* r1 = (s1 != 0xFFFFFFFF) ? maptiles->GetRegion(s1) : NULL;
                const SpriteRegion* r2 = (s2 != 0xFFFFFFFF) ? maptiles->GetRegion(s2) : NULL;
                const SpriteRegion* r3 = (s3 != 0xFFFFFFFF) ? maptiles->GetRegion(s3) : NULL;
                World::Map::SpriteData d1 = { (int)s1, r1 ? r1->u0 : 0, r1 ? r1->v0 : 0, r1 ? r1->u1 : 1, r1 ? r1->v1 : 1 };
                World::Map::SpriteData d2 = { (int)s2, r2 ? r2->u0 : 0, r2 ? r2->v0 : 0, r2 ? r2->u1 : 1, r2 ? r2->v1 : 1 };
                World::Map::SpriteData d3 = { (int)s3, r3 ? r3->u0 : 0, r3 ? r3->v0 : 0, r3 ? r3->u1 : 1, r3 ? r3->v1 : 1 };
                m_map->SetStumpSprites(d1, d2, d3);
                OutputDebugStringA("[GameScene] Stump sprites initialized\n");
            }
        }

        // ─── Initialize tree sprite indices (young + mature) ────────────
        {
            TextureRegistry& reg = TextureRegistry::instance();
            std::tr1::shared_ptr<SpriteAtlas> maptiles = reg.getAtlas("maptiles");
            if (maptiles && m_map) {
                const char* treeNames[] = {
                    "t_young_01", "t_young_02", "t_young_03",
                    "t_acer_rubrum_01", "t_acer_rubrum_02", "t_acer_rubrum_03",
                    "t_acer_rubrum_04", "t_acer_rubrum_05", "t_acer_rubrum_06",
                    "t_birch_01", "t_birch_02",
                    "furtree_small_01", "furtree_small_02", "furtree_small_03",
                    "t_furtree_01", "t_furtree_02", "t_furtree_03", "t_furtree_04"
                };
                int loaded = 0;
                for (int i = 0; i < sizeof(treeNames)/sizeof(treeNames[0]); ++i) {
                    uint32_t idx = maptiles->GetIndex(treeNames[i]);
                    if (idx != 0xFFFFFFFF) {
                        const SpriteRegion* r = maptiles->GetRegion(idx);
                        if (r) {
                            World::Map::SpriteData sd = { (int)idx, r->u0, r->v0, r->u1, r->v1 };
                            m_map->AddTreeSprite(sd);
                            ++loaded;
                        }
                    }
                }
                char dbg[128];
                _snprintf(dbg, sizeof(dbg), "[GameScene] Tree sprites initialized: %d loaded\n", loaded);
                OutputDebugStringA(dbg);
            }
        }

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
                                if (tile.atlasName != "Buildings" || tile.type == World::Tile_None) continue;
                                bool isConstr = false;
                                const SpriteRegion* rr = buildingsAtlas->GetRegion(static_cast<uint32_t>(tile.regionIndex));
                                if (rr) {
                                    const std::string& sn = rr->name;
                                    isConstr = (sn.find("construction") != std::string::npos ||
                                                sn.find("Construction") != std::string::npos);
                                }
                                if (isConstr ||
                                    static_cast<uint32_t>(tile.regionIndex) == cIdx ||
                                    (tile.u0 == 0.0f && tile.v0 == 0.0f && tile.u1 == 1.0f && tile.v1 == 1.0f))
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

        // Restore any buildings placed in the editor from the Buildings layer
        RestoreBuildingsFromLayer();

        // ─── DEBUG: dump all Buildings layer tiles ─────────────────────
        {
            World::TileLayer* bl = m_map ? m_map->GetLayer(World::Buildings) : NULL;
            TextureRegistry& treg = TextureRegistry::instance();
            std::tr1::shared_ptr<SpriteAtlas> batlas = treg.getAtlas("Buildings");
            if (bl) {
                int bw = bl->GetWidth(), bh = bl->GetHeight();
                for (int by = 0; by < bh; ++by) {
                    for (int bx = 0; bx < bw; ++bx) {
                        const World::Tile& btl = bl->GetTile(bx, by);
                        if (btl.atlasName == "Buildings" || (btl.regionIndex >= 0 && batlas)) {
                            const char* spriteName = "";
                            if (batlas) {
                                const SpriteRegion* rr = batlas->GetRegion(static_cast<uint32_t>(btl.regionIndex));
                                if (rr) spriteName = rr->name.c_str();
                            }
                            char dbg[512];
                            _snprintf(dbg, sizeof(dbg), "[BuildingsLayer] (%d,%d) regionIdx=%d sprite='%s' atlas='%s' walkable=%d type=%d u=(%g,%g) v=(%g,%g)\n",
                                bx, by, btl.regionIndex, spriteName, btl.atlasName.c_str(), (int)btl.walkable, (int)btl.type,
                                btl.u0, btl.v0, btl.u1, btl.v1);
                            OutputDebugStringA(dbg);
                        }
                    }
                }
            }
        }

        // Wire StorehouseManager into existing warehouse (if restored from save)
        if (m_economyManager && m_economyManager->GetWarehouse() && m_storehouseManager) {
            World::Warehouse* wh = m_economyManager->GetWarehouse();
            wh->SetStorehouseManager(m_storehouseManager);
        }

        // === Validate warehouse has connectedFlag (in case restored from save) ===
        if (m_economyManager && m_economyManager->GetWarehouse()) {
            World::Warehouse* wh = m_economyManager->GetWarehouse();
            {
                char dbg[256];
                _snprintf(dbg, sizeof(dbg),
                    "[GameScene] Warehouse check: wh=%p connectedFlag=%p flagId=%u\n",
                    wh, wh->connectedFlag,
                    wh->connectedFlag ? wh->connectedFlag->id : 0);
                OutputDebugStringA(dbg);
            }
            if (!wh->connectedFlag) {
                // Find warehouse flag and link it
                for (size_t fi = 0; fi < m_flagManager->GetCount(); ++fi) {
                    World::Flag* f = m_flagManager->GetFlag(fi);
                    if (f && f->building == wh) {
                        wh->connectedFlag = f;
                        char buf[128];
                        _snprintf(buf, sizeof(buf),
                            "[GameScene] Re-linked warehouse to flag %u at (%d,%d)\n",
                            f->id, f->pos.x, f->pos.y);
                        OutputDebugStringA(buf);
                        break;
                    }
                }
            }
            // Update TransportJobManager with warehouse reference
            if (m_transportJobManager) {
                m_transportJobManager->SetWarehouse(wh);
            }
            // WorkerManager no longer needs warehouse reference
        }

        // ─── Create starting warehouse + carriers (only if none restored) ──
        if (m_flagManager && m_economyManager && !m_economyManager->GetWarehouse()) {
            int hqFlagX = 10, hqFlagY = 10;  // node grid position for warehouse flag
            int hqBuildX = 10, hqBuildY = 8; // building footprint (entrance offset 0,2)

            // Mark the building tile on Buildings layer with b_townhall sprite
            World::TileLayer* buildingsLayer = m_map->GetLayer(World::Buildings);
            if (buildingsLayer && hqBuildX >= 0 && hqBuildX < buildingsLayer->GetWidth() &&
                hqBuildY >= 0 && hqBuildY < buildingsLayer->GetHeight())
            {
                World::Tile& bt = buildingsLayer->GetTile(hqBuildX, hqBuildY);
                bt.type = World::Decoration;
                bt.atlasName = "Buildings";
                bt.walkable = false;
                // Set b_townhall sprite from Buildings atlas
                TextureRegistry& reg = TextureRegistry::instance();
                std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
                if (buildingsAtlas) {
                    uint32_t tIdx = buildingsAtlas->GetIndex("b_townhall");
                    if (tIdx != 0xFFFFFFFF) {
                        const SpriteRegion* r = buildingsAtlas->GetRegion(tIdx);
                        if (r) {
                            bt.regionIndex = tIdx;
                            bt.u0 = r->u0;
                            bt.v0 = r->v0;
                            bt.u1 = r->u1;
                            bt.v1 = r->v1;
                        }
                    }
                }
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
            hqFlag->building = warehouse;
            warehouse->map = m_map;
            if (m_storehouseManager) {
                warehouse->SetStorehouseManager(m_storehouseManager);
            }

            // Connect HQ flag to any existing road network
            LinkFlagToRoadNetwork(hqFlag);

            // Seed warehouse with starting resources
            warehouse->AddResource(World::ResourceType_Wood, 500);
            warehouse->AddResource(World::ResourceType_Stone, 500);
            warehouse->AddResource(World::ResourceType_Planks, 200);
            warehouse->AddResource(World::ResourceType_Fish, 100);
            warehouse->AddResource(World::ResourceType_Meat, 100);
            warehouse->AddResource(World::ResourceType_Coal, 100);

            m_economyManager->SetWarehouse(warehouse);
            m_economyManager->AddBuilding(warehouse);
            if (m_transportJobManager) {
                m_transportJobManager->SetWarehouse(warehouse);
            }
            if (m_constructionManager) {
                m_constructionManager->SetWarehouseFlag(hqFlag);
            }
            if (m_carrierManager) {
                m_carrierManager->SetWarehouseFlag(hqFlag);
            }
            // Set warehouse demand for all resource types (LOW priority)
            if (m_demandManager && hqFlag) {
                World::ResourceType allTypes[] = {
                    World::ResourceType_Wood, World::ResourceType_Stone, World::ResourceType_Planks,
                    World::ResourceType_Fish, World::ResourceType_Meat, World::ResourceType_Coal,
                    World::ResourceType_BronzeBar
                };
                for (int ri = 0; ri < sizeof(allTypes)/sizeof(allTypes[0]); ++ri) {
                    m_demandManager->SetDemand(allTypes[ri], 9999, hqFlag->handle, 10);
                }
            }

            {
                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[Startup] Warehouse Wood=%u Stone=%u Planks=%u Fish=%u Meat=%u Coal=%u\n",
                    m_storehouseManager ? m_storehouseManager->GetStoredCount(World::ResourceType_Wood) : 0,
                    m_storehouseManager ? m_storehouseManager->GetStoredCount(World::ResourceType_Stone) : 0,
                    m_storehouseManager ? m_storehouseManager->GetStoredCount(World::ResourceType_Planks) : 0,
                    m_storehouseManager ? m_storehouseManager->GetStoredCount(World::ResourceType_Fish) : 0,
                    m_storehouseManager ? m_storehouseManager->GetStoredCount(World::ResourceType_Meat) : 0,
                    m_storehouseManager ? m_storehouseManager->GetStoredCount(World::ResourceType_Coal) : 0);
                OutputDebugStringA(buf);
            }

            // Sync carriers for HQ flag (Settlers 2: per-segment walking)
            SyncCarriersForFlag(hqFlag);

            // ─── Startup diagnostics ───────────────────────────────────
            {
                char buf[512];
                World::Warehouse* wh = m_economyManager ? m_economyManager->GetWarehouse() : NULL;
                if (wh) {
                    _snprintf(buf, sizeof(buf), "[Startup] Warehouse at (%d,%d) flag=%s flagPos=(%d,%d) flagRoads=%u\n",
                        wh->pos.x, wh->pos.y,
                        wh->connectedFlag ? "YES" : "NO",
                        wh->connectedFlag ? wh->connectedFlag->pos.x : -1,
                        wh->connectedFlag ? wh->connectedFlag->pos.y : -1,
                        wh->connectedFlag ? (unsigned)wh->connectedFlag->roads.size() : 0);
                    OutputDebugStringA(buf);
                } else {
                    OutputDebugStringA("[Startup] WARNING: No warehouse found!\n");
                }
                for (size_t fi = 0; fi < m_flagManager->GetCount(); ++fi) {
                    World::Flag* ff = m_flagManager->GetFlag(fi);
                    if (ff) {
                        _snprintf(buf, sizeof(buf), "[Startup] Flags[%u]: id=%u pos=(%d,%d) type=%d roads=%u",
                            (unsigned)fi, ff->id, ff->pos.x, ff->pos.y, ff->type, (unsigned)ff->roads.size());
                        OutputDebugStringA(buf);
                        for (size_t ri = 0; ri < ff->roads.size(); ++ri) {
                            World::Flag* rra = m_flagManager ? m_flagManager->ResolveFlag(ff->roads[ri]->a) : NULL;
                            World::Flag* rrb = m_flagManager ? m_flagManager->ResolveFlag(ff->roads[ri]->b) : NULL;
                            World::Flag* other = (rra == ff) ? rrb : rra;
                            if (other) {
                                _snprintf(buf, sizeof(buf), " -> road %u to id=%u pos=(%d,%d)",
                                    ff->roads[ri]->id, other->id, other->pos.x, other->pos.y);
                                OutputDebugStringA(buf);
                            }
                        }
                        OutputDebugStringA("\n");
                    }
                }
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
        std::vector<World::FlagData> flagData;
        if (m_flagManager) {
            flagData = m_flagManager->GetFlagData();
        }
        std::vector<World::RoadData> roadData;
        if (m_roadManager) {
            roadData = m_roadManager->GetRoadData();
        }
        MapSerializer::SaveV4(*m_map, "game:\\Media\\Maps\\slot_01.bin", &flagData, &roadData);
    }

    if (m_roadManager) {
        delete m_roadManager;
        m_roadManager = NULL;
    }

    if (m_transportJobManager) {
        delete m_transportJobManager;
        m_transportJobManager = NULL;
    }

    if (m_cargoManager) {
        delete m_cargoManager;
        m_cargoManager = NULL;
    }
    if (m_storehouseManager) {
        delete m_storehouseManager;
        m_storehouseManager = NULL;
    }
    if (m_demandManager) {
        delete m_demandManager;
        m_demandManager = NULL;
    }

    if (m_constructionManager) {
        delete m_constructionManager;
        m_constructionManager = NULL;
    }

    if (m_objectLifecycleManager) {
        delete m_objectLifecycleManager;
        m_objectLifecycleManager = NULL;
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
    if (m_carrierSystem) {
        delete m_carrierSystem;
        m_carrierSystem = NULL;
    }
    if (m_workerManager) {
        delete m_workerManager;
        m_workerManager = NULL;
    }
    if (m_economyManager) {
        delete m_economyManager;
        m_economyManager = NULL;
    }
    if (m_wildlife) {
        m_map->SetWildlifeSystem(NULL);
        delete m_wildlife;
        m_wildlife = NULL;
    }
    if (m_animalManager) {
        delete m_animalManager;
        m_animalManager = NULL;
    }
    if (m_animalSystem) {
        delete m_animalSystem;
        m_animalSystem = NULL;
    }
    if (m_entityManager) {
        delete m_entityManager;
        m_entityManager = NULL;
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
//    OutputDebugStringA("[GameScene::Update] START\n");
    if (!m_loaded) {
        OutputDebugStringA("[GameScene::Update] Not loaded, returning\n");
        return;
    }

    ++m_frameCount;

    // ─── Camera movement and zoom (disabled during menu or town hall panel) ──
    if (!m_menuActive && !m_roadMenuActive && !m_flagMenuActive && !m_geologistMenuActive && !m_townHallPanelOpen && m_camera && m_inputManager) {
        Input::Gamepad* gamepad = m_inputManager->GetGamepad();
        if (gamepad) {
            float moveSpeed = 2000.0f * deltaTime;
            float stickX, stickY;
            gamepad->GetLeftStick(stickX, stickY);
            if (fabsf(stickX) > 0.1f || fabsf(stickY) > 0.1f) {
                m_camera->Move(stickX * moveSpeed, stickY * moveSpeed);
            }
            float rightX, rightY;
            gamepad->GetRightStick(rightX, rightY);
            if (fabsf(rightY) > 0.1f) {
                m_camera->Zoom(rightY * 0.3f * deltaTime);
            }
        }
    }
    if (m_camera) {
        m_camera->Update(deltaTime);
    }

    // ─── Cursor update ────────────────────────────────────────────────
    UpdateCursor();
    m_cursorOnTownHall = false;
    // Check if cursor is on a warehouse building tile
    if (m_map) {
        World::TileLayer* buildingsLayer = m_map->GetLayer(World::Buildings);
        if (buildingsLayer) {
            int bx = m_cursorTileX;
            int by = m_cursorTileY;
            if (bx >= 0 && bx < buildingsLayer->GetWidth() && by >= 0 && by < buildingsLayer->GetHeight()) {
                const World::Tile& tile = buildingsLayer->GetTile(bx, by);
                if (tile.atlasName == "Buildings" && tile.regionIndex >= 0) {
                    TextureRegistry& reg = TextureRegistry::instance();
                    std::tr1::shared_ptr<SpriteAtlas> atlas = reg.getAtlas("Buildings");
                    if (atlas) {
                        const SpriteRegion* region = atlas->GetRegion(tile.regionIndex);
                        if (region) {
                            World::BuildingType type = GetBuildingTypeFromSpriteName(region->name);
                            if (type == World::Storehouse) m_cursorOnTownHall = true;
                        }
                    }
                }
            }
        }
    }
    // Also check flag position (for entrance-flag hover)
    // NOTE: not flag-based detection — A on flag opens road menu, not town hall panel

    // Auto-update A* road preview when cursor moves during road building
    if (m_buildState == BUILDSTATE_PLACE_ROAD) {
        UpdateRoadPreview(m_cursorTileX, m_cursorTileY);
    }

    // ─── Update status text & banner animation ────────────────────────
    // ─── Banner slide logic: show with text, animate for temporary ──
    if (m_bannerLoaded) {
        if (m_statusText.empty()) {
            m_bannerTargetX = 1280.0f; // slide out
        } else {
            m_bannerTargetX = 1280.0f - m_bannerW; // slide in
        }
        float speed = 1200.0f;
        if (m_bannerSlideX > m_bannerTargetX) {
            m_bannerSlideX -= speed * deltaTime;
            if (m_bannerSlideX < m_bannerTargetX) m_bannerSlideX = m_bannerTargetX;
        } else if (m_bannerSlideX < m_bannerTargetX) {
            m_bannerSlideX += speed * deltaTime;
            if (m_bannerSlideX > m_bannerTargetX) m_bannerSlideX = m_bannerTargetX;
        }
    }
    // ─── Update status text timer ───────────────────────────────────
    if (m_statusTextTimer > 0.0f) {
        m_statusTextTimer -= deltaTime;
        if (m_statusTextTimer <= 0.0f) m_statusText = "";
    }
    if (m_statusText.empty() || m_statusTextTimer <= 0.0f) {
        switch (m_buildState) {
            case BUILDSTATE_NONE:
                if (m_menuActive) {
                    m_statusText = "BUILD MENU: select a building";
                } else if (m_roadMenuActive || m_flagMenuActive || m_geologistMenuActive) {
                    m_statusText = "ROAD MENU: choose action";
                } else {
                    m_statusText = "";
                }
                break;
            case BUILDSTATE_PLACE_FLAG:
                m_statusText = "PLACE FLAG: A=place  B=cancel";
                break;
            case BUILDSTATE_PLACE_ROAD: {
                if (!m_roadAutoPath.empty()) {
                    char rdbg[64] = "";
                    _snprintf(rdbg, sizeof(rdbg), " (%d tiles)", (int)m_roadAutoPath.size());
                    m_statusText = std::string("ROAD: auto-path to flag") + rdbg + "  A=confirm";
                } else {
                    char rdbg[64] = "";
                    int pathLen = (int)m_roadPreviewPath.size();
                    if (pathLen > 0) {
                        _snprintf(rdbg, sizeof(rdbg), " %d cells", pathLen);
                    }
                    m_statusText = std::string("ROAD: A=add tile") + rdbg + "  B=cancel";
                }
                break;
            }
            case BUILDSTATE_CONFIRM: {
                char cdbg[128];
                int flagsExact = (m_flagManager && m_flagManager->GetFlagAt(m_cursorTileX, m_cursorTileY)) ? 1 : 0;
                _snprintf(cdbg, sizeof(cdbg), "@(%d,%d) target(%d,%d) exact=%d ",
                    m_cursorTileX, m_cursorTileY, m_confirmTargetX, m_confirmTargetY, flagsExact);
                std::string prefix(cdbg);
                if (m_confirmAction == CONFIRM_PLACE_FLAG)
                    m_statusText = prefix + "Place a flag? A=Yes  B=No";
                else if (m_confirmAction == CONFIRM_START_ROAD)
                    m_statusText = prefix + "Build road? A=Yes  B=No";
                else if (m_confirmAction == CONFIRM_DELETE_FLAG)
                    m_statusText = prefix + "Delete building and flag? A=Yes  B=No";
                break;
            }
        }
    }

    // ─── Gamepad input & UI updates ───────────────────────────────
    HandleGamepadInput();
    UpdateGamepadUI(deltaTime);

    // ─── Geologist timer ──────────────────────────────────────────
    if (m_geologistState == GEOLOGIST_WORKING) {
        m_geologistTimer -= deltaTime;
        if (m_geologistTimer <= 0.0f) {
            m_geologistTimer = 0.0f;
            // Survey complete — mark resource node and show result in banner
            if (m_map && m_geologistTileX >= 0 && m_geologistTileY >= 0) {
                World::ResourceNode& node = m_map->GetResourceNode(m_geologistTileX, m_geologistTileY);
                node.surveyed = true;
                char buf[128];
                if (node.type != World::ResourceType_None && node.amount > 0) {
                    const char* name = "";
                    switch (node.type) {
                        case World::ResourceType_Coal:     name = "Coal vein";        break;
                        case World::ResourceType_IronOre:  name = "Iron ore";         break;
                        case World::ResourceType_GoldOre:  name = "Gold vein";        break;
                        case World::ResourceType_BronzeOre:name = "Bronze ore vein";  break;
                        case World::ResourceType_Stone:    name = "Stone deposit";    break;
                        case World::ResourceType_Marble:   name = "Marble deposit";   break;
                        case World::ResourceType_Granite:  name = "Granite deposit";  break;
                        default:                           name = "Minerals";         break;
                    }
                    _snprintf(buf, sizeof(buf), "%s found! Units: %d", name, node.amount);
                } else {
                    strcpy(buf, "Barren rock - No minerals found");
                }
                m_statusText = buf;
                m_statusTextTimer = 5.0f;
            }
            m_geologistState = GEOLOGIST_NONE;
        } else {
            // Update status text with remaining time
            char buf[48];
            _snprintf(buf, sizeof(buf), "Geologist working... %.0f sec", m_geologistTimer);
            m_statusText = buf;
            m_statusTextTimer = 0.0f;
        }
    }

    // ─── Input handling ──────────────────────────────────────────────
    if (m_inputManager) {
        Input::Gamepad* pad = m_inputManager->GetGamepad();
        if (pad) {
            bool rbPressed = pad->IsButtonPressed(Input::GP_RB);
            bool bPressed = pad->IsButtonPressed(Input::GP_B);
            bool aPressed = pad->IsButtonPressed(Input::GP_A);
            bool yPressed = pad->IsButtonPressed(Input::GP_Y);

            // Back button toggles logistics debug overlay
            if (pad->IsButtonPressed(Input::GP_Back)) {
                m_logisticsDebug = !m_logisticsDebug;
                char dbg[64];
                _snprintf(dbg, sizeof(dbg), "[GameScene] Logistics debug %s\n", m_logisticsDebug ? "ON" : "OFF");
                OutputDebugStringA(dbg);
                m_statusText = m_logisticsDebug ? "LOGISTICS DEBUG ON" : "LOGISTICS DEBUG OFF";
                m_statusTextTimer = 2.0f;
            }

            if (m_buildState == BUILDSTATE_PLACE_FLAG) {
                // Flag placement mode: A to place flag with pending building, B to cancel
                if (aPressed) {
                    PlaceFlag(m_cursorTileX, m_cursorTileY);
                } else if (bPressed) {
                    m_buildState = BUILDSTATE_NONE;
                    m_placementIconIdx = -1;
                    m_placementConstrIdx = -1;
                    m_selectedBuilding = World::Building_None;
                    m_statusText = "Placement cancelled";
                    m_statusTextTimer = 2.0f;
                    OutputDebugStringA("[GameScene] Flag placement cancelled\n");
                }
        } else if (m_buildState == BUILDSTATE_PLACE_ROAD) {
                // Road building mode (tile-by-tile): A to add tile, B to cancel
                if (aPressed) {
                    TryAddRoadTile(m_cursorTileX, m_cursorTileY);
                } else if (bPressed) {
                    m_statusText = "Road cancelled";
                    m_statusTextTimer = 2.0f;
                    CancelRoad();
                    OutputDebugStringA("[GameScene] Road cancelled\n");
                }
                } else if (m_buildState == BUILDSTATE_CONFIRM) {
                // Confirm action: A to confirm, B to cancel
                if (aPressed) {
                    if (m_confirmAction == CONFIRM_PLACE_FLAG) {
                        PlaceFreeFlag(m_confirmTargetX, m_confirmTargetY);
                        m_statusText = "Flag placed!";
                        m_statusTextTimer = 2.0f;
                        m_buildState = BUILDSTATE_NONE;
                        OutputDebugStringA("[GameScene] Confirm: free flag placed\n");
                    } else if (m_confirmAction == CONFIRM_START_ROAD) {
                        StartRoad(m_confirmTargetX, m_confirmTargetY);
                        // StartRoad sets BUILDSTATE_PLACE_ROAD — don't reset
                        OutputDebugStringA("[GameScene] Confirm: starting road\n");
                    } else if (m_confirmAction == CONFIRM_DELETE_FLAG) {
                        OutputDebugStringA("[GameScene] Confirm: deleting flag\n");
                        ConfirmDeleteFlag(m_confirmTargetX, m_confirmTargetY);
                        m_confirmAction = CONFIRM_NONE;
                        m_buildState = BUILDSTATE_NONE;
                    }
                    m_confirmAction = CONFIRM_NONE;
                } else if (bPressed) {
                    m_statusText = "Cancelled";
                    m_statusTextTimer = 2.0f;
                    m_buildState = BUILDSTATE_NONE;
                    m_confirmAction = CONFIRM_NONE;
                    OutputDebugStringA("[GameScene] Confirm cancelled\n");
                }
            } else if (m_menuActive) {
                // Build menu is open
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
                            std::tr1::shared_ptr<SpriteAtlas> iconAtlas = TextureRegistry::instance().getAtlas("Icon");
                            if (iconAtlas) {
                                const SpriteRegion* reg = iconAtlas->GetRegion(selIdx);
                                if (reg) {
                                    m_selectedIconName = reg->name;
                                    m_placementIconIdx = selIdx;
                                    // Extract building name from icon name
                                    std::string buildingName = reg->name;
                                    if (buildingName.compare(0, 3, "ib_") == 0) {
                                        buildingName = buildingName.substr(3);
                                    }
                                    // Map icon name to BuildingType (case-insensitive)
                                    World::BuildingType bt = World::Building_None;
                                    std::string lowerName = buildingName;
                                    for (size_t ci = 0; ci < lowerName.size(); ++ci)
                                        if (lowerName[ci] >= 'A' && lowerName[ci] <= 'Z')
                                            lowerName[ci] = lowerName[ci] - 'A' + 'a';
                                    if (lowerName == "woodcutter") bt = World::Woodcutter;
                                    else if (lowerName == "forester") bt = World::Forester;
                                    else if (lowerName == "sawmill") bt = World::Sawmill;
                                    else if (lowerName == "stonemason") bt = World::Stonemason;
                                    else if (lowerName == "coalmine") bt = World::CoalMine;
                                    else if (lowerName == "ironmine") bt = World::IronMine;
                                    else if (lowerName == "goldmine") bt = World::GoldMine;
                                    else if (lowerName == "ironsmelter") bt = World::IronSmelter;
                                    else if (lowerName == "goldsmelter") bt = World::GoldSmelter;
                                    else if (lowerName == "farm") bt = World::Farm;
                                    else if (lowerName == "mill") bt = World::Mill;
                                    else if (lowerName == "bakery") bt = World::Bakery;
                                    else if (lowerName == "fisher") bt = World::Fisher;
                                    else if (lowerName == "hunter") bt = World::Hunter;
                                    else if (lowerName == "toolworkshop") bt = World::ToolWorkshop;
                                    else if (lowerName == "warehouse" || lowerName == "storehouse") bt = World::Storehouse;
                                    else if (lowerName == "residence") bt = World::Residence;
                                    else if (lowerName == "stronghold") bt = World::Stronghold;
                                    else if (lowerName == "well") bt = World::Well;
                                    else if (lowerName == "bronzemine") bt = World::BronzeMine;
                                    else if (lowerName == "bronzesmelter") bt = World::BronzeSmelter;
                                    else if (lowerName == "toolmaker") bt = World::ToolWorkshop;
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
                                            m_placementConstrIdx = -1;
                                            OutputDebugStringA("[GameScene] WARNING: 'construction' not found in Buildings atlas, using -1\n");
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
            } else if (m_flagMenuActive) {
                // Flag/UIMenu is open
                if (m_flagMenu) {
                    m_flagMenu->Update(pad, deltaTime);

                    if (bPressed || !m_flagMenu->IsVisible()) {
                        m_flagMenuActive = false;
                        m_flagMenu->Hide();
                        OutputDebugStringA("[GameScene] Flag menu closed\n");
                    }

                    if (m_flagMenu->HasSelection()) {
                        int selIdx = m_flagMenu->GetSelectedIndex();
                        if (selIdx >= 0 && selIdx < m_flagMenuItemCount) {
                            if (selIdx == 0) {
                                // Set Flag
                                PlaceFreeFlag(m_cursorTileX, m_cursorTileY);
                                m_statusText = "Flag placed!";
                                m_statusTextTimer = 2.0f;
                            } else if (selIdx == 1) {
                                // Delete Flag
                                OutputDebugStringA("[FlagMenu] Deleting flag...\n");
                                if (m_flagManager) {
                                    World::Flag* f = m_flagManager->GetFlagAt(m_confirmTargetX, m_confirmTargetY);
                                    if (!f) {
                                        // Search nearby for nearest flag
                                        int nearestDist = 999;
                                        for (int dy = -1; dy <= 1; ++dy) {
                                            for (int dx = -1; dx <= 1; ++dx) {
                                                World::Flag* tf = m_flagManager->GetFlagAt(m_confirmTargetX + dx, m_confirmTargetY + dy);
                                                if (tf) { f = tf; break; }
                                            }
                                            if (f) break;
                                        }
                                    }
                                    if (f) {
                                        if (f->type == World::FLAG_WAREHOUSE) {
                                            m_statusText = "Cannot delete town hall flag!";
                                            m_statusTextTimer = 2.0f;
                                        } else if (f->building || f->pendingBuilding != World::Building_None) {
                                            m_confirmAction = CONFIRM_DELETE_FLAG;
                                            m_confirmTargetX = f->pos.x;
                                            m_confirmTargetY = f->pos.y;
                                            m_buildState = BUILDSTATE_CONFIRM;
                                            m_statusText = "Delete building and flag? A=Yes B=No";
                                            m_statusTextTimer = 3.0f;
                                        } else {
                                            ClearRoadTilesForFlag(f);
                                            m_objectLifecycleManager->ForceDeleteFlag(f);
                                            m_statusText = "Flag removed!";
                                            m_statusTextTimer = 2.0f;
                                        }
                                    } else {
                                        m_statusText = "No flag found nearby";
                                        m_statusTextTimer = 2.0f;
                                    }
                                }
                            } else if (selIdx == 2) {
                                // Build Road → start road building from nearest flag or new flag
                                m_flagMenuActive = false;
                                m_flagMenu->Hide();
                                if (m_flagManager && !m_flagManager->GetFlagAt(m_confirmTargetX, m_confirmTargetY)) {
                                    PlaceFreeFlag(m_confirmTargetX, m_confirmTargetY);
                                }
                                StartRoad(m_confirmTargetX, m_confirmTargetY);
                                OutputDebugStringA("[GameScene] Flag menu: Build Road\n");
                            }
                        }
                        m_flagMenuActive = false;
                        m_flagMenu->Hide();
                        m_flagMenu->ResetSelection();
                    }
                }
            } else if (m_townHallPanelOpen) {
                // Town hall resource panel is open → B to close
                if (bPressed) {
                    m_townHallPanelOpen = false;
                    m_statusText = "";
                    m_statusTextTimer = 0.0f;
                }
            } else {
                // Normal mode: A to open town hall or road menu, RB to open build menu
                if (bPressed && m_geologistState == GEOLOGIST_CONFIRM) {
                    CancelGeologistMenu();
                }
                if (aPressed) {
                    // Geologist confirm menu on mountain takes priority
                    bool handled = false;
                    if (m_map) {
                        const World::Tile& objTile = m_map->GetTile(World::Objects, m_cursorTileX, m_cursorTileY);
                        if (objTile.type == World::Mountain || objTile.type == World::MountainOnWater || objTile.type == World::Rock) {
                            if (m_geologistState == GEOLOGIST_CONFIRM) {
                                StartGeologistSurvey();
                            } else if (m_geologistState == GEOLOGIST_NONE) {
                                // Check if already surveyed
                                const World::ResourceNode& node = m_map->GetResourceNode(m_cursorTileX, m_cursorTileY);
                                if (!node.surveyed) {
                                    ShowGeologistConfirm(m_cursorTileX, m_cursorTileY);
                                } else {
                                    m_statusText = "This mountain is already surveyed";
                                    m_statusTextTimer = 2.0f;
                                }
                            }
                            handled = true;
                        }
                    }
                    if (!handled && m_cursorOnTownHall) {
                        m_townHallPanelOpen = true;
                    }
                }
                // ─── Y button: Flag construction menu (skip mountain & water) ─
                if (yPressed) {
                    bool skipFlagMenu = false;
                    if (m_map) {
                        // 1. Skip mountain tiles (have collision)
                        const World::Tile& objTile = m_map->GetTile(World::Objects, m_cursorTileX, m_cursorTileY);
                        if (objTile.type == World::Mountain || objTile.type == World::MountainOnWater || objTile.type == World::Rock) {
                            skipFlagMenu = true;
                        }
                        // 2. Skip water / shallow
                        if (!skipFlagMenu) {
                            BYTE weight = m_map->GetNodeWeight(m_cursorTileX, m_cursorTileY);
                            if (weight == World::Weight_Deep || weight == World::Weight_Shallow) {
                                skipFlagMenu = true;
                            }
                        }
                    }
                    if (!skipFlagMenu) {
                        // Search 3x3 neighborhood for nearest flag
                        World::Flag* nearestFlag = NULL;
                        int nearestDist = 999;
                        int flagX = m_cursorTileX, flagY = m_cursorTileY;
                        if (m_flagManager) {
                            for (int dy = -1; dy <= 1; ++dy) {
                                for (int dx = -1; dx <= 1; ++dx) {
                                    World::Flag* f = m_flagManager->GetFlagAt(m_cursorTileX + dx, m_cursorTileY + dy);
                                    if (f) {
                                        int dist = abs(dx) + abs(dy);
                                        if (dist < nearestDist) {
                                            nearestDist = dist;
                                            nearestFlag = f;
                                            flagX = m_cursorTileX + dx;
                                            flagY = m_cursorTileY + dy;
                                        }
                                    }
                                }
                            }
                        }
                        if (nearestFlag) {
                            m_confirmTargetX = flagX;
                            m_confirmTargetY = flagY;
                            m_flagMenuActive = true;
                            m_flagMenu->Show();
                            OutputDebugStringA("[GameScene] Flag menu opened on flag\n");
                        } else {
                            m_confirmTargetX = m_cursorTileX;
                            m_confirmTargetY = m_cursorTileY;
                            m_flagMenuActive = true;
                            m_flagMenu->Show();
                            OutputDebugStringA("[GameScene] Flag menu opened on ground\n");
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

    // ─── Phase A0: Construction resource transfer (before economy) ────
    if (m_constructionManager) {
        m_constructionManager->Update(deltaTime);
    }

    // ─── Phase A: Economy (synchronous) ───────────────────────────────
    if (m_economyManager && m_carrierManager) {
        // Generate construction resource requests before economy processes them
        if (m_constructionManager) {
            m_constructionManager->GenerateRequests(m_economyManager);
        }
        m_economyManager->Update(deltaTime);
        {
            static int carrierLogCounter = 0;
            if (++carrierLogCounter % 120 == 0) {
                char dbg[256];
                _snprintf(dbg, sizeof(dbg), "[Status] Carriers=%d Flags=%d\n",
                    m_carrierManager->GetCarrierCount(), m_flagManager->GetCount());
                OutputDebugStringA(dbg);
            }
        }
    }

    // ─── Wildlife movement & spawn update ───────────────────────────
        if (m_wildlife) {
            m_wildlife->Update(deltaTime, m_map->GetHabitatRegistry());
        }

        // ─── Wildlife resource node regeneration ─────────────────────
        if (m_map) {
            m_wildlifeRegenTimer += deltaTime;
            if (m_wildlifeRegenTimer >= 60.0f) {
                m_wildlifeRegenTimer = 0.0f;
                m_map->RegenerateWildlifeResources();
            }
            m_treeGrowthTimer += deltaTime;
            if (m_treeGrowthTimer >= 30.0f) {
                m_treeGrowthTimer = 0.0f;
                m_map->GrowTrees();
            }
        }

        // ─── Phase Aw: Worker arrival movement ─────────────────────────
        if (m_workerManager) {
            m_workerManager->Update(deltaTime);
        }

        // ─── Phase B: Carrier updates (per-segment walking) ──────────────
        if (m_carrierManager)
            m_carrierManager->Update(deltaTime);

        // ─── Phase D: Warehouse collects from its flag (AFTER carriers) ──
        if (m_economyManager)
            m_economyManager->CollectWarehouse();

        // ─── Phase D: Collect ground resources to nearest player flag ──
        if (m_map && m_flagManager) {
            // Determine warehouse destination for transport
            uint32_t whFlagId = World::INVALID_FLAG_ID;
            if (m_economyManager) {
                World::Warehouse* wh = m_economyManager->GetWarehouse();
                if (wh && wh->connectedFlag)
                    whFlagId = wh->connectedFlag->id;
            }
            int n = m_map->GetGroundResourceCount();
            for (int gi = n - 1; gi >= 0; --gi) {
                World::GroundResource* gr = m_map->GetGroundResource(gi);
                if (!gr) continue;
                // Wood ground resources are handled by the Woodcutter (CarryWoodHome),
                // not by the generic ground collection loop.
                assert(gr->type != World::ResourceType_Wood || gr->visualOnly);
                if (gr->type == World::ResourceType_Wood)
                    continue;
                // Find nearest player-owned flag with an open slot
                float bestDist = 1e9f;
                World::Flag* bestFlag = NULL;
                for (size_t fi = 0; fi < m_flagManager->GetCount(); ++fi) {
                    World::Flag* f = m_flagManager->GetFlag(fi);
                    if (!f) continue;
                    float dx = (float)(gr->pos.x - f->pos.x);
                    float dy = (float)(gr->pos.y - f->pos.y);
                    float d = dx * dx + dy * dy;
                    if (d < bestDist) {
                        bestDist = d;
                        bestFlag = f;
                    }
                }
                if (bestFlag) {
                    bool added = bestFlag->AddResource(gr->type, 1, whFlagId);
                    if (added) {
                        gr->amount--;
                        char dbg[256];
                        _snprintf(dbg, sizeof(dbg), "[FLAG PUT] flag=%u type=%s amount=1 remaining=%d dst=%u\n",
                                  bestFlag->id,
                                  (gr->type == World::ResourceType_Wood) ? "Wood" : "Resource",
                                  gr->amount, whFlagId);
                        OutputDebugStringA(dbg);
                        if (gr->amount <= 0)
                            m_map->RemoveGroundResource(gi);
                    }
                }
            }
        }

        // ─── Check completed construction sites ────────────────────────
        if (m_constructionManager) {
            const std::vector<World::ConstructionSite*>& sites = m_constructionManager->GetAllSites();
            for (int ci = (int)sites.size() - 1; ci >= 0; --ci) {
                World::ConstructionSite* s = sites[ci];
                if (!s->IsComplete()) continue;
                if (!s->flag->hasBuilding) {
                    ConfirmConstruction(s->flag);
                }
                if (s->builderState == World::Builder_None) {
                    m_constructionManager->RemoveSite(s);
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

//        OutputDebugStringA("[GameScene::Update] DONE\n");
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
            spriteRenderer->SetTextureSlot(SLOT_UI_TOWNHALL_PANEL, uiTex);
        }
        // Icon atlas for build menu icons (separate from UI)
        std::tr1::shared_ptr<SpriteAtlas> iconAtlas = reg.getAtlas("Icon");
        if (iconAtlas && iconAtlas->GetTexture()) {
            spriteRenderer->SetTextureSlot(SLOT_UI_MENU_ICON, iconAtlas->GetTexture());
        } else {
            reg.getTextureOrLoad("Icon");
            iconAtlas = reg.getAtlas("Icon");
            if (iconAtlas && iconAtlas->GetTexture())
                spriteRenderer->SetTextureSlot(SLOT_UI_MENU_ICON, iconAtlas->GetTexture());
            else
                spriteRenderer->SetTextureSlot(SLOT_UI_MENU_ICON, uiAtlas ? uiAtlas->GetTexture() : NULL);
        }
        // Set up dedicated slot for Buildings atlas (for town hall highlight)
        std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
        if (buildingsAtlas && buildingsAtlas->GetTexture()) {
            spriteRenderer->SetTextureSlot(SLOT_BUILDINGS_HIGHLIGHT, buildingsAtlas->GetTexture());
        }
    }

//    OutputDebugStringA("[GameScene::Render] Rendering map\n");
    m_tileRenderer->RenderMap();

    // ─── Full-screen background (LAYER_EFFECTS renders above terrain/world) ─
    {
        TextureRegistry& regBg = TextureRegistry::instance();
        LPDIRECT3DTEXTURE9 bgTex = regBg.getTextureOrLoad("background_game");
        if (bgTex && spriteRenderer) {
            spriteRenderer->SetTextureSlot(SLOT_BACKGROUND, bgTex);
            Graphics::RenderCommandBuilder()
                .UIElement(0, 0, 1280, 720, 0, 0, 1, 1, SLOT_BACKGROUND, 0)
                .Layer(LAYER_EFFECTS)
                .Submit(renderQueue);
        }
    }

    // ─── E/W connection quads for committed road tiles ────────────────
    {
        World::TileLayer* roadsLayer = m_map ? m_map->GetLayer(World::Roads) : NULL;
        if (roadsLayer) {
            TextureRegistry& reg = TextureRegistry::instance();
            std::tr1::shared_ptr<SpriteAtlas> streetsAtlas = reg.getAtlas("streets");
            if (streetsAtlas && streetsAtlas->GetTexture()) {
                spriteRenderer->SetTextureSlot(SLOT_STREETS, streetsAtlas->GetTexture());
                const std::vector<uint32_t>* group = streetsAtlas->GetGroup("street_1");
                if (group && !group->empty()) {
                    uint32_t ewIdx = (*group)[0];
                    const SpriteRegion* ewRegion = streetsAtlas->GetRegion(ewIdx);
                    if (ewRegion) {
                        CoordinateSystem& coords = CoordinateSystem::GetInstance();
                        int rw = roadsLayer->GetWidth();
                        int rh = roadsLayer->GetHeight();
                        for (int y = 0; y < rh; ++y) {
                            for (int x = 0; x < rw - 1; ++x) {
                                const World::Tile& t1 = roadsLayer->GetTile(x, y);
                                if (t1.regionIndex < 0 || t1.atlasName != "streets") continue;
                                const World::Tile& t2 = roadsLayer->GetTile(x + 1, y);
                                if (t2.regionIndex < 0 || t2.atlasName != "streets") continue;
                                float wx1, wy1, wx2, wy2;
                                coords.NodeTileToWorld(x, y, wx1, wy1);
                                coords.NodeTileToWorld(x + 1, y, wx2, wy2);
                                float cx = (wx1 + wx2) * 0.5f;
                                float cy = (wy1 + wy2) * 0.5f;
                                float dx = (float)fabs(wx2 - wx1);
                                Graphics::RenderCommandBuilder()
                                    .WorldSprite(cx - dx * 0.5f, cy - 3.0f,
                                        dx, 6.0f,
                                        ewRegion->u0, ewRegion->v0, ewRegion->u1, ewRegion->v1,
                                        SLOT_STREETS, static_cast<WORD>(30000 + y * 400))
                                    .Submit(renderQueue);
                            }
                        }
                    }
                }
            }
        }
    }

    // ─── Render cursor or placement preview ─────────────────────────────
    if (m_buildState == BUILDSTATE_PLACE_FLAG && m_selectedBuilding != World::Building_None && m_placementManager) {
        m_placementManager->SelectBuilding(m_selectedBuilding);
        PlacementData pd = m_placementManager->GetPlacementData(m_cursorTileX, m_cursorTileY);

        if (pd.spriteRegion) {
            float wx, wy;
            CoordinateSystem::GetInstance().NodeTileToWorld(pd.buildX, pd.buildY, wx, wy);

            if (spriteRenderer) {
                std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
                if (buildingsAtlas && buildingsAtlas->GetTexture())
                    spriteRenderer->SetTextureSlot(SLOT_BUILDINGS_HIGHLIGHT, buildingsAtlas->GetTexture());
            }

            Graphics::RenderCommandBuilder()
                .WorldSprite(wx - pd.spriteRegion->pivotX, wy - pd.spriteRegion->pivotY,
                    (float)pd.spriteRegion->width, (float)pd.spriteRegion->height,
                    pd.spriteRegion->u0, pd.spriteRegion->v0,
                    pd.spriteRegion->u1, pd.spriteRegion->v1,
                    SLOT_BUILDINGS_HIGHLIGHT, static_cast<WORD>(0.98f * 65535.0f))
                .Color(pd.valid ? 0xAAFFFFFF : 0x44FF4444)
                .Layer(LAYER_FOREGROUND)
                .Submit(renderQueue);
        }
//        OutputDebugStringA("[GameScene::Render] Placement preview rendered\n");
    } else if (!m_menuActive && !m_roadMenuActive && !m_flagMenuActive && !m_geologistMenuActive) {
        RenderCursor(renderQueue);
//        OutputDebugStringA("[GameScene::Render] Cursor rendered\n");
    }

    // ─── Render flags (from Buildings atlas, sprite "flag") ────────────
    if (m_flagManager && !m_flagManager->GetFlagPairs().empty() && spriteRenderer) {
        std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
        if (buildingsAtlas && buildingsAtlas->GetTexture()) {
            LPDIRECT3DTEXTURE9 buildingsTex = buildingsAtlas->GetTexture();
            spriteRenderer->SetTextureSlot(SLOT_BUILDINGS_HIGHLIGHT, buildingsTex);

            uint32_t flagIdx = buildingsAtlas->GetIndex("flag");
            const SpriteRegion* flagRegion = buildingsAtlas->GetRegion(flagIdx);
            if (flagRegion) {
                CoordinateSystem& coords = CoordinateSystem::GetInstance();
                const std::vector<std::pair<int,int>>& pairs = m_flagManager->GetFlagPairs();
                for (size_t fi = 0; fi < pairs.size(); ++fi) {
                    int fx = pairs[fi].first;
                    int fy = pairs[fi].second;
                    float wx, wy;
                    coords.NodeTileToWorld(fx, fy, wx, wy);
                    Graphics::RenderCommandBuilder()
                        .WorldSprite(wx - flagRegion->pivotX, wy - flagRegion->pivotY,
                            (float)flagRegion->width, (float)flagRegion->height,
                            flagRegion->u0, flagRegion->v0, flagRegion->u1, flagRegion->v1,
                            SLOT_BUILDINGS_HIGHLIGHT, static_cast<WORD>(30010 + fy * 400))
                        .Submit(renderQueue);
                }
            }
        }
    }

    // ─── Render resource icons on flags ────────────────────────────────
    if (m_flagManager && spriteRenderer) {
        std::tr1::shared_ptr<SpriteAtlas> iconAtlas = reg.getAtlas("Icon");
        if (iconAtlas && iconAtlas->GetTexture()) {
            LPDIRECT3DTEXTURE9 iconTex = iconAtlas->GetTexture();
            spriteRenderer->SetTextureSlot(SLOT_FLAG_RESOURCES, iconTex);

            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            for (size_t fi = 0; fi < m_flagManager->GetCount(); ++fi) {
                World::Flag* flag = m_flagManager->GetFlag(fi);
                if (!flag) continue;
                float fx, fy;
                coords.NodeTileToWorld(flag->pos.x, flag->pos.y, fx, fy);
                int iconY = 0;
                for (int si = 0; si < 8; ++si) {
                    if (flag->slots[si].type == World::ResourceType_None || flag->slots[si].amount <= 0) continue;
                    const char* iconName = NULL;
                    switch (flag->slots[si].type) {
                        case World::ResourceType_Wood:   iconName = "r_wood"; break;
                        case World::ResourceType_Planks: iconName = "r_planks"; break;
                        case World::ResourceType_Stone:  iconName = "r_stone"; break;
                        case World::ResourceType_Fish:   iconName = "r_fish"; break;
                        case World::ResourceType_Meat:   iconName = "r_meat"; break;
                        case World::ResourceType_Bread:  iconName = "r_bread"; break;
                        case World::ResourceType_Coal:   iconName = "r_coal"; break;
                        case World::ResourceType_IronOre: iconName = "r_ironore"; break;
                        case World::ResourceType_GoldOre: iconName = "r_goldore"; break;
                        case World::ResourceType_IronBar: iconName = "r_ironbar"; break;
                        case World::ResourceType_GoldBar: iconName = "r_goldbar"; break;
                        default: break;
                    }
                    if (!iconName) continue;
                    uint32_t idx = iconAtlas->GetIndex(iconName);
                    if (idx == 0xFFFFFFFF) continue;
                    const SpriteRegion* r = iconAtlas->GetRegion(idx);
                    if (!r) continue;

                    Graphics::RenderCommandBuilder()
                        .WorldSprite(fx - r->pivotX * 0.5f, fy - r->pivotY * 0.5f - 30.0f + iconY * -16.0f,
                            r->width * 0.5f, r->height * 0.5f,
                            r->u0, r->v0, r->u1, r->v1,
                            SLOT_FLAG_RESOURCES, static_cast<WORD>(30011 + flag->pos.y * 400 + iconY))
                        .Submit(renderQueue);
                    iconY--;
                }
            }
        }
    }

    // ─── Render carriers and builders ───────────────────────────────────
    if (spriteRenderer) {
        reg.getTextureOrLoad("Units");
        std::tr1::shared_ptr<SpriteAtlas> unitsAtlas = reg.getAtlas("Units");
        if (unitsAtlas && unitsAtlas->GetTexture()) {
            LPDIRECT3DTEXTURE9 unitsTex = unitsAtlas->GetTexture();
            spriteRenderer->SetTextureSlot(SLOT_UNITS, unitsTex);
            CoordinateSystem& coords = CoordinateSystem::GetInstance();

            // Helper: compute Units atlas sprite index for movement direction
            // Carrier (с грузом):    settler_with_cart_*  8=SE, 9=NE, 10=SW, 11=NW
            // Carrier (без груза):    settler_walk_*      0=NE, 1=SE, 2=NW, 3=SW
            // Worker (строитель):     worker_*            4=SE, 5=SW
            auto unitsSpriteIndex = [](bool isCarrier, bool hasCargo, int dx, int dy) -> int {
                if (isCarrier) {
                    if (hasCargo) {
                        if (dy < 0) return (dx >= 0) ? 9 : 11;
                        if (dy > 0) return (dx >= 0) ? 8 : 10;
                        return (dx >= 0) ? 8 : 11;
                    } else {
                        if (dy < 0) return (dx < 0) ? 2 : 0;
                        if (dy > 0) return (dx < 0) ? 3 : 1;
                        return (dx >= 0) ? 1 : 3;
                    }
                } else {
                    return (dx >= 0) ? 4 : 5;
                }
            };

            // Render carriers (per-segment walking)
            static int carrierLogFrame = 0;
            carrierLogFrame++;
            bool logCarriers = (carrierLogFrame % 60 == 0);
            if (m_carrierManager) {
                for (int ci = 0; ci < m_carrierManager->GetCarrierCount(); ++ci) {
                    World::Carrier* carrier = m_carrierManager->GetCarrier(ci);
                    if (!carrier) continue;

                    const Vector2i* pathTiles = NULL;
                    int pathCount = 0;
                    float ep = 0.0f;
                    float walkDir = carrier->walkDir;

                    if (World::IsTransitState(carrier->state)) {
                        if (carrier->transitCount < 2) continue;
                        pathTiles = carrier->transitTiles;
                        pathCount = (int)carrier->transitCount;
                        ep = carrier->transitProgress;
                    } else {
                        if (!carrier->road || carrier->road->tileCount < 2) continue;
                        pathTiles = carrier->road->tiles;
                        pathCount = (int)carrier->road->tileCount;
                        ep = carrier->ep;
                    }

                    int pathLen = pathCount - 1;
                    if (ep < 0.0f) ep = 0.0f;
                    if (ep > (float)pathLen) ep = (float)pathLen;
                    int idx = (int)ep;
                    float frac = ep - (float)idx;
                    if (idx >= pathLen) { idx = pathLen - 1; frac = 1.0f; }
                    if (idx < 0) { idx = 0; frac = 0.0f; }

                    const Vector2i& tileA = pathTiles[idx];
                    const Vector2i& tileB = pathTiles[idx + 1];

                    int dx = (walkDir > 0.0f) ? (tileB.x - tileA.x) : (tileA.x - tileB.x);
                    int dy = (walkDir > 0.0f) ? (tileB.y - tileA.y) : (tileA.y - tileB.y);
                    bool hasCargo = (carrier->m_cargo != NULL);
                    int spriteIdx = unitsSpriteIndex(true, hasCargo, dx, dy);

                    // Carrier graphical behavior log (every ~60 frames)
                    if (logCarriers) {
                        const char* stateNames[] = { "WalkingToPost", "Working", "ReturningHome" };
                        const char* sn = (carrier->state >= 0 && carrier->state < 3) ? stateNames[carrier->state] : "?";
                        const char* cn = carrier->m_cargo ? World::ResourceTypeToString(carrier->m_cargo->type) : "empty";
                        char dbg[256];
                        _snprintf(dbg, sizeof(dbg),
                            "[CARRIER] %d: state=%s ep=%.1f dir=%.1f sprite=%d cargo=%s path=%d tiles=(%d,%d)-(%d,%d)\n",
                            ci, sn, ep, walkDir, spriteIdx, cn, pathLen,
                            tileA.x, tileA.y, tileB.x, tileB.y);
                        OutputDebugStringA(dbg);
                    }

                    float wx0, wy0, wx1, wy1;
                    coords.NodeTileToWorld(tileA.x, tileA.y, wx0, wy0);
                    coords.NodeTileToWorld(tileB.x, tileB.y, wx1, wy1);
                    float wx = wx0 + (wx1 - wx0) * frac;
                    float wy = wy0 + (wy1 - wy0) * frac;

                    const SpriteRegion* r = unitsAtlas->GetRegion(spriteIdx);
                    if (!r) continue;

                    Graphics::RenderCommandBuilder()
                        .WorldSprite(wx - r->pivotX, wy - r->pivotY,
                            (float)r->width, (float)r->height,
                            r->u0, r->v0, r->u1, r->v1,
                            SLOT_UNITS, static_cast<WORD>(30020 + tileA.y * 400))
                        .Submit(renderQueue);

                    // Render cargo icon above carrier
                    if (carrier->m_cargo) {
                        const char* cargoIconName = World::ResourceTypeToIconName(carrier->m_cargo->type);
                        if (cargoIconName && cargoIconName[0]) {
                            std::tr1::shared_ptr<SpriteAtlas> cargoAtlas = reg.getAtlas("Icon");
                            if (cargoAtlas) {
                                uint32_t cargoIdx = cargoAtlas->GetIndex(cargoIconName);
                                if (cargoIdx != 0xFFFFFFFF) {
                                    const SpriteRegion* cargoR = cargoAtlas->GetRegion(cargoIdx);
                                    if (cargoR) {
                                        float cargoSize = 16.0f;
                                        Graphics::RenderCommandBuilder()
                                            .WorldSprite(wx - cargoSize * 0.5f, wy - r->pivotY - cargoSize,
                                                cargoSize, cargoSize,
                                                cargoR->u0, cargoR->v0, cargoR->u1, cargoR->v1,
                                                SLOT_UI_MENU_ICON, static_cast<WORD>(30030 + tileA.y * 400))
                                            .Submit(renderQueue);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Render building workers (Fisher static, Hunter moving)
            if (m_flagManager) {
                for (size_t fi = 0; fi < m_flagManager->GetCount(); ++fi) {
                    World::Flag* flag = m_flagManager->GetFlag((int)fi);
                    if (!flag || !flag->building) continue;

                    float wx, wy;
                    int wSpriteIdx = -1;
                    bool moving = false;

                    // Hunter has a moving worker – query its position via virtual method
                    if (flag->building->GetWorkerRenderInfo(wx, wy, wSpriteIdx)) {
                        moving = true;
                        coords.NodeTileToWorld(wx, wy, wx, wy);
                    }
                    if (wSpriteIdx < 0) continue;
                    const SpriteRegion* wr = unitsAtlas->GetRegion(wSpriteIdx);
                    if (!wr) continue;

                    Graphics::RenderCommandBuilder()
                        .WorldSprite(wx - wr->pivotX, wy - wr->pivotY,
                            (float)wr->width, (float)wr->height,
                            wr->u0, wr->v0, wr->u1, wr->v1,
                            SLOT_UNITS, static_cast<WORD>(30020 + (moving ? (int)(wy + 0.5f) : flag->building->pos.y) * 400))
                        .Submit(renderQueue);
                }
            }

            // Render arriving workers (walking from warehouse to their building)
            if (m_workerManager && unitsAtlas) {
                for (int wi = 0; wi < m_workerManager->GetActiveCount(); ++wi) {
                    const World::Worker* w = m_workerManager->GetWorkerByActiveIdx(wi);
                    if (w->state != World::WorkerState_MovingToJob) continue;
                    float wx = w->posX;
                    float wy = w->posY;
                    int spriteIdx = 4;  // generic walk sprite
                    coords.NodeTileToWorld(wx, wy, wx, wy);
                    const SpriteRegion* wr = unitsAtlas->GetRegion(spriteIdx);
                    if (!wr) continue;
                    Graphics::RenderCommandBuilder()
                        .WorldSprite(wx - wr->pivotX, wy - wr->pivotY,
                            (float)wr->width, (float)wr->height,
                            wr->u0, wr->v0, wr->u1, wr->v1,
                            SLOT_UNITS, static_cast<WORD>(30020 + (int)(wy + 0.5f) * 400))
                        .Submit(renderQueue);
                }
            }

            // Render builders (ep-based movement along road tiles)
            if (m_constructionManager) {
                const std::vector<World::ConstructionSite*>& sites = m_constructionManager->GetAllSites();
                for (size_t si = 0; si < sites.size(); ++si) {
                    World::ConstructionSite* site = sites[si];
                    if (!site || !site->flag) continue;
                    if (site->builderState == World::Builder_None) continue;

                    float wx, wy;
                    int spriteIdx = 4;

                    if (site->builderState == World::Builder_Walking || site->builderState == World::Builder_Returning) {
                        if (site->builderRouteCount < 2) continue;
                        uint32_t fromIdx = site->builderRouteIndex;
                        uint32_t toIdx = fromIdx + 1;
                        if (fromIdx >= site->builderRouteCount - 1) {
                            size_t lastIdx = site->builderRouteCount - 1;
                            World::Flag* f = site->builderRoute[lastIdx];
                            coords.NodeTileToWorld(f->pos.x, f->pos.y, wx, wy);
                        } else {
                            World::Flag* fromFlag = site->builderRoute[fromIdx];
                            World::Flag* toFlag = site->builderRoute[toIdx];
                            World::Road* road = m_roadManager ? m_roadManager->GetRoadBetween(fromFlag, toFlag) : NULL;
                            if (road && road->tileCount >= 2) {
                                int tc = (int)road->tileCount;
                                float pathLen = (float)(tc - 1);
                                float pos = site->builderEp;
                                if (pos < 0.0f) pos = 0.0f;
                                if (pos > pathLen) pos = pathLen;
                                int tileIdx = (int)pos;
                                float frac = pos - (float)tileIdx;
                                if (tileIdx >= tc - 1) { tileIdx = tc - 2; frac = 1.0f; }
                                if (tileIdx < 0) { tileIdx = 0; frac = 0.0f; }
                                const Vector2i& tileA = road->tiles[tileIdx];
                                const Vector2i& tileB = road->tiles[tileIdx + 1];
                                float wx0, wy0, wx1, wy1;
                                coords.NodeTileToWorld(tileA.x, tileA.y, wx0, wy0);
                                coords.NodeTileToWorld(tileB.x, tileB.y, wx1, wy1);
                                wx = wx0 + (wx1 - wx0) * frac;
                                wy = wy0 + (wy1 - wy0) * frac;
                                int bdx = tileB.x - tileA.x;
                                int bdy = tileB.y - tileA.y;
                                spriteIdx = unitsSpriteIndex(false, false, bdx, bdy);
                            } else {
                                float wx0, wy0, wx1, wy1;
                                coords.NodeTileToWorld(fromFlag->pos.x, fromFlag->pos.y, wx0, wy0);
                                coords.NodeTileToWorld(toFlag->pos.x, toFlag->pos.y, wx1, wy1);
                                float pathLen = 1.0f;
                                float t = (pathLen > 0.0f) ? site->builderEp / pathLen : 0.0f;
                                if (t < 0.0f) t = 0.0f;
                                if (t > 1.0f) t = 1.0f;
                                wx = wx0 + (wx1 - wx0) * t;
                                wy = wy0 + (wy1 - wy0) * t;
                                int bdx = toFlag->pos.x - fromFlag->pos.x;
                                int bdy = toFlag->pos.y - fromFlag->pos.y;
                                spriteIdx = unitsSpriteIndex(false, false, bdx, bdy);
                            }
                        }
                    } else if (site->builderState == World::Builder_Building) {
                        coords.NodeTileToWorld(site->flag->pos.x, site->flag->pos.y, wx, wy);
                    } else {
                        coords.NodeTileToWorld(site->flag->pos.x, site->flag->pos.y, wx, wy);
                    }

                    const SpriteRegion* r = unitsAtlas->GetRegion(spriteIdx);
                    if (!r) continue;

                    Graphics::RenderCommandBuilder()
                        .WorldSprite(wx - r->pivotX, wy - r->pivotY,
                            (float)r->width, (float)r->height,
                            r->u0, r->v0, r->u1, r->v1,
                            SLOT_UNITS, static_cast<WORD>(30020 + site->flag->pos.y * 400))
                        .Submit(renderQueue);
                }
            }
        }
    }

    // ─── Render wildlife (animal sprites) ───────────────────────────
    if (m_wildlife) {
        const std::vector<World::Animal>& animals = m_wildlife->GetAllAnimals();
        if (!animals.empty()) {
            reg.getTextureOrLoad("Units");
            std::tr1::shared_ptr<SpriteAtlas> unitsAtlas = reg.getAtlas("Units");
            if (unitsAtlas && unitsAtlas->GetTexture()) {
                LPDIRECT3DTEXTURE9 unitsTex = unitsAtlas->GetTexture();
                spriteRenderer->SetTextureSlot(SLOT_UNITS, unitsTex);
                const std::vector<uint32_t>* animalGroup = unitsAtlas->GetGroup("Animals");
                if (animalGroup && !animalGroup->empty()) {
                    CoordinateSystem& coords = CoordinateSystem::GetInstance();
                    for (size_t i = 0; i < animals.size(); ++i) {
                        const World::Animal& a = animals[i];
                        if (a.state != World::AnimalState_Alive) continue;
                        if (a.type < 0 || a.type >= World::AnimalType_Count) continue;

                        int rawIdx = (int)a.type;
                        int dirIdx = World::VelocityToDirIndex(a.vx, a.vy);
                        int dirSpriteIdx = rawIdx * World::AnimalDirSpriteCount() + dirIdx;
                        int spriteIdx;
                        if (dirSpriteIdx < (int)animalGroup->size()) {
                            spriteIdx = dirSpriteIdx;
                        } else if (rawIdx < (int)animalGroup->size()) {
                            spriteIdx = rawIdx;
                        } else {
                            continue;
                        }
                        uint32_t regionIdx = (*animalGroup)[spriteIdx];
                        const SpriteRegion* r = unitsAtlas->GetRegion(regionIdx);
                        if (!r) continue;
                        float wx, wy;
                        coords.NodeTileToWorld(a.x, a.y, wx, wy);
                        Graphics::RenderCommandBuilder()
                            .WorldSprite(wx - r->pivotX, wy - r->pivotY,
                                (float)r->width, (float)r->height,
                                r->u0, r->v0, r->u1, r->v1,
                                SLOT_UNITS, static_cast<WORD>(30005 + (int)(a.y + 0.5f) * 400))
                            .Submit(renderQueue);
                    }
                }
            }
        }
    }

    // ─── Render road preview (placed tiles) ─────────────────────────
    if (m_buildState == BUILDSTATE_PLACE_ROAD && !m_roadPreviewPath.empty()) {
        std::tr1::shared_ptr<SpriteAtlas> streetsAtlas = reg.getAtlas("streets");
        if (!streetsAtlas) return;
        CoordinateSystem& coords = CoordinateSystem::GetInstance();

        // Compute X offset so road preview tiles visually align with flag sprite's center
        float flagAlignOffsetX = 0.0f;
        {   std::tr1::shared_ptr<SpriteAtlas> ba = reg.getAtlas("Buildings");
            uint32_t fi = ba->GetIndex("flag"); const SpriteRegion* fr = ba->GetRegion(fi);
            if (fr) { const std::vector<uint32_t>* rg = streetsAtlas->GetGroup("street_1");
            if (rg && !rg->empty()) { const SpriteRegion* rr = streetsAtlas->GetRegion((*rg)[0]);
            if (rr) { flagAlignOffsetX = (fr->width * 0.5f - fr->pivotX) - (rr->width * 0.5f - rr->pivotX); }}}}

        for (size_t i = 0; i < m_roadPreviewPath.size(); ++i) {
            int px = m_roadPreviewPath[i].first;
            int py = m_roadPreviewPath[i].second;
            World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
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

            Graphics::RenderCommandBuilder()
                .WorldSprite(wx - region->pivotX + flagAlignOffsetX, wy - region->pivotY,
                    (float)region->width, (float)region->height,
                    region->u0, region->v0, region->u1, region->v1,
                    SLOT_STREETS, static_cast<WORD>(0.98f * 65535.0f))
                .Color(D3DCOLOR_ARGB(160, 255, 255, 255))
                .Layer(LAYER_FOREGROUND)
                .Submit(renderQueue);
        }

        // E/W connection quads for preview path
        const std::vector<uint32_t>* ewGroup = streetsAtlas->GetGroup("street_1");
        if (ewGroup && !ewGroup->empty()) {
            uint32_t ewIdx = (*ewGroup)[0];
            const SpriteRegion* ewRegion = streetsAtlas->GetRegion(ewIdx);
            if (ewRegion) {
                for (size_t i = 0; i + 1 < m_roadPreviewPath.size(); ++i) {
                    int x1 = m_roadPreviewPath[i].first;
                    int y1 = m_roadPreviewPath[i].second;
                    int x2 = m_roadPreviewPath[i + 1].first;
                    int y2 = m_roadPreviewPath[i + 1].second;
                    if (abs(x1 - x2) == 1 && y1 == y2) {
                        float wx1, wy1, wx2, wy2;
                        coords.NodeTileToWorld(x1, y1, wx1, wy1);
                        coords.NodeTileToWorld(x2, y2, wx2, wy2);
                        float cx = (wx1 + wx2) * 0.5f;
                        float cy = (wy1 + wy2) * 0.5f;
                        float dx = (float)fabs(wx2 - wx1);
                        Graphics::RenderCommandBuilder()
                            .WorldSprite(cx - dx * 0.5f + flagAlignOffsetX, cy - 3.0f,
                                dx, 6.0f,
                                ewRegion->u0, ewRegion->v0, ewRegion->u1, ewRegion->v1,
                                SLOT_STREETS, static_cast<WORD>(0.98f * 65535.0f))
                            .Color(D3DCOLOR_ARGB(160, 255, 255, 255))
                            .Layer(LAYER_FOREGROUND)
                            .Submit(renderQueue);
                    }
                }
            }
        }
    }

    // ─── Render auto-path preview (blue, when cursor on flag) ─────
    if (m_buildState == BUILDSTATE_PLACE_ROAD && !m_roadAutoPath.empty()) {
        std::tr1::shared_ptr<SpriteAtlas> streetsAtlas = reg.getAtlas("streets");
        if (streetsAtlas) {
            CoordinateSystem& coords = CoordinateSystem::GetInstance();

            float flagAlignOffsetX = 0.0f;
            {   std::tr1::shared_ptr<SpriteAtlas> ba = reg.getAtlas("Buildings");
                uint32_t fi = ba->GetIndex("flag"); const SpriteRegion* fr = ba->GetRegion(fi);
                if (fr) { const std::vector<uint32_t>* rg = streetsAtlas->GetGroup("street_1");
                if (rg && !rg->empty()) { const SpriteRegion* rr = streetsAtlas->GetRegion((*rg)[0]);
                if (rr) { flagAlignOffsetX = (fr->width * 0.5f - fr->pivotX) - (rr->width * 0.5f - rr->pivotX); }}}}

            const std::vector<uint32_t>* group = streetsAtlas->GetGroup("street_1");
            if (group && !group->empty()) {
                uint32_t regionIdx = (*group)[0];
                const SpriteRegion* region = streetsAtlas->GetRegion(regionIdx);
                if (region) {
                    for (size_t i = 0; i < m_roadAutoPath.size(); ++i) {
                        int ax = m_roadAutoPath[i].first;
                        int ay = m_roadAutoPath[i].second;
                        float wx, wy;
                        coords.NodeTileToWorld(ax, ay, wx, wy);
                        Graphics::RenderCommandBuilder()
                            .WorldSprite(wx - region->pivotX + flagAlignOffsetX, wy - region->pivotY,
                                (float)region->width, (float)region->height,
                                region->u0, region->v0, region->u1, region->v1,
                                SLOT_STREETS, static_cast<WORD>(0.98f * 65535.0f))
                            .Color(D3DCOLOR_ARGB(160, 100, 200, 255))
                            .Layer(LAYER_FOREGROUND)
                            .Submit(renderQueue);
                    }
                }
            }

            // E/W connection quads for auto-path
            const std::vector<uint32_t>* ewGroup = streetsAtlas->GetGroup("street_1");
            if (ewGroup && !ewGroup->empty()) {
                uint32_t ewIdx = (*ewGroup)[0];
                const SpriteRegion* ewRegion = streetsAtlas->GetRegion(ewIdx);
                if (ewRegion) {
                    for (size_t i = 0; i + 1 < m_roadAutoPath.size(); ++i) {
                        int x1 = m_roadAutoPath[i].first;
                        int y1 = m_roadAutoPath[i].second;
                        int x2 = m_roadAutoPath[i + 1].first;
                        int y2 = m_roadAutoPath[i + 1].second;
                        if (abs(x1 - x2) == 1 && y1 == y2) {
                            float wx1, wy1, wx2, wy2;
                            coords.NodeTileToWorld(x1, y1, wx1, wy1);
                            coords.NodeTileToWorld(x2, y2, wx2, wy2);
                            float cx = (wx1 + wx2) * 0.5f;
                            float cy = (wy1 + wy2) * 0.5f;
                            float dx = (float)fabs(wx2 - wx1);
                            Graphics::RenderCommandBuilder()
                                .WorldSprite(cx - dx * 0.5f + flagAlignOffsetX, cy - 3.0f,
                                    dx, 6.0f,
                                    ewRegion->u0, ewRegion->v0, ewRegion->u1, ewRegion->v1,
                                    SLOT_STREETS, static_cast<WORD>(0.98f * 65535.0f))
                                .Color(D3DCOLOR_ARGB(160, 100, 200, 255))
                                .Layer(LAYER_FOREGROUND)
                                .Submit(renderQueue);
                        }
                    }
                }
            }
        }
    }

    // ─── Render valid neighbor tiles (green) ────────────────
    if (m_buildState == BUILDSTATE_PLACE_ROAD && !m_roadValidNeighbors.empty()) {
        std::tr1::shared_ptr<SpriteAtlas> streetsAtlas = reg.getAtlas("streets");
        if (streetsAtlas) {
            CoordinateSystem& coords = CoordinateSystem::GetInstance();

            float flagAlignOffsetX = 0.0f;
            {   std::tr1::shared_ptr<SpriteAtlas> ba = reg.getAtlas("Buildings");
                uint32_t fi = ba->GetIndex("flag"); const SpriteRegion* fr = ba->GetRegion(fi);
                if (fr) { const std::vector<uint32_t>* rg = streetsAtlas->GetGroup("street_1");
                if (rg && !rg->empty()) { const SpriteRegion* rr = streetsAtlas->GetRegion((*rg)[0]);
                if (rr) { flagAlignOffsetX = (fr->width * 0.5f - fr->pivotX) - (rr->width * 0.5f - rr->pivotX); }}}}

            const std::vector<uint32_t>* group = streetsAtlas->GetGroup("street_1");
            if (group && !group->empty()) {
                uint32_t regionIdx = (*group)[0];
                const SpriteRegion* region = streetsAtlas->GetRegion(regionIdx);
                if (region) {
                    for (size_t i = 0; i < m_roadValidNeighbors.size(); ++i) {
                        int nx = m_roadValidNeighbors[i].first;
                        int ny = m_roadValidNeighbors[i].second;
                        float wx, wy;
                        coords.NodeTileToWorld(nx, ny, wx, wy);
                        Graphics::RenderCommandBuilder()
                            .WorldSprite(wx - region->pivotX + flagAlignOffsetX, wy - region->pivotY,
                                (float)region->width, (float)region->height,
                                region->u0, region->v0, region->u1, region->v1,
                                SLOT_STREETS, static_cast<WORD>(0.99f * 65535.0f))
                            .Color(D3DCOLOR_ARGB(120, 255, 100, 100))
                            .Layer(LAYER_FOREGROUND)
                            .Submit(renderQueue);
                    }
                }
            }
        }
    }

    // ─── Render build menu (if active) ───────────────────────────────────
    if (m_buildMenu && m_menuActive) {
        m_buildMenu->Render();
    }

    // ─── Render flag/UIMenu (if active) ──────────────────────────────────
    if (m_flagMenu && m_flagMenuActive) {
        m_flagMenu->Render();
    }

    // ─── Hunting spots overlay when hunter building is selected ─────────
    if (m_flagMenuActive && m_flagManager && m_map && m_textManager) {
        World::Flag* flag = m_flagManager->GetFlagAt(m_confirmTargetX, m_confirmTargetY);
        if (flag && flag->building && flag->building->type == World::Hunter) {
            Logic::ResourceRegistry* registry = m_map->GetResourceRegistry();
            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            std::tr1::shared_ptr<SpriteAtlas> iconAtlas = TextureRegistry::instance().getAtlas("Icon");
            if (registry && iconAtlas) {
                uint32_t deerIcon = iconAtlas->GetIndex("r_deer");
                if (deerIcon != 0xFFFFFFFF) {
                    const SpriteRegion* deerR = iconAtlas->GetRegion(deerIcon);
                    const std::vector<Vector2i>& spawners = registry->GetWorldResources(World::ResourceType_WildlifeSpawner_Deer);
                    for (size_t si = 0; si < spawners.size(); ++si) {
                        const World::ResourceNode& node = m_map->GetResourceNode(spawners[si].x, spawners[si].y);
                        if (node.type != World::ResourceType_WildlifeSpawner_Deer) continue;
                        float wx, wy;
                        coords.NodeTileToWorld((float)spawners[si].x, (float)spawners[si].y, wx, wy);
                        // Render deer icon
                        if (deerR) {
                            float iconSize = 20.0f;
                            Graphics::RenderCommandBuilder()
                                .WorldSprite(wx - iconSize * 0.5f, wy - iconSize,
                                    iconSize, iconSize,
                                    deerR->u0, deerR->v0, deerR->u1, deerR->v1,
                                    SLOT_UI_MENU_ICON, static_cast<WORD>(0.99f * 65535.0f))
                                .Color(D3DCOLOR_ARGB(200, 255, 255, 255))
                                .Layer(LAYER_FOREGROUND)
                                .Submit(renderQueue);
                        }
                        // Render amount text
                        char buf[8];
                        _snprintf(buf, sizeof(buf), "%d", node.amount);
                        m_textManager->DrawTextToWorld(buf, wx, wy - 28.0f, D3DCOLOR_ARGB(255, 255, 255, 0), 0.07f);
                    }
                }
            }
        }
    }

    // ─── Ground resource overlays (sprite + count text) ────────────────
    {
        TextureRegistry& reg2 = TextureRegistry::instance();
        std::tr1::shared_ptr<SpriteAtlas> iconAtlas = reg2.getAtlas("Icon");
        if (iconAtlas && m_map && m_textManager) {
            if (!m_groundWoodIconLoaded) {
                uint32_t idx = iconAtlas->GetIndex("r_exotic_wood");
                m_groundWoodIconIdx = (idx != 0xFFFFFFFF) ? (int)idx : -1;
                m_groundWoodIconLoaded = true;
            }
            int n = m_map->GetGroundResourceCount();
            if (n > 0) {
                CoordinateSystem& coords = CoordinateSystem::GetInstance();
                int iconIdx = m_groundWoodIconIdx;
                const SpriteRegion* iconR = (iconIdx >= 0) ? iconAtlas->GetRegion(iconIdx) : NULL;
                for (int gi = 0; gi < n; ++gi) {
                    World::GroundResource* gr = m_map->GetGroundResource(gi);
                    if (!gr) continue;
                    float wx, wy;
                    coords.NodeTileToWorld(gr->pos.x, gr->pos.y, wx, wy);
                    // Render icon sprite
                    if (iconR) {
                        float iconSize = 24.0f;
                        Graphics::RenderCommandBuilder()
                            .WorldSprite(wx - iconSize * 0.5f, wy - iconSize - 8.0f,
                                iconSize, iconSize,
                                iconR->u0, iconR->v0, iconR->u1, iconR->v1,
                                SLOT_UI_MENU_ICON, static_cast<WORD>(0.99f * 65535.0f))
                            .Color(D3DCOLOR_ARGB(220, 255, 255, 255))
                            .Layer(LAYER_FOREGROUND)
                            .Submit(renderQueue);
                    }
                    // Render amount text
                    char buf[16];
                    _snprintf(buf, sizeof(buf), "%d", gr->amount);
                    m_textManager->DrawTextToWorld(buf, wx, wy - 40.0f, D3DCOLOR_ARGB(255, 255, 255, 0), 0.08f);
                }
            }
        }
    }

    if (!m_menuActive && !m_roadMenuActive && !m_flagMenuActive && !m_geologistMenuActive && !m_townHallPanelOpen) {
        // ─── Town hall highlight when cursor is over it ──────────────────
        if (m_cursorOnTownHall) {
            std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
            if (buildingsAtlas) {
                uint32_t thIdx = buildingsAtlas->GetIndex("b_townhall");
                if (thIdx != 0xFFFFFFFF) {
                    const SpriteRegion* r = buildingsAtlas->GetRegion(thIdx);
                    if (r) {
                        float wx, wy;
                        CoordinateSystem::GetInstance().NodeTileToWorld(10, 8, wx, wy);
                        Graphics::RenderCommandBuilder()
                            .WorldSprite(wx - r->pivotX, wy - r->pivotY,
                                (float)r->width, (float)r->height,
                                r->u0, r->v0, r->u1, r->v1,
                                SLOT_BUILDINGS_HIGHLIGHT, static_cast<WORD>(0.99f * 65535.0f))
                            .Color(D3DCOLOR_ARGB(80, 255, 255, 255))
                            .Layer(LAYER_FOREGROUND)
                            .Submit(renderQueue);
                    }
                }
            }
        }
    }

    // ─── Town hall info panel ──────────────────────────────────────────
    if (m_townHallPanelOpen && m_townHallPanelBgIdx >= 0) {
        LPDIRECT3DTEXTURE9 uiTex = NULL;
        std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
        if (uiAtlas) uiTex = uiAtlas->GetTexture();
        if (spriteRenderer && uiTex) {
            float screenW = 1280.0f;
            float screenH = 720.0f;
            float panelLeft = (screenW - m_townHallPanelW) * 0.5f;
            float panelTop = (screenH - m_townHallPanelH) * 0.5f;

            Graphics::RenderCommandBuilder()
                .UIElement(panelLeft, panelTop,
                    m_townHallPanelW, m_townHallPanelH,
                    m_townHallPanelU0, m_townHallPanelV0, m_townHallPanelU1, m_townHallPanelV1,
                    SLOT_UI_TOWNHALL_PANEL, 10)
                .Submit(renderQueue);

            // Render resource counts on the panel (total stock across all storage)
            if (m_textManager && m_economyManager) {
                float tx = panelLeft + 40.0f;
                float ty = panelTop + 30.0f;
                float lineH = 28.0f;
                char buf[64];

                _snprintf(buf, sizeof(buf), "Wood: %d", m_economyManager->GetTotalStock(World::ResourceType_Wood));
                m_textManager->DrawString(buf, tx, ty, 0xFFFFFFFF, 0.08f); ty += lineH;
                _snprintf(buf, sizeof(buf), "Planks: %d", m_economyManager->GetTotalStock(World::ResourceType_Planks));
                m_textManager->DrawString(buf, tx, ty, 0xFFFFFFFF, 0.08f); ty += lineH;
                _snprintf(buf, sizeof(buf), "Stone: %d", m_economyManager->GetTotalStock(World::ResourceType_Stone));
                m_textManager->DrawString(buf, tx, ty, 0xFFFFFFFF, 0.08f); ty += lineH;
                _snprintf(buf, sizeof(buf), "Fish: %d", m_economyManager->GetTotalStock(World::ResourceType_Fish));
                m_textManager->DrawString(buf, tx, ty, 0xFFFFFFFF, 0.08f); ty += lineH;
                _snprintf(buf, sizeof(buf), "Meat: %d", m_economyManager->GetTotalStock(World::ResourceType_Meat));
                m_textManager->DrawString(buf, tx, ty, 0xFFFFFFFF, 0.08f); ty += lineH;
                _snprintf(buf, sizeof(buf), "Coal: %d", m_economyManager->GetTotalStock(World::ResourceType_Coal));
                m_textManager->DrawString(buf, tx, ty, 0xFFFFFFFF, 0.08f); ty += lineH;
            }
        }
    }

    // ─── Highlight all buildings when town hall panel is open ────────────
    if (m_townHallPanelOpen && m_flagManager) {
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
        if (buildingsAtlas) {
            for (size_t fi = 0; fi < m_flagManager->GetCount(); ++fi) {
                World::Flag* flag = m_flagManager->GetFlag(fi);
                if (!flag || !flag->building) continue;
                uint32_t sprIdx;
                if (flag->building->IsDepleted() && flag->building->m_depletedSpriteIdx >= 0) {
                    sprIdx = (uint32_t)flag->building->m_depletedSpriteIdx;
                } else {
                    const char* spriteName = GetBuildingSpriteName(flag->building->type);
                    if (!spriteName || !*spriteName) continue;
                    sprIdx = buildingsAtlas->GetIndex(spriteName);
                }
                if (sprIdx == 0xFFFFFFFF) continue;
                const SpriteRegion* r = buildingsAtlas->GetRegion(sprIdx);
                if (!r) continue;
                int bldX = flag->building->pos.x;
                int bldY = flag->building->pos.y;
                float wx, wy;
                coords.NodeTileToWorld(bldX, bldY, wx, wy);
                Graphics::RenderCommandBuilder()
                    .WorldSprite(wx - r->pivotX, wy - r->pivotY,
                        (float)r->width, (float)r->height,
                        r->u0, r->v0, r->u1, r->v1,
                        SLOT_BUILDINGS_HIGHLIGHT, static_cast<WORD>(0.99f * 65535.0f))
                    .Color(D3DCOLOR_ARGB(80, 255, 255, 255))
                    .Layer(LAYER_FOREGROUND)
                    .Submit(renderQueue);
            }
        }
    }

    // ─── Work-site sprites (mine frameworks at resource nodes) ──────────
    {
        std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
        if (buildingsAtlas && m_economyManager) {
            LPDIRECT3DTEXTURE9 buildingsTex = buildingsAtlas->GetTexture();
            if (spriteRenderer && buildingsTex)
                spriteRenderer->SetTextureSlot(SLOT_BUILDINGS_HIGHLIGHT, buildingsTex);
            for (int i = 0; i < m_economyManager->GetBuildingCount(); ++i) {
                World::Building* b = m_economyManager->GetBuilding(i);
                if (!b) continue;
                Vector2i wsPos;
                const char* wsSpriteName = NULL;
                if (!b->GetWorkSiteRenderInfo(wsPos, wsSpriteName)) continue;
                if (!wsSpriteName || !*wsSpriteName) continue;
                uint32_t sprIdx = buildingsAtlas->GetIndex(wsSpriteName);
                if (sprIdx == 0xFFFFFFFF) {
                    std::string lowerName = wsSpriteName;
                    for (size_t ci = 0; ci < lowerName.size(); ++ci)
                        if (lowerName[ci] >= 'A' && lowerName[ci] <= 'Z')
                            lowerName[ci] = lowerName[ci] - 'A' + 'a';
                    sprIdx = buildingsAtlas->GetIndex(lowerName.c_str());
                }
                if (sprIdx == 0xFFFFFFFF) continue;
                const SpriteRegion* r = buildingsAtlas->GetRegion(sprIdx);
                if (!r) continue;
                float wx, wy;
                CoordinateSystem::GetInstance().NodeTileToWorld(wsPos.x, wsPos.y, wx, wy);
                Graphics::RenderCommandBuilder()
                    .WorldSprite(wx - r->pivotX, wy - r->pivotY,
                        (float)r->width, (float)r->height,
                        r->u0, r->v0, r->u1, r->v1,
                        SLOT_BUILDINGS_HIGHLIGHT, static_cast<WORD>(0.97f * 65535.0f))
                    .Layer(LAYER_EFFECTS)
                    .Submit(renderQueue);
            }
        }
    }

    // ─── Resource HUD bar at top of screen ──────────────────────────────
    if (m_resourceHudLoaded) {
        TextureRegistry& reg2 = TextureRegistry::instance();
        std::tr1::shared_ptr<SpriteAtlas> resIconAtlas = reg2.getAtlas("Icon");
        if (resIconAtlas) {
            float barX = 100.0f;
            float barY = 6.0f;
            float iconSize = 28.0f;
            float spacing = 60.0f;

            for (int i = 0; i < RESOURCE_HUD_COUNT; ++i) {
                if (m_resourceHud[i].iconIdx < 0) continue;

                const SpriteRegion* r = resIconAtlas->GetRegion(m_resourceHud[i].iconIdx);
                if (!r) continue;

                // Render icon
                Graphics::RenderCommandBuilder()
                    .UIElement(barX, barY,
                        iconSize, iconSize,
                        r->u0, r->v0, r->u1, r->v1,
                        SLOT_UI_MENU_ICON, 200)
                    .Layer(LAYER_FOREGROUND)
                    .Submit(renderQueue);

                // Render resource count (total stock across all storage)
                if (m_economyManager) {
                    char buf[32];
                    _snprintf(buf, sizeof(buf), "%d", m_economyManager->GetTotalStock(m_resourceHud[i].type));
                    float textX = barX + iconSize + 4.0f;
                    float textY = barY + (iconSize - 14.0f) * 0.5f;
                    m_textManager->DrawString(buf, textX, textY, 0xFFFFFFFF, 0.07f);
                }

                barX += spacing;
            }
        }
    }

    // ─── Geologist overlay: mountain highlight + survey UI ──────────
    RenderGeologistOverlay(renderQueue);

    // ─── Gamepad UI: push cursor & popups to render queue ──────────
    PushUiToQueue();

    // ─── Render notification banner (bunner_info) + status text ──────────
    if (m_textManager && !m_statusText.empty()) {
        float screenW = 1280.0f;
        float screenH = 720.0f;
        float textY = screenH - 40.0f;
        // Ensure UI atlas texture is set on the banner slot before submitting
        if (m_bannerLoaded && spriteRenderer) {
            TextureRegistry& regB = TextureRegistry::instance();
            std::tr1::shared_ptr<SpriteAtlas> uiAtlasB = regB.getAtlas("ui");
            if (uiAtlasB && uiAtlasB->GetTexture()) {
                spriteRenderer->SetTextureSlot(SLOT_UI_MENU_BG, uiAtlasB->GetTexture());
            }
        }
        // Banner sliding from right edge (top-left pivot), LAYER_EFFECTS so it
        // sorts before LAYER_UI text regardless of texture slot differences
        if (m_bannerLoaded && m_bannerSlideX < 1280.0f) {
            Graphics::RenderCommandBuilder()
                .UIElement(m_bannerSlideX, textY - m_bannerH,
                    m_bannerW, m_bannerH,
                    m_bannerU0, m_bannerV0, m_bannerU1, m_bannerV1,
                    SLOT_UI_MENU_BG, 0)
                .Layer(LAYER_EFFECTS)
                .Submit(renderQueue);
        }
        // Status text inside the banner
        float textX = m_bannerSlideX + 40.0f;
        m_textManager->DrawString(m_statusText, textX, textY - m_bannerH + 4.0f + 25.0f, 0xFFFFFFFF, 0.096f);
    }

    // ─── Logistics debug overlay ───────────────────────────────────────
    if (m_logisticsDebug && m_textManager) {
        CoordinateSystem& coords = CoordinateSystem::GetInstance();

        // Flags: resource amounts, ID, neighbor count
        if (m_flagManager) {
            char buf[64];
            for (size_t fi = 0; fi < m_flagManager->GetCount(); ++fi) {
                World::Flag* flag = m_flagManager->GetFlag(fi);
                if (!flag) continue;
                float wx, wy;
                coords.NodeTileToWorld(flag->pos.x, flag->pos.y, wx, wy);

                // Build resource summary string
                buf[0] = '\0';
                int avail = 0;
                for (int si = 0; si < 8; ++si) {
                    if (flag->slots[si].type != World::ResourceType_None && flag->slots[si].amount > 0) {
                        char tag[8];
                        switch (flag->slots[si].type) {
                            case World::ResourceType_Wood:   _snprintf(tag, sizeof(tag), "W:%d ", flag->slots[si].amount); break;
                            case World::ResourceType_Planks: _snprintf(tag, sizeof(tag), "P:%d ", flag->slots[si].amount); break;
                            case World::ResourceType_Stone:  _snprintf(tag, sizeof(tag), "S:%d ", flag->slots[si].amount); break;
                            case World::ResourceType_Fish:   _snprintf(tag, sizeof(tag), "F:%d ", flag->slots[si].amount); break;
                            case World::ResourceType_Meat:   _snprintf(tag, sizeof(tag), "M:%d ", flag->slots[si].amount); break;
                            case World::ResourceType_Coal:   _snprintf(tag, sizeof(tag), "C:%d ", flag->slots[si].amount); break;
                            default:
                                _snprintf(tag, sizeof(tag), "%s:%d ",
                                    World::ResourceTypeToString(flag->slots[si].type), flag->slots[si].amount);
                                break;
                        }
                        avail += _snprintf(buf + avail, sizeof(buf) - avail, "%s", tag);
                        if (avail >= (int)sizeof(buf) - 2) break;
                    }
                }
                _snprintf(buf + avail, sizeof(buf) - avail, "~%d", flag->id);

                float ty = wy + 12.0f;
                if (flag->hasBuilding) ty += 20.0f;
                m_textManager->DrawString(buf, wx - 30.0f, ty, D3DCOLOR_ARGB(220, 255, 255, 200), 0.06f, FONT_DEBUG, FONT_STYLE_NORMAL, 0.05f, LAYER_EFFECTS);
            }
        }

        // Carriers: cargo (per-segment walking)
        if (m_carrierManager) {
            char buf[64];
            for (int ci = 0; ci < m_carrierManager->GetCarrierCount(); ++ci) {
                World::Carrier* carrier = m_carrierManager->GetCarrier(ci);
                if (!carrier) continue;

                const Vector2i* pathTiles = NULL;
                int pathCount = 0;
                float ep = 0.0f;

                if (World::IsTransitState(carrier->state)) {
                    if (carrier->transitCount < 2) continue;
                    pathTiles = carrier->transitTiles;
                    pathCount = (int)carrier->transitCount;
                    ep = carrier->transitProgress;
                } else {
                    if (!carrier->road || carrier->road->tileCount < 2) continue;
                    pathTiles = carrier->road->tiles;
                    pathCount = (int)carrier->road->tileCount;
                    ep = carrier->ep;
                }

                int pathLen = pathCount - 1;
                if (ep < 0.0f) ep = 0.0f;
                if (ep > (float)pathLen) ep = (float)pathLen;
                int idx = (int)ep;
                float frac = ep - (float)idx;
                if (idx >= pathLen) { idx = pathLen - 1; frac = 1.0f; }
                if (idx < 0) { idx = 0; frac = 0.0f; }

                const Vector2i& tileA = pathTiles[idx];
                const Vector2i& tileB = pathTiles[idx + 1];

                float cx, cy, nx, ny;
                coords.NodeTileToWorld(tileA.x, tileA.y, cx, cy);
                coords.NodeTileToWorld(tileB.x, tileB.y, nx, ny);
                float wx = cx + (nx - cx) * frac;
                float wy = cy + (ny - cy) * frac;

                const char* cargoName = "Idle";
                if (carrier->m_cargo) {
                    cargoName = World::ResourceTypeToString(carrier->m_cargo->type);
                }

                if (carrier->road) {
                    _snprintf(buf, sizeof(buf), "%s %u<->%u", cargoName,
                        carrier->m_roadEndpointA ? carrier->m_roadEndpointA->id : 0,
                        carrier->m_roadEndpointB ? carrier->m_roadEndpointB->id : 0);
                } else {
                    _snprintf(buf, sizeof(buf), "%s (transit)", cargoName);
                }
                m_textManager->DrawString(buf, wx - 20.0f, wy - 20.0f, D3DCOLOR_ARGB(220, 200, 255, 200), 0.05f, FONT_DEBUG, FONT_STYLE_NORMAL, 0.05f, LAYER_EFFECTS);
            }
        }

        // Legend
        m_textManager->DrawTextToScreen("LOGISTICS DEBUG ON (Back=toggle)", 10.0f, 10.0f, D3DCOLOR_ARGB(180, 255, 255, 255), 0.08f);
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

        // Match EditorScene: use sprite atlas pivot so cursor center aligns with node position
        Graphics::RenderCommandBuilder()
            .WorldSprite(worldX - cursorRegion->pivotX, worldY - cursorRegion->pivotY,
                (float)cursorRegion->width, (float)cursorRegion->height,
                cursorRegion->u0, cursorRegion->v0, cursorRegion->u1, cursorRegion->v1,
                SLOT_UI_CURSOR, static_cast<WORD>(0.99f * 65535.0f))
            .Layer(LAYER_FOREGROUND)
            .Submit(renderQueue);
    }

    void GameScene::InitBuildMenu()
    {
        if (!m_buildMenu) {
            OutputDebugStringA("[GameScene] WARNING: m_buildMenu is null\n");
            return;
        }

        TextureRegistry& reg = TextureRegistry::instance();
        reg.getTextureOrLoad("ui");
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

        // Load icons from separate Icon atlas (ib_ prefixed building icons)
        reg.getTextureOrLoad("Icon");
        std::tr1::shared_ptr<SpriteAtlas> iconAtlas = reg.getAtlas("Icon");
        if (!iconAtlas) {
            OutputDebugStringA("[GameScene] WARNING: Icon atlas not found\n");
            return;
        }

        LPDIRECT3DTEXTURE9 iconTex = iconAtlas->GetTexture();
        if (!iconTex) {
            OutputDebugStringA("[GameScene] WARNING: Icon atlas has no texture\n");
            return;
        }

        // Register texture slots for the build menu
        SpriteRenderer* sr = m_renderer ? m_renderer->GetSpriteRenderer() : NULL;
        if (sr) {
            sr->SetTextureSlot(SLOT_UI_MENU_BG, uiTex);
            sr->SetTextureSlot(SLOT_UI_MENU_CELL, uiTex);
            sr->SetTextureSlot(SLOT_UI_MENU_ICON, iconTex);
        }

        m_buildMenu->SetTextureSlots(SLOT_UI_MENU_BG, SLOT_UI_MENU_CELL, SLOT_UI_MENU_ICON);
        m_buildMenu->SetTextures(uiTex, uiTex, iconTex);

        GridMenu::TileUV bgUV = {0,0,1,1}, cellUV = {0,0,1,1};
        uint32_t bgIdx = uiAtlas->GetIndex("menu_Grid");
        if (bgIdx != 0xFFFFFFFF) {
            const SpriteRegion* r = uiAtlas->GetRegion(bgIdx);
            if (r) { bgUV.u0 = r->u0; bgUV.v0 = r->v0; bgUV.u1 = r->u1; bgUV.v1 = r->v1; }
        } else {
            OutputDebugStringA("[GameScene] WARNING: 'menu_Grid' NOT FOUND in UI atlas\n");
        }
        uint32_t cellIdx = uiAtlas->GetIndex("menu_cell1");
        if (cellIdx != 0xFFFFFFFF) {
            const SpriteRegion* r = uiAtlas->GetRegion(cellIdx);
            if (r) { cellUV.u0 = r->u0; cellUV.v0 = r->v0; cellUV.u1 = r->u1; cellUV.v1 = r->v1; }
        } else {
            OutputDebugStringA("[GameScene] WARNING: 'menu_cell1' NOT FOUND in UI atlas\n");
        }
        m_buildMenu->SetBackgroundUV(bgUV);
        m_buildMenu->SetCellUV(cellUV);

        // Load icon_building group from Icon atlas
        std::vector<GridMenu::TileUV> tileUVs;
        std::vector<int> spriteIndices;
        std::vector<std::string> cellLabels;

        const std::vector<uint32_t>* groupIndices = iconAtlas->GetGroup("icon_building");
        if (groupIndices && !groupIndices->empty()) {
            for (size_t gi = 0; gi < groupIndices->size(); ++gi) {
                uint32_t spriteIdx = (*groupIndices)[gi];
                const SpriteRegion* reg = iconAtlas->GetRegion(spriteIdx);
                if (!reg) continue;
                GridMenu::TileUV uv;
                uv.u0 = reg->u0; uv.v0 = reg->v0;
                uv.u1 = reg->u1; uv.v1 = reg->v1;
                tileUVs.push_back(uv);
                spriteIndices.push_back((int)spriteIdx);
                // Use part after "ib_" as label, or full name
                std::string label = reg->name;
                if (label.compare(0, 3, "ib_") == 0) label = label.substr(3);
                cellLabels.push_back(label);
            }
        }

        {
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[GameScene] Found %d sprites in icon_building group\n", (int)tileUVs.size());
            OutputDebugStringA(dbg);
        }

        if (tileUVs.empty()) {
            OutputDebugStringA("[GameScene] icon_building group empty, skipping menu setup\n");
            return;
        }

        m_buildMenu->SetCellLabels(cellLabels);
        m_buildMenu->SetCellSpacing(80.0f, 80.0f);
        m_buildMenu->SetCellPadding(4.0f);
        m_buildMenu->SetCellVisualSize(64.0f, 64.0f);

        m_buildMenu->SetTileData(tileUVs, spriteIndices);

        // Initialize resource HUD icons (r_* sprites from Icon atlas)
        {
            m_resourceHud[0].type = World::ResourceType_Wood;      m_resourceHud[0].iconName = "r_wood";
            m_resourceHud[1].type = World::ResourceType_Stone;     m_resourceHud[1].iconName = "r_stone";
            m_resourceHud[2].type = World::ResourceType_Planks;    m_resourceHud[2].iconName = "r_planks";
            m_resourceHud[3].type = World::ResourceType_Fish;      m_resourceHud[3].iconName = "r_fish";
            m_resourceHud[4].type = World::ResourceType_Meat;      m_resourceHud[4].iconName = "r_meat";
            m_resourceHud[5].type = World::ResourceType_Bread;     m_resourceHud[5].iconName = "r_bread";
            m_resourceHud[6].type = World::ResourceType_Coal;      m_resourceHud[6].iconName = "r_coal";
            m_resourceHud[7].type = World::ResourceType_IronOre;   m_resourceHud[7].iconName = "r_ironore";
            m_resourceHud[8].type = World::ResourceType_GoldOre;   m_resourceHud[8].iconName = "r_goldore";
            m_resourceHud[9].type = World::ResourceType_IronBar;   m_resourceHud[9].iconName = "r_ironbar";
            m_resourceHud[10].type = World::ResourceType_GoldBar;  m_resourceHud[10].iconName = "r_goldbar";

            for (int i = 0; i < RESOURCE_HUD_COUNT; ++i) {
                if (m_resourceHud[i].iconName) {
                    uint32_t idx = iconAtlas->GetIndex(m_resourceHud[i].iconName);
                    m_resourceHud[i].iconIdx = (idx != 0xFFFFFFFF) ? (int)idx : -1;
                }
            }
            m_resourceHudLoaded = true;
            OutputDebugStringA("[GameScene] Resource HUD initialized\n");
        }

        OutputDebugStringA("[GameScene::InitBuildMenu] DONE\n");
    }

    void GameScene::InitRoadMenu()
    {
        if (!m_roadMenu) {
            OutputDebugStringA("[GameScene] WARNING: m_roadMenu is null\n");
            return;
        }

        TextureRegistry& reg = TextureRegistry::instance();
        reg.getTextureOrLoad("ui");
        std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
        if (!uiAtlas) {
            OutputDebugStringA("[GameScene] WARNING: UI atlas not found for road menu\n");
            return;
        }

        LPDIRECT3DTEXTURE9 uiTex = uiAtlas->GetTexture();
        if (!uiTex) {
            OutputDebugStringA("[GameScene] WARNING: UI atlas has no texture\n");
            return;
        }

        SpriteRenderer* sr = m_renderer ? m_renderer->GetSpriteRenderer() : NULL;
        if (sr) {
            sr->SetTextureSlot(SLOT_UI_ROAD_BG, uiTex);
            sr->SetTextureSlot(SLOT_UI_ROAD_CELL, uiTex);
            sr->SetTextureSlot(SLOT_UI_ROAD_ICON, uiTex);
        }

        m_roadMenu->SetTextureSlots(SLOT_UI_ROAD_BG, SLOT_UI_ROAD_CELL, SLOT_UI_ROAD_ICON);
        // No cell background texture → GridMenu skips cell background rendering
        m_roadMenu->SetBackgroundTexture(uiTex);
        m_roadMenu->SetAtlasTexture(uiTex);

        GridMenu::TileUV bgUV = {0,0,1,1};
        // Use menu_creat_flag_road as background at native size
        uint32_t newBgIdx = uiAtlas->GetIndex("menu_creat_flag_road");
        if (newBgIdx != 0xFFFFFFFF) {
            const SpriteRegion* r = uiAtlas->GetRegion(newBgIdx);
            if (r) {
                bgUV.u0 = r->u0; bgUV.v0 = r->v0;
                bgUV.u1 = r->u1; bgUV.v1 = r->v1;
                m_roadMenu->SetMenuSize((float)r->width, (float)r->height);
            }
        } else {
            uint32_t bgIdx = uiAtlas->GetIndex("menu_Grid");
            if (bgIdx != 0xFFFFFFFF) {
                const SpriteRegion* r = uiAtlas->GetRegion(bgIdx);
                if (r) { bgUV.u0 = r->u0; bgUV.v0 = r->v0; bgUV.u1 = r->u1; bgUV.v1 = r->v1; }
            }
        }
        m_roadMenu->SetBackgroundUV(bgUV);

        // Road/flag icons (3 items): set flag, delete flag, switch to build menu
        const char* iconNames[] = {
            "icon_set_flag",
            "icon_delete_flag",
            "icon_Streets",
        };
        const char* iconLabels[] = {
            "Set Flag",
            "Delete Flag",
            "Buildings",
        };
        std::vector<GridMenu::TileUV> tileUVs;
        std::vector<int> spriteIndices;
        std::vector<std::string> cellLabels;
        int iconH = 32;
        for (int i = 0; i < 3; ++i) {
            uint32_t idx = uiAtlas->GetIndex(iconNames[i]);
            if (idx == 0xFFFFFFFF) {
                char dbg[256];
                _snprintf(dbg, sizeof(dbg), "[GameScene] WARNING: '%s' NOT FOUND in UI atlas\n", iconNames[i]);
                OutputDebugStringA(dbg);
                continue;
            }
            const SpriteRegion* r = uiAtlas->GetRegion(idx);
            if (!r) continue;
            GridMenu::TileUV uv;
            uv.u0 = r->u0; uv.v0 = r->v0;
            uv.u1 = r->u1; uv.v1 = r->v1;
            tileUVs.push_back(uv);
            spriteIndices.push_back((int)idx);
            cellLabels.push_back(iconLabels[i]);
            if ((int)r->height > iconH) iconH = (int)r->height;
        }

        m_roadMenu->SetCellLabels(cellLabels);
        m_roadMenu->SetCellSpacing(110.0f, 60.0f);
        m_roadMenu->SetCellPadding(4.0f);
        m_roadMenu->SetCellVisualSize((float)iconH, (float)iconH);
        m_roadMenu->SetTileData(tileUVs, spriteIndices);

        OutputDebugStringA("[GameScene::InitRoadMenu] DONE\n");
    }

    void GameScene::InitFlagMenu()
    {
        if (!m_flagMenu) {
            OutputDebugStringA("[GameScene] WARNING: m_flagMenu is null\n");
            return;
        }

        TextureRegistry& reg = TextureRegistry::instance();
        reg.getTextureOrLoad("ui");
        std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
        if (!uiAtlas) {
            OutputDebugStringA("[GameScene] WARNING: UI atlas not found for flag menu\n");
            return;
        }

        LPDIRECT3DTEXTURE9 uiTex = uiAtlas->GetTexture();
        if (!uiTex) {
            OutputDebugStringA("[GameScene] WARNING: UI atlas has no texture\n");
            return;
        }

        const float scale = 0.5f;

        // Background: menu_creat_flag_road scaled proportionally
        UIMenu::BackgroundData bg = {0,0,1,1, 0,0,0,0};
        uint32_t bgIdx = uiAtlas->GetIndex("menu_creat_flag_road");
        if (bgIdx != 0xFFFFFFFF) {
            const SpriteRegion* r = uiAtlas->GetRegion(bgIdx);
            if (r) {
                bg.u0 = r->u0; bg.v0 = r->v0;
                bg.u1 = r->u1; bg.v1 = r->v1;
                bg.w = (float)r->width * scale;
                bg.h = (float)r->height * scale;
                bg.x = 640.0f - bg.w * 0.5f;
                bg.y = 360.0f - bg.h * 0.5f;
            }
        }
        m_flagMenu->SetBackground(bg);
        m_flagMenu->SetAtlas(uiTex, SLOT_UI_ROAD_BG);

        // Three items: Set Flag, Delete Flag, Build Road
        const char* iconNames[] = {"icon_set_flag", "icon_delete_flag", "icon_Streets"};
        const char* iconLabels[] = {"Set Flag", "Delete Flag", "Build Road"};

        m_flagMenuItemCount = 3;
        float menuCX = 640.0f;
        float menuY = bg.y + bg.h * 0.5f;
        float itemSpacing = bg.w / (float)(m_flagMenuItemCount + 1);
        int iconSize = 32;

        for (int i = 0; i < m_flagMenuItemCount; ++i) {
            uint32_t idx = uiAtlas->GetIndex(iconNames[i]);
            UIMenu::ItemData& item = m_flagMenuItemData[i];
            if (idx != 0xFFFFFFFF) {
                const SpriteRegion* r = uiAtlas->GetRegion(idx);
                if (r) {
                    item.u0 = r->u0; item.v0 = r->v0;
                    item.u1 = r->u1; item.v1 = r->v1;
                    item.w = (float)r->width * scale;
                    item.h = (float)r->height * scale;
                }
            }
            if (item.w < 1.0f) { item.w = (float)iconSize; item.h = (float)iconSize; }
            item.x = menuCX + (float)(i - 1) * itemSpacing - item.w * 0.5f;
            item.y = menuY - item.h * 0.5f;
            item.label = iconLabels[i];
        }
        m_flagMenu->SetItems(m_flagMenuItemData, m_flagMenuItemCount);

        OutputDebugStringA("[GameScene::InitFlagMenu] DONE\n");
    }

    void GameScene::InitGeologistMenu()
    {
        if (!m_geologistMenu) {
            OutputDebugStringA("[GameScene] WARNING: m_geologistMenu is null\n");
            return;
        }

        TextureRegistry& reg = TextureRegistry::instance();
        reg.getTextureOrLoad("ui");
        std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
        if (!uiAtlas) {
            OutputDebugStringA("[GameScene] WARNING: UI atlas not found for geologist menu\n");
            return;
        }

        LPDIRECT3DTEXTURE9 uiTex = uiAtlas->GetTexture();
        if (!uiTex) {
            OutputDebugStringA("[GameScene] WARNING: UI atlas has no texture\n");
            return;
        }

        // Background: geologist_menu sprite, fallback to menu_creat_flag_road
        UIMenu::BackgroundData bg = {0,0,1,1, 0,0,0,0};
        bool bgFound = false;
        const char* bgNames[] = { "geologist_menu", "menu_creat_flag_road" };
        for (int bi = 0; bi < 2 && !bgFound; ++bi) {
            uint32_t bgIdx = uiAtlas->GetIndex(bgNames[bi]);
            if (bgIdx != 0xFFFFFFFF) {
                const SpriteRegion* r = uiAtlas->GetRegion(bgIdx);
                if (r) {
                    bg.u0 = r->u0; bg.v0 = r->v0;
                    bg.u1 = r->u1; bg.v1 = r->v1;
                    bg.w = (float)r->width;
                    bg.h = (float)r->height;
                    bg.x = 640.0f - bg.w * 0.5f;
                    bg.y = 360.0f - bg.h * 0.5f;
                    bgFound = true;
                    OutputDebugStringA("[GameScene::InitGeologistMenu] using background: ");
                    OutputDebugStringA(bgNames[bi]);
                    OutputDebugStringA("\n");
                }
            }
        }
        if (bgFound) {
            m_geologistMenu->SetBackground(bg);
        }
        m_geologistMenu->SetAtlas(uiTex, SLOT_UI_MENU_BG);

        OutputDebugStringA("[GameScene::InitGeologistMenu] DONE\n");
    }

    // Hex grid parity: on odd Y, the hex neighbor at dy>0 has X one less
    // than on even Y. Adjust entrance offset accordingly so the flag
    // always lands on the correct SE hex neighbor of the building.
    void AdjustEntranceForParity(bool buildingEvenY, int& entranceX, int entranceY)
    {
        if (!buildingEvenY && entranceY != 0 && entranceX > 0) {
            entranceX = entranceX - 1;
        }
    }

    void GameScene::ClearRoadTilesForFlag(World::Flag* flag)
    {
        if (!m_map || !flag) return;
        World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
        if (!roadsLayer) return;
        for (size_t i = 0; i < flag->roads.size(); ++i) {
            World::Road* road = flag->roads[i];
            if (!road) continue;
            for (uint32_t t = 0; t < road->tileCount; ++t) {
                int tx = road->tiles[t].x;
                int ty = road->tiles[t].y;
                if (tx < 0 || ty < 0 || tx >= roadsLayer->GetWidth() || ty >= roadsLayer->GetHeight())
                    continue;
                World::Tile& rt = roadsLayer->GetTile(tx, ty);
                if (rt.atlasName != "streets") continue;
                rt.atlasName = "";
                rt.regionIndex = -1;
                rt.walkable = false;
                UpdateRoadNeighbors(tx, ty);
            }
        }
    }

    void GameScene::GetEntranceOffset(const std::string& buildingName, int& outX, int& outY)
    {
        outX = 0; outY = 0;
        TextureRegistry& reg = TextureRegistry::instance();
        reg.getTextureOrLoad("Buildings");
        std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
        if (!buildingsAtlas) return;

        // Buildings atlas uses "b_" prefix + lowercase name (e.g. "b_woodcutter")
        uint32_t idx = buildingsAtlas->GetIndex(buildingName.c_str());
        if (idx == 0xFFFFFFFF && !buildingName.empty()) {
            std::string lowerName = buildingName;
            for (size_t ci = 0; ci < lowerName.size(); ++ci)
                if (lowerName[ci] >= 'A' && lowerName[ci] <= 'Z')
                    lowerName[ci] = lowerName[ci] - 'A' + 'a';
            idx = buildingsAtlas->GetIndex(lowerName.c_str());
        }
        if (idx == 0xFFFFFFFF && !buildingName.empty()) {
            std::string prefixed = std::string("b_") + buildingName;
            idx = buildingsAtlas->GetIndex(prefixed.c_str());
        }
        if (idx == 0xFFFFFFFF && !buildingName.empty() && buildingName.compare(0, 2, "b_") != 0) {
            std::string prefixedLower = std::string("b_") + buildingName;
            for (size_t ci = 0; ci < prefixedLower.size(); ++ci)
                if (prefixedLower[ci] >= 'A' && prefixedLower[ci] <= 'Z')
                    prefixedLower[ci] = prefixedLower[ci] - 'A' + 'a';
            idx = buildingsAtlas->GetIndex(prefixedLower.c_str());
        }
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
        if (!m_map) { OutputDebugStringA("[CanPlaceBuilding] FAIL: no map\n"); return false; }

        // Get building collision footprint from atlas
        const char* spriteNamePtr = GetBuildingSpriteName(type);
        std::string spriteName = spriteNamePtr ? spriteNamePtr : "";

        int footOffX = 0, footOffY = 0;
        int footW = 1, footH = 1;
        std::vector<std::pair<int,int>> footMask;
        {
            TextureRegistry& reg = TextureRegistry::instance();
            reg.getTextureOrLoad("Buildings");
            std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
            if (buildingsAtlas) {
                uint32_t idx = buildingsAtlas->GetIndex(spriteName.c_str());
                if (idx != 0xFFFFFFFF) {
                    const SpriteRegion* r = buildingsAtlas->GetRegion(idx);
                    if (r) {
                        footOffX = r->collOffX;
                        footOffY = r->collOffY;
                        footW = (int)r->collWidth;
                        footH = (int)r->collHeight;
                        footMask = r->collMask;
                    }
                }
            }
        }

        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        int nodesW = coords.GetNodesWidth();
        int nodesH = coords.GetNodesHeight();

        // Helper lambda to check a single tile
        auto checkTile = [&](int tx, int ty) -> bool {
            if (tx < 0 || tx >= nodesW || ty < 0 || ty >= nodesH) return false;

            World::TileLayer* buildingsLayer = m_map->GetLayer(World::Buildings);
            if (buildingsLayer) {
                const World::Tile& bt = buildingsLayer->GetTile(tx, ty);
                if (bt.type != World::Tile_None) {
                    char dbg[256];
                    _snprintf(dbg, sizeof(dbg), "[CanPlaceBuilding] FAIL: occupied at (%d,%d)\n", tx, ty);
                    OutputDebugStringA(dbg);
                    return false;
                }
            }

            // Roads allowed under building footprint — flag on road is valid,
            // roads are on a separate layer and coexist with buildings.

            World::TileLayer* objectsLayer = m_map->GetLayer(World::Objects);
            if (objectsLayer) {
                const World::Tile& ot = objectsLayer->GetTile(tx, ty);
                if (ot.u1 > ot.u0 && ot.v1 > ot.v0) {
                    OutputDebugStringA("[CanPlaceBuilding] FAIL: object present\n");
                    return false;
                }
            }

            if (m_flagManager && m_flagManager->GetFlagAt(tx, ty)) {
                OutputDebugStringA("[CanPlaceBuilding] FAIL: flag at footprint\n");
                return false;
            }

            return true;
        };

        // Hardcoded override: ensure 2x2 buildings use correct footprint
        {
            bool is2x2 = (type == World::Stonemason || type == World::Sawmill || type == World::Farm || type == World::Mill);
            if (is2x2 && (footW != 2 || footH != 2)) {
                footW = 2;
                footH = 2;
                char warn[256];
                _snprintf(warn, sizeof(warn), "[CanPlaceBuilding] WARNING: %s atlas footprint != 2x2, forcing 2x2\n",
                    GetBuildingSpriteName(type));
                OutputDebugStringA(warn);
            }
        }

        int anchorX = buildX + footOffX;
        int anchorY = buildY + footOffY;

        if (!footMask.empty()) {
            for (size_t i = 0; i < footMask.size(); ++i) {
                int tx = anchorX + footMask[i].first;
                int ty = anchorY + footMask[i].second;
                if (!checkTile(tx, ty)) return false;
            }
        } else {
            for (int dy = 0; dy < footH; ++dy) {
                for (int dx = 0; dx < footW; ++dx) {
                    int tx = anchorX + dx;
                    int ty = anchorY + dy;
                    if (!checkTile(tx, ty)) return false;
                }
            }
        }

        // For mines, require the resource node directly on the anchor tile
        World::ResourceType requiredRes = GetResourceTypeForMine(type);
        if (requiredRes != World::ResourceType_None) {
            if (anchorX < 0 || anchorX >= nodesW || anchorY < 0 || anchorY >= nodesH) return false;
            const World::ResourceNode& node = m_map->GetResourceNode(anchorX, anchorY);
            if (node.type != requiredRes) {
                char dbg[256];
                _snprintf(dbg, sizeof(dbg), "[CanPlaceBuilding] FAIL: no %s node at anchor (%d,%d)\n",
                    World::ResourceTypeToString(requiredRes), anchorX, anchorY);
                OutputDebugStringA(dbg);
                return false;
            }
        }

        return true;
    }

    void GameScene::PlaceFlag(int tileX, int tileY)
    {
        if (!m_flagManager || !m_map || !m_placementManager) return;
        if (m_selectedBuilding == World::Building_None) return;

        m_placementManager->SelectBuilding(m_selectedBuilding);
        PlacementData data = m_placementManager->GetPlacementData(tileX, tileY);
        if (!data.valid) {
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[GameScene] PlaceFlag: cannot place at (%d,%d): %s\n",
                tileX, tileY, data.errorMsg ? data.errorMsg : "unknown");
            OutputDebugStringA(dbg);
            m_placementManager->SelectBuilding(m_selectedBuilding);
            return;
        }

        World::Flag* flag = m_flagManager->CreateFlag(data.flagX, data.flagY);
        flag->type = World::FLAG_NORMAL;
        flag->pendingBuilding = m_selectedBuilding;
        flag->hasBuilding = true;

        SplitRoadAtFlag(flag);
        LinkFlagToRoadNetwork(flag);
        SyncCarriersForFlag(flag);
        CreateConstructionSite(flag, data.buildX, data.buildY);
        if (m_constructionManager) m_constructionManager->MarkBuilderRoutesDirty();

        m_buildState = BUILDSTATE_NONE;
        m_placementIconIdx = -1;
        m_placementConstrIdx = -1;
        m_selectedBuilding = World::Building_None;
        m_statusText = "Building construction started!";
        m_statusTextTimer = 2.0f;
        {
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[GameScene] PlaceFlag: placed type=%d at (%d,%d)\n",
                (int)flag->pendingBuilding, data.buildX, data.buildY);
            OutputDebugStringA(dbg);
        }
    }

    void GameScene::PlaceFreeFlag(int tileX, int tileY)
    {
        if (!m_flagManager || !m_map) return;

        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        int nodesW = coords.GetNodesWidth();
        int nodesH = coords.GetNodesHeight();
        if (tileX < 0 || tileX >= nodesW || tileY < 0 || tileY >= nodesH) return;

        BYTE weight = m_map->GetNodeWeight(tileX, tileY);
        if (weight == World::Weight_Deep || weight == World::Weight_Block) return;

        if (m_flagManager->GetFlagAt(tileX, tileY)) return;

        World::Flag* flag = m_flagManager->CreateFlag(tileX, tileY);
        flag->type = World::FLAG_NORMAL;
        flag->pendingBuilding = World::Building_None;
        flag->hasBuilding = false;

        // Split any road that passes through this flag position (BEFORE linking)
        SplitRoadAtFlag(flag);

        LinkFlagToRoadNetwork(flag);
        SyncCarriersForFlag(flag);
        if (m_constructionManager) m_constructionManager->MarkBuilderRoutesDirty();

        char dbg[256];
        _snprintf(dbg, sizeof(dbg), "[GameScene] Free flag placed at (%d,%d)\n", tileX, tileY);
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
            char dbg2[128];
            _snprintf(dbg2, sizeof(dbg2), "[GameScene] Sites count=%u\n", (unsigned)m_constructionManager->GetCount());
            OutputDebugStringA(dbg2);
        }

        // Get full collision footprint for the building type
        int footOffX = 0, footOffY = 0;
        int footW = 1, footH = 1;
        int buildingSpriteIdx = 0; // fallback sprite index
        std::vector<std::pair<int,int>> footMask;
        {
            const char* namePtr = GetBuildingSpriteName(flag->pendingBuilding);
            std::string spriteName = (namePtr && namePtr[0]) ? namePtr : "b_unknown";
            TextureRegistry& reg = TextureRegistry::instance();
            reg.getTextureOrLoad("Buildings");
            std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
            if (buildingsAtlas) {
                uint32_t idx = buildingsAtlas->GetIndex(spriteName.c_str());
                if (idx == 0xFFFFFFFF) {
                    std::string lowerName = spriteName;
                    for (size_t ci = 0; ci < lowerName.size(); ++ci)
                        if (lowerName[ci] >= 'A' && lowerName[ci] <= 'Z')
                            lowerName[ci] = lowerName[ci] - 'A' + 'a';
                    idx = buildingsAtlas->GetIndex(lowerName.c_str());
                }
                if (idx != 0xFFFFFFFF) {
                    buildingSpriteIdx = (int)idx;
                    const SpriteRegion* r = buildingsAtlas->GetRegion(idx);
                    if (r) {
                        footOffX = r->collOffX;
                        footOffY = r->collOffY;
                        footW = (int)r->collWidth;
                        footH = (int)r->collHeight;
                        footMask = r->collMask;
                    }
                }
            }
        }

        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        int nodesW = coords.GetNodesWidth();
        int nodesH = coords.GetNodesHeight();

        // Hardcoded override: ensure 2x2 buildings use correct footprint
        // (atlas collWidth/collHeight may be wrong for some building types)
        {
            World::BuildingType bt = flag->pendingBuilding;
            bool is2x2 = (bt == World::Stonemason || bt == World::Sawmill || bt == World::Farm || bt == World::Mill);
            if (is2x2 && (footW != 2 || footH != 2)) {
                footW = 2;
                footH = 2;
            }
        }

        // Mark all footprint tiles on the Buildings layer.
        // Only the base tile (dx=0, dy=0) renders the construction tent sprite;
        // the other tiles are marked occupied to block placement but stay invisible.
        auto markTile = [&](int tx, int ty, bool isBaseTile) {
            if (tx < 0 || tx >= nodesW || ty < 0 || ty >= nodesH) return;
            World::Tile& bTile = buildingsLayer->GetTile(tx, ty);
            if (bTile.type == World::Tile_None) {
                bTile.type = World::Decoration;
                bTile.walkable = true;
                bTile.buildingType = (int)flag->pendingBuilding;
                if (isBaseTile) {
                    bTile.atlasName = "Buildings";
                    bTile.regionIndex = buildingSpriteIdx;
                    bTile.u0 = CONSTRUCTION_U0; bTile.v0 = CONSTRUCTION_V0;
                    bTile.u1 = CONSTRUCTION_U1; bTile.v1 = CONSTRUCTION_V1;
                } else {
                    bTile.atlasName = "";
                    bTile.regionIndex = -1;
                    bTile.u0 = 0.0f; bTile.v0 = 0.0f;
                    bTile.u1 = 1.0f; bTile.v1 = 1.0f;
                }
            }
        };

        if (!footMask.empty()) {
            for (size_t i = 0; i < footMask.size(); ++i) {
                markTile(siteX + footOffX + footMask[i].first,
                         siteY + footOffY + footMask[i].second,
                         i == 0);
            }
        } else {
            for (int dy = 0; dy < footH; ++dy) {
                for (int dx = 0; dx < footW; ++dx) {
                    markTile(siteX + footOffX + dx, siteY + footOffY + dy,
                             dx == 0 && dy == 0);
                }
            }
        }

        _snprintf(dbg, sizeof(dbg), "[GameScene] ConstructionSite created at (%d,%d) type=%d wood=%d stone=%d\n",
            siteX, siteY, (int)flag->pendingBuilding, site->woodNeeded, site->stoneNeeded);
        OutputDebugStringA(dbg);
    }

    const char* GameScene::GetBuildingName(World::BuildingType type) const
    {
        return GetBuildingSpriteName(type);
    }

    const char* GameScene::GetBuildingSpriteName(World::BuildingType type) const
    {
        switch (type) {
            case World::Woodcutter:   return "b_woodcutter";
            case World::Forester:     return "b_forester";
            case World::Sawmill:      return "b_sawmill";
            case World::Stonemason:   return "b_mason";
            case World::CoalMine:
            case World::BronzeMine:   return "b_mine";
            case World::IronMine:     return "b_mine";
            case World::GoldMine:     return "b_mine";
            case World::IronSmelter:  return "b_ironsmelter";
            case World::GoldSmelter:  return "b_goldsmelter";
            case World::BronzeSmelter: return "b_bronzesmelter";
            case World::Farm:         return "b_farm";
            case World::Mill:         return "b_mill";
            case World::Bakery:       return "b_bakery";
            case World::Fisher:       return "b_fisher";
            case World::Hunter:       return "b_hunter";
            case World::ToolWorkshop: return "b_toolworkshop";
            case World::Storehouse:   return "b_warehouse";
            case World::Well:         return "b_well";
            case World::Barracks:     return "b_barracks";
            default:                  return "";
        }
    }

    World::ResourceType GameScene::GetResourceTypeForMine(World::BuildingType buildingType) const
    {
        switch (buildingType) {
            case World::CoalMine:   return World::ResourceType_Coal;
            case World::BronzeMine: return World::ResourceType_BronzeOre;
            case World::IronMine:   return World::ResourceType_IronOre;
            case World::GoldMine:   return World::ResourceType_GoldOre;
            default:                return World::ResourceType_None;
        }
    }

    bool GameScene::IsMineType(World::BuildingType type) const
    {
        return GetResourceTypeForMine(type) != World::ResourceType_None;
    }

    void GameScene::AssignOreDepositsToMountains()
    {
        if (!m_map) { OutputDebugStringA("[AssignOre] FAIL: no map\n"); return; }
        World::TileLayer* objLayer = m_map->GetLayer(World::Objects);
        if (!objLayer) { OutputDebugStringA("[AssignOre] FAIL: no Objects layer\n"); return; }

        int w = objLayer->GetWidth();
        int h = objLayer->GetHeight();
        int mountainCount = 0;
        int assigned = 0;

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const World::Tile& tile = objLayer->GetTile(x, y);
                if (tile.type == World::Mountain || tile.type == World::MountainOnWater) {
                    mountainCount++;
                } else {
                    continue;
                }

                World::ResourceNode& rn = m_map->GetResourceNode(x, y);
                if (rn.type != World::ResourceType_Stone && rn.type != World::ResourceType_None) continue;

                float roll = (float)rand() / RAND_MAX;
                World::ResourceType oreType;
                int amount;

                if (roll < 0.30f) {
                    oreType = World::ResourceType_Coal;
                    amount = 15 + rand() % 11;
                } else if (roll < 0.50f) {
                    oreType = World::ResourceType_IronOre;
                    amount = 10 + rand() % 11;
                } else if (roll < 0.70f) {
                    oreType = World::ResourceType_BronzeOre;
                    amount = 12 + rand() % 9;
                } else if (roll < 0.85f) {
                    oreType = World::ResourceType_GoldOre;
                    amount = 8 + rand() % 8;
                } else {
                    oreType = World::ResourceType_Stone;
                    amount = 15 + rand() % 16;
                }

                rn.type = oreType;
                rn.amount = amount;
                rn.isVisible = true;
                rn.surveyed = false;

                if (m_economyManager) {
                    m_economyManager->GetRegistry().RegisterWorldResource(oreType, x, y);
                }
                assigned++;
            }
        }

        {
            char dbg[512];
            int pos = _snprintf(dbg, sizeof(dbg), "[GameScene] Mountains=%d assigned=%d map=(%dx%d)", mountainCount, assigned, w, h);
            if (assigned > 0) {
                pos += _snprintf(dbg + pos, sizeof(dbg) - pos, "\n");
                for (int y = 0; y < h && assigned > 0; ++y) {
                    for (int x = 0; x < w; ++x) {
                        const World::Tile& tile = objLayer->GetTile(x, y);
                        if (tile.type != World::Mountain && tile.type != World::MountainOnWater) continue;
                        const World::ResourceNode& rn = m_map->GetResourceNode(x, y);
                        if (rn.type == World::ResourceType_None) continue;
                        pos += _snprintf(dbg + pos, sizeof(dbg) - pos, "  Mountain (%d,%d) -> %s (amount=%d)\n", x, y, World::ResourceTypeToString(rn.type), rn.amount);
                    }
                }
            }
            OutputDebugStringA(dbg);
        }
    }

    World::BuildingType GameScene::GetBuildingTypeFromSpriteName(const std::string& name) const
    {
        std::string key = name;
        if (key.compare(0, 2, "b_") == 0)
            key = key.substr(2);

        struct { const char* name; World::BuildingType type; } entries[] = {
            { "woodcutter",   World::Woodcutter },
            { "sawmill",      World::Sawmill },
            { "coalmine",     World::CoalMine },
            { "ironmine",     World::IronMine },
            { "goldmine",     World::GoldMine },
            { "ironsmelter",  World::IronSmelter },
            { "goldsmelter",  World::GoldSmelter },
            { "farm",         World::Farm },
            { "mill",         World::Mill },
            { "bakery",       World::Bakery },
            { "fisher",       World::Fisher },
            { "hunter",       World::Hunter },
            { "toolworkshop", World::ToolWorkshop },
            { "warehouse",    World::Storehouse },
            { "townhall",     World::Storehouse },
            { "bronzemine",   World::BronzeMine },
            { "bronzesmelter", World::BronzeSmelter },
        };

        for (int i = 0; i < sizeof(entries)/sizeof(entries[0]); ++i) {
            if (key == entries[i].name)
                return entries[i].type;
        }
        return World::Building_None;
    }

    void GameScene::RestoreBuildingsFromLayer()
    {
        if (!m_map || !m_flagManager || !m_economyManager) return;

        World::TileLayer* buildingsLayer = m_map->GetLayer(World::Buildings);
        if (!buildingsLayer) return;

        TextureRegistry& reg = TextureRegistry::instance();
        std::tr1::shared_ptr<SpriteAtlas> atlas = reg.getAtlas("Buildings");
        if (!atlas) return;

        int w = buildingsLayer->GetWidth();
        int h = buildingsLayer->GetHeight();

        int restored = 0;
        int skipped = 0;

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const World::Tile& tile = buildingsLayer->GetTile(x, y);
                if (tile.atlasName != "Buildings") continue;
                if (tile.regionIndex < 0) continue;

                const SpriteRegion* region = atlas->GetRegion(tile.regionIndex);
                if (!region) continue;

                // Skip construction sites (check name AND UV mismatch)
                const std::string& spriteName = region->name;
                bool isConstruction = (spriteName.find("construction") != std::string::npos ||
                    spriteName.find("Construction") != std::string::npos);
                // Also detect by UV mismatch: if tile UVs don't match the atlas region,
                // the UVs were overridden (e.g. for construction sites)
                if (isConstruction)
                { skipped++; continue; }

                // Determine building type: from serialized tile, sprite name, or resource node
                bool isBuildingSprite = region->isBuilding;
                World::BuildingType type = World::Building_None;
                if (tile.buildingType >= 0) {
                    type = static_cast<World::BuildingType>(tile.buildingType);
                } else {
                    type = GetBuildingTypeFromSpriteName(spriteName);
                    if (type == World::Building_None) {
                        // Check for mine sprite — determine type from resource node (old saves)
                        std::string key = spriteName;
                        if (key.compare(0, 2, "b_") == 0) key = key.substr(2);
                        if (key == "mine" && m_map) {
                            const World::ResourceNode& rn = m_map->GetResourceNode(x, y);
                            switch (rn.type) {
                                case World::ResourceType_Coal:      type = World::CoalMine; break;
                                case World::ResourceType_IronOre:   type = World::IronMine; break;
                                case World::ResourceType_GoldOre:   type = World::GoldMine; break;
                                case World::ResourceType_BronzeOre: type = World::BronzeMine; break;
                                case World::ResourceType_Stone:     type = World::Stonemason; break;
                                default:
                                    type = World::CoalMine;
                                    { char buf[256]; _snprintf(buf, sizeof(buf), "[Restore] Unknown mine node at (%d,%d), defaulting to CoalMine\n", x, y); OutputDebugStringA(buf); }
                                    break;
                            }
                        }
                    }
                }
                if (type == World::Building_None && !isBuildingSprite)
                { skipped++; continue; }
                if (type == World::Building_None) continue; // decorative only

                // Entrance position (flag goes here)
                int entranceX = region->entranceX;
                int entranceY = region->entranceY;

                // For town hall / warehouse sprites that have entranceX=0, entranceY=0,
                // use the standard entrance offset (0, 2) — the flag node below the building
                bool isWarehouseType = (type == World::Storehouse);
                if (isWarehouseType && entranceX == 0 && entranceY == 0) {
                    entranceX = 0;
                    entranceY = 2;
                }

                if (entranceX == 0 && entranceY == 0) { skipped++; continue; }

                {
                    bool buildingEvenY = (y % 2 == 0);
                    AdjustEntranceForParity(buildingEvenY, entranceX, entranceY);
                }
                int flagX = x + entranceX;
                int flagY = y + entranceY;

                // Find existing flag at entrance position
                World::Flag* flag = m_flagManager->GetFlagAt(flagX, flagY);
                if (!flag) {
                    flag = m_flagManager->CreateFlag(flagX, flagY);
                    flag->type = World::FLAG_BUILDING;
                }

                // Skip if this flag already has a building linked
                if (flag->building) { skipped++; continue; }

                // Only create one warehouse — skip if already restored
                if (isWarehouseType && m_economyManager->GetWarehouse()) { skipped++; continue; }

                World::Building* building = NULL;

                if (isWarehouseType) {
                    // Create a Warehouse for Storehouse-type sprites
                    World::Warehouse* wh = new World::Warehouse(x, y, 0);
                    wh->state = World::State_Finished;
                    wh->connectedFlag = flag;
                    wh->map = m_map;
                    flag->building = wh;
                    flag->hasBuilding = true;
                    flag->pendingBuilding = World::Building_None;
                    flag->type = World::FLAG_WAREHOUSE;
                    building = wh;

                    if (m_storehouseManager) {
                        wh->SetStorehouseManager(m_storehouseManager);
                    }

                    // Seed with starting resources
                    wh->AddResource(World::ResourceType_Wood, 500);
                    wh->AddResource(World::ResourceType_Stone, 500);
                    wh->AddResource(World::ResourceType_Planks, 200);
                    wh->AddResource(World::ResourceType_Fish, 100);
                    wh->AddResource(World::ResourceType_Meat, 100);
                    wh->AddResource(World::ResourceType_Coal, 100);

                    m_economyManager->SetWarehouse(wh);
                    if (m_transportJobManager) {
                        m_transportJobManager->SetWarehouse(wh);
                    }
                    if (m_constructionManager) {
                        m_constructionManager->SetWarehouseFlag(flag);
                    }
                    if (m_carrierManager) {
                        m_carrierManager->SetWarehouseFlag(flag);
                    }
                    // Set warehouse demand for all resource types (restore path)
                    if (m_demandManager && flag) {
                        World::ResourceType allTypes[] = {
                            World::ResourceType_Wood, World::ResourceType_Stone, World::ResourceType_Planks,
                            World::ResourceType_Fish, World::ResourceType_Meat, World::ResourceType_Coal,
                            World::ResourceType_BronzeBar
                        };
                        for (int ri = 0; ri < sizeof(allTypes)/sizeof(allTypes[0]); ++ri) {
                            m_demandManager->SetDemand(allTypes[ri], 9999, flag->handle, 10);
                        }
                    }
                } else {
                    building = World::CreateBuilding(type, x, y, 0, m_map);
                    if (!building) { skipped++; continue; }
                    building->state = World::State_Finished;
                    building->connectedFlag = flag;
                    building->map = m_map;
                    flag->building = building;
                    flag->hasBuilding = true;
                    flag->pendingBuilding = World::Building_None;

                    // Set footprint from atlas region for ForceDeleteBuilding safety
                    {
                        const SpriteRegion* r2 = atlas->GetRegion(tile.regionIndex);
                        if (r2) {
                            building->m_footprintX = r2->collOffX;
                            building->m_footprintY = r2->collOffY;
                            building->m_footprintW = (int)r2->collWidth;
                            building->m_footprintH = (int)r2->collHeight;
                            if (building->m_footprintW < 1) building->m_footprintW = 1;
                            if (building->m_footprintH < 1) building->m_footprintH = 1;
                            // Hardcoded override: ensure 2x2 buildings use correct footprint
                            bool is2x2 = (type == World::Stonemason || type == World::Sawmill || type == World::Farm || type == World::Mill);
                            if (is2x2 && (building->m_footprintW != 2 || building->m_footprintH != 2)) {
                                building->m_footprintW = 2;
                                building->m_footprintH = 2;
                            }
                        }
                    }

                    // Restored buildings already have workers
                    if (building->m_maxPopulation > 0) {
                        building->m_population = 1;
                        char dbg[256];
                        _snprintf(dbg, sizeof(dbg),
                            "[Restore] Worker assigned: type=%d pop=%d maxPop=%d at (%d,%d)\n",
                            building->type, building->m_population,
                            building->m_maxPopulation, x, y);
                        OutputDebugStringA(dbg);
                    }
                }

                m_economyManager->AddBuilding(building);
                LinkFlagToRoadNetwork(flag);
                SyncCarriersForFlag(flag);

                char dbg[256];
                _snprintf(dbg, sizeof(dbg), "[Restore] %s at (%d,%d) flag=(%d,%d)%s\n",
                    spriteName.c_str(), x, y, flagX, flagY,
                    isWarehouseType ? " (WAREHOUSE)" : "");
                OutputDebugStringA(dbg);
                restored++;
            }
        }

        char dbg[256];
        _snprintf(dbg, sizeof(dbg), "[Restore] Buildings restored=%d skipped=%d\n", restored, skipped);
        OutputDebugStringA(dbg);
    }

    void GameScene::ConfirmConstruction(World::Flag* flag)
    {
        if (!flag || !m_map) return;

        World::ConstructionSite* site = m_constructionManager ? m_constructionManager->GetSiteForFlag(flag) : NULL;
        if (!site) return;

        // Footprint dimensions — set from atlas below, used for building creation
        int footOffX = 0, footOffY = 0, footW = 1, footH = 1;

        World::TileLayer* buildingsLayer = m_map->GetLayer(World::Buildings);
        if (buildingsLayer) {
            const char* buildingName = GetBuildingSpriteName(site->buildingType);

            TextureRegistry& reg = TextureRegistry::instance();
            reg.getTextureOrLoad("Buildings");
            std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");

            if (buildingsAtlas && buildingName[0] != '\0') {
                // Try the exact name first, then lowercase fallback
                uint32_t spriteIdx = buildingsAtlas->GetIndex(buildingName);
                if (spriteIdx == 0xFFFFFFFF) {
                    std::string lowerName = buildingName;
                    for (size_t ci = 0; ci < lowerName.size(); ++ci)
                        if (lowerName[ci] >= 'A' && lowerName[ci] <= 'Z')
                            lowerName[ci] = lowerName[ci] - 'A' + 'a';
                    spriteIdx = buildingsAtlas->GetIndex(lowerName.c_str());
                }
                // Fallback: try common building sprites if the specific one is missing
                if (spriteIdx == 0xFFFFFFFF) {
                    const char* fallbacks[] = { "b_warehouse", "b_residence", "b_well", "b_mason" };
                    for (int fi = 0; fi < 4 && spriteIdx == 0xFFFFFFFF; ++fi)
                        spriteIdx = buildingsAtlas->GetIndex(fallbacks[fi]);
                }
                if (spriteIdx != 0xFFFFFFFF) {
                    const SpriteRegion* r = buildingsAtlas->GetRegion(spriteIdx);
                    if (r) {
                        // Get footprint dimensions from the atlas region
                        footOffX = r->collOffX;
                        footOffY = r->collOffY;
                        footW = (int)r->collWidth;
                        footH = (int)r->collHeight;
                        if (footW < 1) footW = 1;
                        if (footH < 1) footH = 1;
                        // Hardcoded override: ensure 2x2 buildings use correct footprint
                        {
                            bool is2x2 = (site->buildingType == World::Stonemason || site->buildingType == World::Sawmill || site->buildingType == World::Farm || site->buildingType == World::Mill);
                            if (is2x2 && (footW != 2 || footH != 2)) {
                                footW = 2;
                                footH = 2;
                            }
                        }
                        CoordinateSystem& coords = CoordinateSystem::GetInstance();
                        int nodesW = coords.GetNodesWidth();
                        int nodesH = coords.GetNodesHeight();
                        // Anchor tile: show the finished building sprite
                        {
                            int ax = site->x + footOffX;
                            int ay = site->y + footOffY;
                            if (ax >= 0 && ax < nodesW && ay >= 0 && ay < nodesH) {
                                World::Tile& tile = buildingsLayer->GetTile(ax, ay);
                                tile.atlasName = "Buildings";
                                tile.type = World::Decoration;
                                tile.regionIndex = (int)spriteIdx;
                                tile.u0 = r->u0; tile.v0 = r->v0;
                                tile.u1 = r->u1; tile.v1 = r->v1;
                                tile.walkable = true;
                            }
                        }
                        // Non-anchor footprint tiles: clear construction tents
                        for (int dy = 0; dy < footH; ++dy) {
                            for (int dx = 0; dx < footW; ++dx) {
                                if (dx == 0 && dy == 0) continue;
                                int tx = site->x + footOffX + dx;
                                int ty = site->y + footOffY + dy;
                                if (tx >= 0 && tx < nodesW && ty >= 0 && ty < nodesH) {
                                    World::Tile& tile = buildingsLayer->GetTile(tx, ty);
                                    tile.atlasName = "";
                                    tile.type = World::Tile_None;
                                    tile.regionIndex = -1;
                                    tile.walkable = false;
                                    tile.buildingType = -1;
                                }
                            }
                        }
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
            // Store footprint on the Building object for ForceDeleteBuilding tile cleanup
            building->m_footprintX = footOffX;
            building->m_footprintY = footOffY;
            building->m_footprintW = footW;
            building->m_footprintH = footH;
            building->connectedFlag = flag;
            flag->building = building;
            flag->hasBuilding = true;
            flag->pendingBuilding = World::Building_None;

            // Look up depleted sprite index for mines
            const char* depletedSpriteName = NULL;
            switch (site->buildingType) {
                case World::Stonemason: depletedSpriteName = "mine_ruin_stone_marble"; break;
                case World::CoalMine:
                case World::BronzeMine:
                case World::IronMine:
                case World::GoldMine: depletedSpriteName = "mine_ruin"; break;
            }
            if (depletedSpriteName) {
                TextureRegistry& reg2 = TextureRegistry::instance();
                reg2.getTextureOrLoad("Buildings");
                std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas2 = reg2.getAtlas("Buildings");
                if (buildingsAtlas2) {
                    uint32_t depletedIdx = buildingsAtlas2->GetIndex(depletedSpriteName);
                    if (depletedIdx != 0xFFFFFFFF)
                        building->m_depletedSpriteIdx = (int)depletedIdx;
                }
            }

            if (m_economyManager) {
                m_economyManager->AddBuilding(building);
            }

            // Spawn worker walking from warehouse to this building
            if (m_workerManager && m_economyManager && m_economyManager->GetWarehouse()
                && m_economyManager->GetWarehouse()->connectedFlag
                && building->m_maxPopulation > 0)
            {
                World::Flag* whFlag = m_economyManager->GetWarehouse()->connectedFlag;
                m_workerManager->SpawnWorker(building, (float)whFlag->pos.x, (float)whFlag->pos.y);
            }
        }

        m_statusText = "Building completed!";
        m_statusTextTimer = 2.0f;

//        OutputDebugStringA("[GameScene] ConfirmConstruction - building completed\n");
    }

    void GameScene::ClearBuildingFootprint(int startX, int startY, int width, int height)
    {
        if (!m_map) return;
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        int nodesW = coords.GetNodesWidth();
        int nodesH = coords.GetNodesHeight();
        World::TileLayer* buildingsLayer = m_map->GetLayer(World::Buildings);
        if (!buildingsLayer) return;
        for (int dy = 0; dy < height; ++dy) {
            for (int dx = 0; dx < width; ++dx) {
                int tx = startX + dx;
                int ty = startY + dy;
                if (tx < 0 || tx >= nodesW || ty < 0 || ty >= nodesH) continue;
                World::Tile& t = buildingsLayer->GetTile(tx, ty);
                t.atlasName = "";
                t.type = World::Tile_None;
                t.regionIndex = -1;
                t.walkable = false;
                t.buildingType = -1;
            }
        }
    }

    void GameScene::ConfirmDeleteFlag(int tileX, int tileY)
    {
        if (!m_flagManager || !m_map) return;
        World::Flag* flag = m_flagManager->GetFlagAt(tileX, tileY);
        if (!flag) return;

        char dbg[256];
        _snprintf(dbg, sizeof(dbg), "[GameScene] ConfirmDeleteFlag at (%d,%d) flag type=%d\n",
            tileX, tileY, (int)flag->type);
        OutputDebugStringA(dbg);

        // Safety check — cannot delete town hall flag
        if (flag->type == World::FLAG_WAREHOUSE) {
            m_statusText = "Cannot delete town hall flag!";
            m_statusTextTimer = 2.0f;
            return;
        }

        // Clear building/construction footprint on Buildings layer
        if (flag->building || flag->pendingBuilding != World::Building_None) {
            const char* buildingName = NULL;
            int buildX = flag->pos.x;
            int buildY = flag->pos.y;

            if (flag->building) {
                buildingName = GetBuildingName(flag->building->type);
                buildX = flag->building->pos.x;
                buildY = flag->building->pos.y;
            } else {
                buildingName = GetBuildingName(flag->pendingBuilding);
            }

            // For completed buildings, use stored position; for construction sites compute from entrance offset
            if (flag->building) {
                buildX = flag->building->pos.x;
                buildY = flag->building->pos.y;
            } else {
                std::string nameStr = buildingName ? buildingName : "";
                if (nameStr.empty()) {
                    buildX = flag->pos.x;
                    buildY = flag->pos.y;
                } else {
                    if (nameStr.compare(0, 3, "ib_") == 0)
                        nameStr = nameStr.substr(3);
                    int entranceX = 0, entranceY = 0;
                    GetEntranceOffset(nameStr, entranceX, entranceY);
                    buildY = flag->pos.y - entranceY;
                    {
                        bool buildingEvenY = (buildY % 2 == 0);
                        AdjustEntranceForParity(buildingEvenY, entranceX, entranceY);
                    }
                    buildX = flag->pos.x - entranceX;
                }
            }

            // Clear footprint tiles
            World::TileLayer* buildingsLayer = m_map->GetLayer(World::Buildings);
            if (buildingsLayer) {
                TextureRegistry& reg = TextureRegistry::instance();
                reg.getTextureOrLoad("Buildings");
                std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
                if (buildingsAtlas && buildingName && buildingName[0]) {
                    std::string spriteName = buildingName;
                    if (spriteName.compare(0, 2, "b_") != 0)
                        spriteName = std::string("b_") + spriteName;
                    uint32_t idx = buildingsAtlas->GetIndex(spriteName.c_str());
                    if (idx == 0xFFFFFFFF) {
                        std::string lowerName = spriteName;
                        for (size_t ci = 0; ci < lowerName.size(); ++ci)
                            if (lowerName[ci] >= 'A' && lowerName[ci] <= 'Z')
                                lowerName[ci] = lowerName[ci] - 'A' + 'a';
                        idx = buildingsAtlas->GetIndex(lowerName.c_str());
                    }
                    if (idx != 0xFFFFFFFF) {
                        const SpriteRegion* r = buildingsAtlas->GetRegion(idx);
                        if (r) {
                            int footOffX = r->collOffX;
                            int footOffY = r->collOffY;
                            int footW = (int)r->collWidth;
                            int footH = (int)r->collHeight;
                            // Hardcoded override: ensure 2x2 buildings use correct footprint
                            World::BuildingType dtype = (flag->building) ? flag->building->type : flag->pendingBuilding;
                            bool is2x2 = (dtype == World::Stonemason || dtype == World::Sawmill || dtype == World::Farm || dtype == World::Mill);
                            if (is2x2 && (footW != 2 || footH != 2)) {
                                footW = 2;
                                footH = 2;
                            }
                            ClearBuildingFootprint(buildX + footOffX, buildY + footOffY, footW, footH);
                        }
                    }
                }
            }

            // Remove the building from EconomyManager and delete the object
            if (flag->building) {
                m_objectLifecycleManager->ForceDeleteBuilding(flag->building, m_map);
                flag->building = NULL;
                flag->hasBuilding = false;
            }

            // Remove construction site if present
            if (flag->pendingBuilding != World::Building_None) {
                if (m_constructionManager) {
                    m_constructionManager->RemoveSiteAt(buildX, buildY);
                }
                flag->pendingBuilding = World::Building_None;
            }
        }

        // Remove visual road tiles, then the flag via lifecycle manager
        ClearRoadTilesForFlag(flag);
        m_objectLifecycleManager->ForceDeleteFlag(flag);

        m_statusText = "Building and flag deleted!";
        m_statusTextTimer = 2.0f;

        _snprintf(dbg, sizeof(dbg), "[GameScene] ConfirmDeleteFlag done at (%d,%d)\n", tileX, tileY);
        OutputDebugStringA(dbg);
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
        m_roadValidNeighbors.clear();
        m_roadAutoPath.clear();
        m_statusText = "ROAD: A=add tile  B=cancel";
        OutputDebugStringA("[GameScene] Road building started (tile-by-tile)\n");
    }

    void GameScene::UpdateRoadPreview(int cursorX, int cursorY)
    {
        if (m_buildState != BUILDSTATE_PLACE_ROAD) return;
        if (!m_map || m_roadPreviewPath.empty()) return;

        int lastX = m_roadPreviewPath.back().first;
        int lastY = m_roadPreviewPath.back().second;

        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        int nodesW = coords.GetNodesWidth();
        int nodesH = coords.GetNodesHeight();
        World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
        World::TileLayer* objectsLayer = m_map->GetLayer(World::Objects);
        World::TileLayer* buildingsLayer = m_map->GetLayer(World::Buildings);
        World::TileLayer* placementLayer = m_map->GetLayer(World::Placement);

        // ── Compute 4 valid neighbors matching CalcPatternAt ──
        bool evenRow = (lastY % 2 == 0);
        int nx[4], ny[4];
        if (evenRow) {
            nx[0] = lastX + 1; ny[0] = lastY - 1; // 1: (x+1, y-1)
            nx[1] = lastX + 1; ny[1] = lastY + 1; // 2: (x+1, y+1)
            nx[2] = lastX;     ny[2] = lastY + 1; // 4: (x, y+1)
            nx[3] = lastX;     ny[3] = lastY - 1; // 8: (x, y-1)
        } else {
            nx[0] = lastX;     ny[0] = lastY - 1; // 1: (x, y-1)
            nx[1] = lastX;     ny[1] = lastY + 1; // 2: (x, y+1)
            nx[2] = lastX - 1; ny[2] = lastY + 1; // 4: (x-1, y+1)
            nx[3] = lastX - 1; ny[3] = lastY - 1; // 8: (x-1, y-1)
        }

        m_roadValidNeighbors.clear();
        for (int i = 0; i < 4; ++i) {
            int tx = nx[i], ty = ny[i];
            if (tx < 0 || tx >= nodesW || ty < 0 || ty >= nodesH) continue;

            bool alreadyPlaced = false;
            for (size_t j = 0; j < m_roadPreviewPath.size(); ++j) {
                if (m_roadPreviewPath[j].first == tx && m_roadPreviewPath[j].second == ty) {
                    alreadyPlaced = true; break;
                }
            }
            if (alreadyPlaced) {
                char dbg[128];
                _snprintf(dbg, sizeof(dbg), "[Road] Dir %d: (%d,%d) already in path\n", i, tx, ty);
                OutputDebugStringA(dbg);
                continue;
            }

            BYTE w = m_map->GetNodeWeight(tx, ty);
            if (w == World::Weight_Deep || w == World::Weight_Block) {
                char dbg[128];
                _snprintf(dbg, sizeof(dbg), "[Road] Dir %d: (%d,%d) weight=%d (blocked)\n", i, tx, ty, (int)w);
                OutputDebugStringA(dbg);
                continue;
            }

            bool hasFlag = m_flagManager && m_flagManager->GetFlagAt(tx, ty) != NULL;

            if (objectsLayer && !hasFlag) {
                const World::Tile& ot = objectsLayer->GetTile(tx, ty);
                if (ot.u1 > ot.u0 && ot.v1 > ot.v0) {
                    char dbg[128];
                    _snprintf(dbg, sizeof(dbg), "[Road] Dir %d: (%d,%d) blocked by object\n", i, tx, ty);
                    OutputDebugStringA(dbg);
                    continue;
                }
            }
            if (buildingsLayer && !hasFlag) {
                const World::Tile& bt = buildingsLayer->GetTile(tx, ty);
                if (bt.regionIndex >= 0) {
                    char dbg[256];
                    _snprintf(dbg, sizeof(dbg), "[Road] Dir %d: (%d,%d) blocked by building regionIdx=%d atlas='%s'\n", i, tx, ty, bt.regionIndex, bt.atlasName.c_str());
                    OutputDebugStringA(dbg);
                    continue;
                }
            }
            if (placementLayer && !hasFlag) {
                const World::Tile& pt = placementLayer->GetTile(tx, ty);
                if (pt.regionIndex >= 0 && pt.atlasName != "streets") {
                    char dbg[128];
                    _snprintf(dbg, sizeof(dbg), "[Road] Dir %d: (%d,%d) blocked by placement '%s'\n", i, tx, ty, pt.atlasName.c_str());
                    OutputDebugStringA(dbg);
                    continue;
                }
            }

            char dbg[128];
            _snprintf(dbg, sizeof(dbg), "[Road] Dir %d: (%d,%d) VALID\n", i, tx, ty);
            OutputDebugStringA(dbg);
            m_roadValidNeighbors.push_back(std::make_pair(tx, ty));
        }

        // ── Additionally, if cursor is on a flag, compute A* auto-path ──
        m_roadAutoPath.clear();
        bool cursorOnFlag = m_flagManager && m_flagManager->GetFlagAt(cursorX, cursorY) != NULL;
        bool cursorSameAsLast = (cursorX == lastX && cursorY == lastY);
        if (cursorOnFlag && !cursorSameAsLast)
        {
            struct RoadPassable {
                World::Map* map;
                World::FlagManager* flagManager;
                World::TileLayer* roadsLayer;
                World::TileLayer* objectsLayer;
                World::TileLayer* buildingsLayer;
                World::TileLayer* placementLayer;
                RoadPassable(World::Map* m, World::FlagManager* fm,
                    World::TileLayer* rl, World::TileLayer* ol,
                    World::TileLayer* bl, World::TileLayer* pl)
                    : map(m), flagManager(fm), roadsLayer(rl), objectsLayer(ol),
                      buildingsLayer(bl), placementLayer(pl) {}
                bool operator()(int x, int y) {
                    BYTE w = map->GetNodeWeight(x, y);
                    if (w == World::Weight_Deep || w == World::Weight_Block) return false;
                    bool hasFlag = flagManager && flagManager->GetFlagAt(x, y) != NULL;
                    if (hasFlag) return true;
                    if (roadsLayer) {
                        const World::Tile& rt = roadsLayer->GetTile(x, y);
                        if (rt.regionIndex >= 0) return true;
                    }
                    if (objectsLayer) {
                        const World::Tile& ot = objectsLayer->GetTile(x, y);
                        if (ot.u1 > ot.u0 && ot.v1 > ot.v0) return false;
                    }
                    if (buildingsLayer) {
                        const World::Tile& bt = buildingsLayer->GetTile(x, y);
                        if (bt.regionIndex >= 0) return false;
                    }
                    if (placementLayer) {
                        const World::Tile& pt = placementLayer->GetTile(x, y);
                        if (pt.regionIndex >= 0 && pt.atlasName != "streets") return false;
                    }
                    return true;
                }
            };
            struct RoadCost {
                float operator()(int, int) { return 1.0f; }
            };
            Logic::IsoNeighbors isoNeighbors;
            Logic::AStar::FindPath(
                lastX, lastY, cursorX, cursorY,
                nodesW, nodesH,
                RoadPassable(m_map, m_flagManager, roadsLayer, objectsLayer, buildingsLayer, placementLayer),
                RoadCost(),
                isoNeighbors,
                m_roadAutoPath
            );
            if (!m_roadAutoPath.empty() && m_roadAutoPath[0].first == lastX && m_roadAutoPath[0].second == lastY) {
                m_roadAutoPath.erase(m_roadAutoPath.begin());
            }
        }
    }

    void GameScene::TryAddRoadTile(int x, int y)
    {
        if (m_buildState != BUILDSTATE_PLACE_ROAD || m_roadPreviewPath.empty()) return;

        int lastX = m_roadPreviewPath.back().first;
        int lastY = m_roadPreviewPath.back().second;

        // Case 1: cursor on the last tile → place flag and finish
        if (x == lastX && y == lastY) {
            CommitRoad();
            return;
        }

        // Case 2: auto-path to a flag
        if (!m_roadAutoPath.empty()) {
            // Append all auto-path tiles (they lead to the destination flag)
            for (size_t i = 0; i < m_roadAutoPath.size(); ++i) {
                m_roadPreviewPath.push_back(m_roadAutoPath[i]);
            }
            m_statusText = "ROAD: auto-path built!";
            CommitRoad();
            return;
        }

        // Check if (x,y) is a valid neighbor (tile-by-tile mode)
        bool isValid = false;
        int dirIndex = -1;
        for (size_t i = 0; i < m_roadValidNeighbors.size(); ++i) {
            if (m_roadValidNeighbors[i].first == x && m_roadValidNeighbors[i].second == y) {
                isValid = true; dirIndex = (int)i; break;
            }
        }
        if (!isValid) {
            m_statusText = "Cannot build here!";
            m_statusTextTimer = 1.5f;
            return;
        }

        // Determine which hex direction this is from the last tile
        int dx = x - lastX;
        int dy = y - lastY;
        bool evenRow = (lastY % 2 == 0);
        int hexDir = -1; // 0=W,1=E,2=NW,3=NE,4=SW,5=SE
        if      (dx == -1 && dy ==  0) hexDir = 0; // W
        else if (dx ==  1 && dy ==  0) hexDir = 1; // E
        else if (evenRow) {
            if      (dx == -1 && dy == -1) hexDir = 2; // NW
            else if (dx ==  0 && dy == -1) hexDir = 3; // NE
            else if (dx == -1 && dy ==  1) hexDir = 4; // SW
            else if (dx ==  0 && dy ==  1) hexDir = 5; // SE
        } else {
            if      (dx ==  0 && dy == -1) hexDir = 2; // NW
            else if (dx ==  1 && dy == -1) hexDir = 3; // NE
            else if (dx ==  0 && dy ==  1) hexDir = 4; // SW
            else if (dx ==  1 && dy ==  1) hexDir = 5; // SE
        }

        m_roadPreviewPath.push_back(std::make_pair(x, y));

        {
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[Road] Added tile (%d,%d) dir=%d hexDir=%d pathLen=%u  last->(%d,%d)\n",
                x, y, dirIndex, hexDir, (unsigned)m_roadPreviewPath.size(), lastX, lastY);
            OutputDebugStringA(dbg);
        }

        // Neighbor has an existing flag → commit road
        if (m_flagManager && m_flagManager->GetFlagAt(x, y)) {
            CommitRoad();
            return;
        }

        // Neighbor has an existing road (no flag) → commit (connect to it)
        if (m_map) {
            World::TileLayer* rl = m_map->GetLayer(World::Roads);
            if (rl && rl->GetTile(x, y).regionIndex >= 0) {
                char dbg[128];
                _snprintf(dbg, sizeof(dbg), "[Road] Tile (%d,%d) has existing road — committing\n", x, y);
                OutputDebugStringA(dbg);
                CommitRoad();
                return;
            }
        }

        // Empty tile → continue building
        m_statusText = "ROAD: A=add tile  B=cancel";
        char dbg[128];
        _snprintf(dbg, sizeof(dbg), "[Road] Tile added: (%d,%d) path=%u cells\n",
            x, y, (unsigned)m_roadPreviewPath.size());
        OutputDebugStringA(dbg);
    }

    void GameScene::CommitRoad()
    {
        if (m_buildState != BUILDSTATE_PLACE_ROAD) return;
        if (!m_map) return;

        World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
        if (!roadsLayer) return;

        // Reject if any intermediate tile (not start, not end) already has a road
        for (size_t i = 1; i + 1 < m_roadPreviewPath.size(); ++i) {
            int px = m_roadPreviewPath[i].first;
            int py = m_roadPreviewPath[i].second;
            const World::Tile& rt = roadsLayer->GetTile(px, py);
            if (rt.regionIndex >= 0) {
                m_statusText = "Cannot build through existing road!";
                m_statusTextTimer = 3.0f;
                CancelRoad();
                return;
            }
        }

        World::TileLayer* placementLayer = m_map->GetLayer(World::Placement);

        for (size_t i = 0; i < m_roadPreviewPath.size(); ++i) {
            int px = m_roadPreviewPath[i].first;
            int py = m_roadPreviewPath[i].second;
            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            int nodesW = coords.GetNodesWidth();
            int nodesH = coords.GetNodesHeight();
            if (px < 0 || px >= nodesW || py < 0 || py >= nodesH) continue;

            World::TileLayer* objectsLayer = m_map->GetLayer(World::Objects);
            bool hasObject = false;
            if (objectsLayer) {
                const World::Tile& ot = objectsLayer->GetTile(px, py);
                if (ot.u1 > ot.u0 && ot.v1 > ot.v0) hasObject = true;
            }
            if (hasObject) {
                if (!m_flagManager || !m_flagManager->GetFlagAt(px, py)) continue;
            }
            if (placementLayer) {
                const World::Tile& pt = placementLayer->GetTile(px, py);
                if (pt.regionIndex >= 0 && pt.atlasName != "streets") continue;
            }

            World::Tile& tile = roadsLayer->GetTile(px, py);
            if (tile.regionIndex < 0) {
                tile.type = World::Decoration;
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

            World::TileLayer* objectsLayer2 = m_map->GetLayer(World::Objects);
            bool hasObject = false;
            if (objectsLayer2) {
                const World::Tile& ot = objectsLayer2->GetTile(px, py);
                if (ot.u1 > ot.u0 && ot.v1 > ot.v0) hasObject = true;
            }
            if (hasObject) {
                if (!m_flagManager || !m_flagManager->GetFlagAt(px, py)) continue;
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
                // If end position sits on an existing road, split it (no-op if already split)
                SplitRoadAtFlag(endFlag);
            }

            // Create Road object linking start and end flags
            World::Flag* startFlag = m_flagManager ? m_flagManager->GetFlagAt(m_roadStartX, m_roadStartY) : NULL;
            {
                char dbg[256];
                _snprintf(dbg, sizeof(dbg), "[RoadDebug] CommitRoad: start=(%d,%d) end=(%d,%d) startFlag=%p endFlag=%p\n",
                    m_roadStartX, m_roadStartY, endX, endY, startFlag, endFlag);
                OutputDebugStringA(dbg);
                if (!startFlag) {
                    _snprintf(dbg, sizeof(dbg), "[RoadDebug] NO START FLAG AT (%d,%d) — searching adjacent...\n",
                        m_roadStartX, m_roadStartY);
                    OutputDebugStringA(dbg);
                    bool evenRow = (m_roadStartY % 2 == 0);
                    int sx[6], sy[6];
                    if (evenRow) {
                        int eSX[] = {m_roadStartX-1, m_roadStartX+1, m_roadStartX-1, m_roadStartX, m_roadStartX-1, m_roadStartX};
                        int eSY[] = {m_roadStartY, m_roadStartY, m_roadStartY-1, m_roadStartY-1, m_roadStartY+1, m_roadStartY+1};
                        memcpy(sx, eSX, sizeof(sx)); memcpy(sy, eSY, sizeof(sy));
                    } else {
                        int oSX[] = {m_roadStartX-1, m_roadStartX+1, m_roadStartX, m_roadStartX+1, m_roadStartX, m_roadStartX+1};
                        int oSY[] = {m_roadStartY, m_roadStartY, m_roadStartY-1, m_roadStartY-1, m_roadStartY+1, m_roadStartY+1};
                        memcpy(sx, oSX, sizeof(sx)); memcpy(sy, oSY, sizeof(sy));
                    }
                    for (int di = 0; di < 6; ++di) {
                        World::Flag* adj = m_flagManager ? m_flagManager->GetFlagAt(sx[di], sy[di]) : NULL;
                        if (adj) {
                            _snprintf(dbg, sizeof(dbg), "[RoadDebug] Found adjacent start flag at (%d,%d) id=%u\n",
                                sx[di], sy[di], adj->id);
                            OutputDebugStringA(dbg);
                            startFlag = adj;
                            break;
                        }
                    }
                }
            }
            if (startFlag && endFlag && startFlag != endFlag && m_roadManager) {
                // Build tile path from preview
                std::vector<Vector2i> tilePath;
                for (size_t pi = 0; pi < m_roadPreviewPath.size(); ++pi) {
                    Vector2i v;
                    v.x = m_roadPreviewPath[pi].first;
                    v.y = m_roadPreviewPath[pi].second;
                    tilePath.push_back(v);
                }
                // Create the Road (registers with both flags)
                World::Road* road = m_roadManager->CreateRoad(startFlag, endFlag, tilePath);
                if (road) {
                    char dbg[256];
                    _snprintf(dbg, sizeof(dbg), "[GameScene] Road %u created: (%d,%d) <-> (%d,%d) tiles=%u\n",
                        road->id, m_roadStartX, m_roadStartY, endX, endY, (unsigned)road->tileCount);
                    OutputDebugStringA(dbg);
                }

                SyncCarriersForFlag(startFlag);
                SyncCarriersForFlag(endFlag);
            }
        }

        m_statusText = "Road built!";
        m_statusTextTimer = 2.0f;
        CancelRoad();
    }

void GameScene::CancelRoad()
{
    m_buildState = BUILDSTATE_NONE;
    m_roadStartX = -1;
    m_roadStartY = -1;
    m_roadPreviewPath.clear();
    m_roadValidNeighbors.clear();
    m_roadAutoPath.clear();
}

    static std::vector<Vector2i> FindRoadTilePath(
        World::Map* map,
        int startX, int startY, int endX, int endY)
    {
        std::vector<Vector2i> result;
        if (!map) return result;
        World::TileLayer* roadsLayer = map->GetLayer(World::Roads);
        if (!roadsLayer) return result;

        int rw = roadsLayer->GetWidth();
        int rh = roadsLayer->GetHeight();
        if (startX < 0 || startX >= rw || startY < 0 || startY >= rh) return result;
        if (endX < 0 || endX >= rw || endY < 0 || endY >= rh) return result;

        std::vector<int> parent(rw * rh, -1);
        std::queue<std::pair<int,int>> q;
        q.push(std::make_pair(startX, startY));
        parent[startY * rw + startX] = -2; // mark visited

        while (!q.empty()) {
            int cx = q.front().first;
            int cy = q.front().second;
            q.pop();

            if (cx == endX && cy == endY) {
                // Reconstruct path
                int px = endX, py = endY;
                while (px != startX || py != startY) {
                    Vector2i v; v.x = px; v.y = py;
                    result.push_back(v);
                    int p = parent[py * rw + px];
                    px = p & 0xFFFF;
                    py = (p >> 16) & 0xFFFF;
                    if (px == startX && py == startY) break;
                }
                Vector2i sv; sv.x = startX; sv.y = startY;
                result.push_back(sv);
                std::reverse(result.begin(), result.end());
                return result;
            }

            bool evenRow = (cy % 2 == 0);
            int nx[6], ny[6];
            if (evenRow) {
                int eNX[] = {cx-1, cx+1, cx-1, cx, cx-1, cx};
                int eNY[] = {cy, cy, cy-1, cy-1, cy+1, cy+1};
                memcpy(nx, eNX, sizeof(nx)); memcpy(ny, eNY, sizeof(ny));
            } else {
                int oNX[] = {cx-1, cx+1, cx, cx+1, cx, cx+1};
                int oNY[] = {cy, cy, cy-1, cy-1, cy+1, cy+1};
                memcpy(nx, oNX, sizeof(nx)); memcpy(ny, oNY, sizeof(ny));
            }
            for (int di = 0; di < 6; ++di) {
                int tx = nx[di], ty = ny[di];
                if (tx < 0 || tx >= rw || ty < 0 || ty >= rh) continue;
                if (parent[ty * rw + tx] != -1) continue;
                const World::Tile& rt = roadsLayer->GetTile(tx, ty);
                if (rt.atlasName != "streets") continue;
                parent[ty * rw + tx] = cx | (cy << 16);
                q.push(std::make_pair(tx, ty));
            }
        }
        return result; // path not found
    }

    void GameScene::SplitRoadAtFlag(World::Flag* flag)
    {
        if (!flag || !m_roadManager || !m_carrierManager) return;

        for (size_t i = 0; i < m_roadManager->GetCount(); ++i) {
            World::Road* road = m_roadManager->GetRoad(i);
            if (!road) continue;

            // Check if flag sits on this road (but NOT at either endpoint)
            int splitIdx = -1;
            for (uint32_t t = 1; t + 1 < road->tileCount; ++t) {
                if (road->tiles[t].x == flag->pos.x && road->tiles[t].y == flag->pos.y) {
                    splitIdx = (int)t;
                    break;
                }
            }
            if (splitIdx < 0) continue;

            World::Flag* ra = m_flagManager ? m_flagManager->ResolveFlag(road->a) : NULL;
            World::Flag* rb = m_flagManager ? m_flagManager->ResolveFlag(road->b) : NULL;
            char dbg[256];
            _snprintf(dbg, sizeof(dbg),
                "[RoadSplit] Flag %u(%d,%d) splits road %u: A(%d,%d) -> X -> B(%d,%d) at tile[%d]\n",
                flag->id, flag->pos.x, flag->pos.y,
                road->id,
                ra ? ra->pos.x : -1, ra ? ra->pos.y : -1,
                rb ? rb->pos.x : -1, rb ? rb->pos.y : -1,
                splitIdx);
            OutputDebugStringA(dbg);

            // Save endpoints
            World::Flag* a = ra;
            World::Flag* b = rb;

            if (!a || !b) {
                _snprintf(dbg, sizeof(dbg),
                    "[RoadSplit] Road %u has unresolvable endpoint, skipping split\n", road->id);
                OutputDebugStringA(dbg);
                continue;
            }

            // Split is not deletion — force-remove the old road (carrier+job cleaned up)
            // and recreate as two segments; carriers sync onto the new roads.
            // Build two tile paths: A->X and X->B
            std::vector<Vector2i> pathAX, pathXB;
            for (uint32_t t = 0; t <= (uint32_t)splitIdx; ++t)
                pathAX.push_back(road->tiles[t]);
            for (uint32_t t = (uint32_t)splitIdx; t < road->tileCount; ++t)
                pathXB.push_back(road->tiles[t]);

            // Remove old road and its carrier via lifecycle manager
            m_objectLifecycleManager->ForceDeleteRoad(road);

            // Create two new roads (guarded by GetRoadBetween — safe if already exist)
            World::Road* ax = m_roadManager->CreateRoad(a, flag, pathAX);
            World::Road* xb = m_roadManager->CreateRoad(flag, b, pathXB);

            // Sync carriers for new segments only
            if (ax) m_carrierManager->SyncCarriersForRoad(ax);
            if (xb) m_carrierManager->SyncCarriersForRoad(xb);

            _snprintf(dbg, sizeof(dbg),
                "[RoadSplit] Done: road %u (%d,%d)<->(%d,%d) and %u (%d,%d)<->(%d,%d)\n",
                ax ? ax->id : 0, a->pos.x, a->pos.y, flag->pos.x, flag->pos.y,
                xb ? xb->id : 0, flag->pos.x, flag->pos.y, b->pos.x, b->pos.y);
            OutputDebugStringA(dbg);

            return; // only one road can pass through a given position
        }
    }

    void GameScene::SyncCarriersForFlag(World::Flag* flag)
    {
        if (!flag || !m_carrierManager || !m_roadManager) return;

        // Carriers only spawn on roads connected to the warehouse/town hall
        World::Flag* wh = m_constructionManager ? m_constructionManager->GetWarehouseFlag() : NULL;
        bool connected = false;
        if (wh && flag == wh) {
            connected = true;
        } else if (wh) {
            connected = (m_roadManager->FindFlagPath(wh, flag).size() >= 2);
        }
        // If no warehouse exists yet, no carriers are created (safe — early in init)

        char dbg[256];
        _snprintf(dbg, sizeof(dbg), "[SyncCarriers] Flag %u (%d,%d) roads=%u %s\n",
            flag->id, flag->pos.x, flag->pos.y, (unsigned)flag->roads.size(),
            connected ? "(connected)" : "(isolated — no carriers)");
        OutputDebugStringA(dbg);
        if (!connected) return;

        for (size_t i = 0; i < flag->roads.size(); ++i) {
            World::Road* road = flag->roads[i];
            if (!road) continue;
            if (road->tileCount < 2) continue;
            if (m_carrierManager->GetCarrierForRoad(road)) continue;
            m_carrierManager->CreateCarrier(road);
            // Only proceed if carrier was actually created
            if (!m_carrierManager->GetCarrierForRoad(road)) continue;
            World::Flag* rra = m_flagManager ? m_flagManager->ResolveFlag(road->a) : NULL;
            World::Flag* rrb = m_flagManager ? m_flagManager->ResolveFlag(road->b) : NULL;
            World::Flag* other = (rra == flag) ? rrb : rra;
            _snprintf(dbg, sizeof(dbg), "[Carrier] Created: road %u flag %u (%d,%d) <-> %u (%d,%d) tiles=%u\n",
                road->id, flag->id, flag->pos.x, flag->pos.y,
                other ? other->id : 0, other ? other->pos.x : -1, other ? other->pos.y : -1,
                (unsigned)road->tileCount);
            OutputDebugStringA(dbg);
            // Propagate to the other endpoint to cover chains of newly connected roads
            if (other) {
                SyncCarriersForFlag(other);
            }
        }
    }

    void GameScene::LinkFlagToRoadNetwork(World::Flag* flag)
    {
        if (!flag || !m_map || !m_flagManager || !m_roadManager) return;

        World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
        if (!roadsLayer) return;

        int rw = roadsLayer->GetWidth();
        int rh = roadsLayer->GetHeight();

        // BFS on EXISTING road tiles to find reachable flags and create Road objects
        int roadsCreated = 0;
        {
            std::vector<bool> visited(rw * rh, false);
            std::queue<std::pair<int,int>> q;
            std::vector<int> parent(rw * rh, -1);
            q.push(std::make_pair(flag->pos.x, flag->pos.y));
            visited[flag->pos.y * rw + flag->pos.x] = true;
            parent[flag->pos.y * rw + flag->pos.x] = -2;

            while (!q.empty()) {
                int cx = q.front().first;
                int cy = q.front().second;
                q.pop();

                World::Flag* other = (cx == flag->pos.x && cy == flag->pos.y) ? NULL : m_flagManager->GetFlagAt(cx, cy);
                if (other) {
                    if (!m_roadManager->GetRoadBetween(flag, other)) {
                        // Reconstruct tile path from BFS parent chain
                        std::vector<Vector2i> tilePath;
                        int px = cx, py = cy;
                        while (px != flag->pos.x || py != flag->pos.y) {
                            Vector2i v; v.x = px; v.y = py;
                            tilePath.push_back(v);
                            int p = parent[py * rw + px];
                            px = p & 0xFFFF;
                            py = (p >> 16) & 0xFFFF;
                        }
                        Vector2i sv; sv.x = flag->pos.x; sv.y = flag->pos.y;
                        tilePath.push_back(sv);
                        std::reverse(tilePath.begin(), tilePath.end());
                        m_roadManager->CreateRoad(flag, other, tilePath);
                        roadsCreated++;
                    }
                    continue;
                }

                bool evenRow = (cy % 2 == 0);
                int nx[6], ny[6];
                if (evenRow) {
                    int eNX[] = {cx-1, cx+1, cx-1, cx, cx-1, cx};
                    int eNY[] = {cy, cy, cy-1, cy-1, cy+1, cy+1};
                    memcpy(nx, eNX, sizeof(nx));
                    memcpy(ny, eNY, sizeof(ny));
                } else {
                    int oNX[] = {cx-1, cx+1, cx, cx+1, cx, cx+1};
                    int oNY[] = {cy, cy, cy-1, cy-1, cy+1, cy+1};
                    memcpy(nx, oNX, sizeof(nx));
                    memcpy(ny, oNY, sizeof(ny));
                }
                for (int di = 0; di < 6; ++di) {
                    int tx = nx[di];
                    int ty = ny[di];
                    if (tx < 0 || tx >= rw || ty < 0 || ty >= rh) continue;
                    if (visited[ty * rw + tx]) continue;
                    const World::Tile& rt = roadsLayer->GetTile(tx, ty);
                    if (rt.atlasName != "streets") continue;
                    visited[ty * rw + tx] = true;
                    parent[ty * rw + tx] = cx | (cy << 16);
                    q.push(std::make_pair(tx, ty));
                }
            }

            if (roadsCreated > 0) {
                char dbg[256];
                _snprintf(dbg, sizeof(dbg), "[Graph] LinkFlagToRoadNetwork: flag %u at (%d,%d) linked via roads to %d flag(s)\n",
                    flag->id, flag->pos.x, flag->pos.y, roadsCreated);
                OutputDebugStringA(dbg);
            }
        }

        if (flag->roads.empty()) {
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[Graph] LinkFlagToRoadNetwork: flag %u at (%d,%d) isolated — build a road to connect it\n",
                flag->id, flag->pos.x, flag->pos.y);
            OutputDebugStringA(dbg);
        }
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

    // ─── Geologist system ───────────────────────────────────────────────────
    void GameScene::ShowGeologistConfirm(int tx, int ty)
    {
        m_geologistState = GEOLOGIST_CONFIRM;
        m_geologistTileX = tx;
        m_geologistTileY = ty;
        m_geologistMenuActive = true;
        if (m_geologistMenu) m_geologistMenu->Show();
        m_statusText = "Геолог: A=да  B=нет";
        m_statusTextTimer = 0.0f;
    }

    void GameScene::StartGeologistSurvey()
    {
        if (m_geologistState != GEOLOGIST_CONFIRM) return;
        m_geologistState = GEOLOGIST_WORKING;
        m_geologistTimer = 60.0f;
        m_geologistMenuActive = false;
        if (m_geologistMenu) m_geologistMenu->Hide();
        m_statusText = "Geologist working...";
        m_statusTextTimer = 0.0f;
    }

    void GameScene::CancelGeologistMenu()
    {
        m_geologistState = GEOLOGIST_NONE;
        m_geologistTileX = -1;
        m_geologistTileY = -1;
        m_geologistMenuActive = false;
        if (m_geologistMenu) m_geologistMenu->Hide();
        m_statusText = "Survey cancelled";
        m_statusTextTimer = 2.0f;
    }

    void GameScene::RenderGeologistOverlay(Graphics::RenderQueue* renderQueue)
    {
        if (!m_map || !renderQueue) return;
        CoordinateSystem& coords = CoordinateSystem::GetInstance();

        TextureRegistry& reg = TextureRegistry::instance();
        std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");

        // 1. Mountain highlight — semi-transparent overlay on mountain tiles under cursor
        if (m_map) {
            const World::Tile& objTile = m_map->GetTile(World::Objects, m_cursorTileX, m_cursorTileY);
            if (objTile.type == World::Mountain || objTile.type == World::MountainOnWater || objTile.type == World::Rock) {
                // Render a semi-transparent colored overlay at the mountain tile position
                SpriteRenderer* sr = m_renderer ? m_renderer->GetSpriteRenderer() : NULL;
                float wx, wy;
                coords.NodeTileToWorld(m_cursorTileX, m_cursorTileY, wx, wy);
                int tileW = 60, tileH = 30;
                // Try to get tile dimensions from the mountain sprite in the Objects layer
                {
                    const World::Tile& objTile2 = m_map->GetTile(World::Objects, m_cursorTileX, m_cursorTileY);
                    if (objTile2.regionIndex >= 0) {
                        std::tr1::shared_ptr<SpriteAtlas> maptilesAtlas = reg.getAtlas("maptiles");
                        if (maptilesAtlas) {
                            const SpriteRegion* mountainReg = maptilesAtlas->GetRegion((uint32_t)objTile2.regionIndex);
                            if (mountainReg) {
                                tileW = (int)mountainReg->width;
                                tileH = (int)mountainReg->height;
                            }
                        }
                    }
                }
                std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
                LPDIRECT3DTEXTURE9 buildingsTex = buildingsAtlas ? buildingsAtlas->GetTexture() : NULL;
                if (sr && buildingsTex) sr->SetTextureSlot(SLOT_BUILDINGS_HIGHLIGHT, buildingsTex);
                Graphics::RenderCommandBuilder()
                    .WorldSprite(wx, wy,
                        (float)tileW, (float)tileH,
                        0.5f, 0.5f, 0.5001f, 0.5001f,
                        SLOT_BUILDINGS_HIGHLIGHT, static_cast<WORD>(0.98f * 65535.0f))
                    .Color(D3DCOLOR_ARGB(80, 255, 255, 0))
                    .Layer(LAYER_EFFECTS)
                    .Submit(renderQueue);
            }
        }

        // 2. Geologist confirm menu — UIMenu background + custom sprite/text items
        if (m_geologistMenu && (m_geologistMenuActive || m_geologistState == GEOLOGIST_CONFIRM)) {
            if (!m_geologistMenuActive && m_geologistState == GEOLOGIST_CONFIRM) {
                m_geologistMenuActive = true;
                m_geologistMenu->Show();
            }
            m_geologistMenu->Render();
            if (m_textManager && m_geologistMenu->IsVisible()) {
                std::tr1::shared_ptr<SpriteAtlas> uiAtl = reg.getAtlas("ui");
                std::tr1::shared_ptr<SpriteAtlas> iconAtl = reg.getAtlas("Icon");
                SpriteRenderer* sr2 = m_renderer ? m_renderer->GetSpriteRenderer() : NULL;
                float cx = 640.0f;
                float yOff = 200.0f;
                int iconSize = 40;

                // Slot constants for lambda capture (Xbox 360 compiler bug: enum values not accessible in lambdas)
                WORD kSlotBg = SLOT_UI_MENU_BG;
                WORD kSlotIcon = SLOT_UI_MENU_ICON;
                // Helper lambda: render a sprite by name from UI atlas, fallback Icon, fallback colored quad
                auto renderIcon = [&](const char* name, float x, float y, float w, float h, D3DCOLOR fallback) {
                    bool ok = false;
                    if (sr2) {
                        LPDIRECT3DTEXTURE9 tex = NULL;
                        float u0=0.5f,v0=0.5f,u1=0.5001f,v1=0.5001f;
                        if (uiAtl) {
                            uint32_t idx = uiAtl->GetIndex(name);
                            if (idx != 0xFFFFFFFF) {
                                const SpriteRegion* r = uiAtl->GetRegion(idx);
                                if (r) { u0=r->u0;v0=r->v0;u1=r->u1;v1=r->v1; tex=uiAtl->GetTexture(); }
                            }
                        }
                        if (!tex && iconAtl) {
                            uint32_t idx = iconAtl->GetIndex(name);
                            if (idx != 0xFFFFFFFF) {
                                const SpriteRegion* r = iconAtl->GetRegion(idx);
                                if (r) { u0=r->u0;v0=r->v0;u1=r->u1;v1=r->v1; tex=iconAtl->GetTexture(); }
                            }
                        }
                        if (tex) {
                            WORD slot = tex == uiAtl->GetTexture() ? kSlotBg : kSlotIcon;
                            sr2->SetTextureSlot(slot, tex);
                            Graphics::RenderCommandBuilder()
                                .UIElement(x, y, w, h, u0, v0, u1, v1, slot, 100)
                                .Submit(renderQueue);
                            ok = true;
                        }
                    }
                    if (!ok) {
                        Graphics::RenderCommandBuilder()
                            .UIElement(x, y, w, h, 0.5f, 0.5f, 0.5001f, 0.5001f, kSlotBg, 100)
                            .Color(fallback)
                            .Submit(renderQueue);
                    }
                };

                // Layout: icon_mountain, "Геолог", icon_geologist, ornament_1, "Отправить..."
                renderIcon("icon_mountain", cx - 24.0f, yOff, 48.0f, 48.0f, D3DCOLOR_ARGB(200, 140, 110, 80));
                m_textManager->DrawTextCenteredToScreen("Геолог", cx, yOff + 54.0f, D3DCOLOR_ARGB(255, 255, 255, 220), 0.095f, FONT_MENU, FONT_STYLE_NORMAL, LAYER_FOREGROUND);
                renderIcon("icon_geologist", cx - 18.0f, yOff + 90.0f, 36.0f, 36.0f, D3DCOLOR_ARGB(200, 255, 220, 100));
                renderIcon("ornament_1", cx - 50.0f, yOff + 132.0f, 100.0f, 14.0f, D3DCOLOR_ARGB(180, 180, 150, 80));
                m_textManager->DrawTextCenteredToScreen("Отправить геолога для поиска полезных ископаемых",
                    cx, yOff + 162.0f, D3DCOLOR_ARGB(255, 200, 200, 200), 0.08f, FONT_MENU, FONT_STYLE_NORMAL, LAYER_FOREGROUND);
            }
        }

        // 3. Resource icons on SURVEYED mountains (already explored)
        int w = m_map->GetWidth() * 2;
        int h = m_map->GetHeight() * 4;
        SpriteRenderer* sr = m_renderer ? m_renderer->GetSpriteRenderer() : NULL;
        std::tr1::shared_ptr<SpriteAtlas> iconAtlas = reg.getAtlas("Icon");

        for (int y = 0; y < h; y += 1) {
            for (int x = 0; x < w; x += 1) {
                const World::ResourceNode& node = m_map->GetResourceNode(x, y);
                if (!node.surveyed || node.type == World::ResourceType_None || node.amount <= 0) continue;

                float wx, wy;
                coords.NodeTileToWorld(x, y, wx, wy);

                // Try to render the deposit icon from the Icon atlas
                bool iconRendered = false;
                if (iconAtlas && sr) {
                    LPDIRECT3DTEXTURE9 iconTex = iconAtlas->GetTexture();
                    if (iconTex) {
                        const char* depositName = World::ResourceTypeToDepositIconName(node.type);
                        if (depositName && depositName[0] != '\0') {
                            sr->SetTextureSlot(SLOT_UI_MENU_ICON, iconTex);
                            uint32_t depositIdx = iconAtlas->GetIndex(depositName);
                            if (depositIdx != 0xFFFFFFFF) {
                                const SpriteRegion* depositReg = iconAtlas->GetRegion(depositIdx);
                                if (depositReg) {
                                    Graphics::RenderCommandBuilder()
                                        .WorldSprite(wx, wy - 40.0f,
                                            (float)depositReg->width * 0.8f, (float)depositReg->height * 0.8f,
                                            depositReg->u0, depositReg->v0, depositReg->u1, depositReg->v1,
                                            SLOT_UI_MENU_ICON, static_cast<WORD>(0.97f * 65535.0f))
                                        .Color(D3DCOLOR_ARGB(220, 255, 255, 255))
                                        .Layer(LAYER_EFFECTS)
                                        .Submit(renderQueue);
                                    iconRendered = true;
                                }
                            }
                        }
                    }
                }
                // Fallback: colored quad if deposit icon not found
                if (!iconRendered) {
                    D3DCOLOR fallbackColor = D3DCOLOR_ARGB(200, 255, 255, 0);
                    switch (node.type) {
                        case World::ResourceType_Coal:    fallbackColor = D3DCOLOR_ARGB(200, 80, 80, 80);    break;
                        case World::ResourceType_IronOre: fallbackColor = D3DCOLOR_ARGB(200, 180, 100, 50);  break;
                        case World::ResourceType_GoldOre: fallbackColor = D3DCOLOR_ARGB(200, 255, 215, 0);   break;
                        case World::ResourceType_Stone:   fallbackColor = D3DCOLOR_ARGB(200, 150, 150, 150); break;
                        case World::ResourceType_Marble:  fallbackColor = D3DCOLOR_ARGB(200, 200, 180, 220); break;
                        case World::ResourceType_Granite: fallbackColor = D3DCOLOR_ARGB(200, 130, 90, 70);   break;
                        default:                         fallbackColor = D3DCOLOR_ARGB(200, 255, 255, 0);   break;
                    }
                    Graphics::RenderCommandBuilder()
                        .WorldSprite(wx, wy - 40.0f,
                            24.0f, 24.0f,
                            0.5f, 0.5f, 0.5001f, 0.5001f,
                            SLOT_UI_MENU_ICON, static_cast<WORD>(0.97f * 65535.0f))
                        .Color(fallbackColor)
                        .Layer(LAYER_EFFECTS)
                        .Submit(renderQueue);
                }
            }
        }

        // 4. Geologist working indicator on the mountain being surveyed
        if (m_geologistState == GEOLOGIST_WORKING && m_geologistTileX >= 0 && m_geologistTileY >= 0) {
            float wx, wy;
            coords.NodeTileToWorld(m_geologistTileX, m_geologistTileY, wx, wy);
            bool iconRendered = false;
            if (iconAtlas && sr) {
                LPDIRECT3DTEXTURE9 iconTex = iconAtlas->GetTexture();
                if (iconTex) {
                    sr->SetTextureSlot(SLOT_UI_MENU_ICON, iconTex);
                    uint32_t workIdx = iconAtlas->GetIndex("icon_geologist_work");
                    if (workIdx != 0xFFFFFFFF) {
                        const SpriteRegion* workReg = iconAtlas->GetRegion(workIdx);
                        if (workReg) {
                            Graphics::RenderCommandBuilder()
                                .WorldSprite(wx, wy - 50.0f,
                                    (float)workReg->width, (float)workReg->height,
                                    workReg->u0, workReg->v0, workReg->u1, workReg->v1,
                                    SLOT_UI_MENU_ICON, static_cast<WORD>(0.97f * 65535.0f))
                                    .Layer(LAYER_EFFECTS)
                                    .Submit(renderQueue);
                            iconRendered = true;
                        }
                    }
                }
            }
            // Fallback: pulsing yellow quad if icon sprite not found
            if (!iconRendered) {
                Graphics::RenderCommandBuilder()
                    .WorldSprite(wx - 16.0f, wy - 50.0f,
                        32.0f, 32.0f,
                        0.5f, 0.5f, 0.5001f, 0.5001f,
                        SLOT_UI_MENU_ICON, static_cast<WORD>(0.97f * 65535.0f))
                    .Color(D3DCOLOR_ARGB(180, 255, 255, 0))
                    .Layer(LAYER_EFFECTS)
                    .Submit(renderQueue);
            }
        }
    }

    // ─── Gamepad cursor & D-Pad navigation ──────────────────────────────────
    void GameScene::HandleGamepadInput()
    {
        if (!m_inputManager || !m_map) return;
        if (m_menuActive || m_roadMenuActive || m_flagMenuActive || m_geologistMenuActive || m_townHallPanelOpen) return;

        Input::Gamepad* pad = m_inputManager->GetGamepad();
        if (!pad || !pad->IsConnected()) return;

        int dx = 0, dy = 0;

        // D-Pad: discrete tile movement
        if (pad->IsButtonPressed(Input::GP_DPadUp))    { dy = -1; m_gamepadActive = true; }
        if (pad->IsButtonPressed(Input::GP_DPadDown))  { dy = 1;  m_gamepadActive = true; }
        if (pad->IsButtonPressed(Input::GP_DPadLeft))  { dx = -1; m_gamepadActive = true; }
        if (pad->IsButtonPressed(Input::GP_DPadRight)) { dx = 1;  m_gamepadActive = true; }

        // Left stick: continuous movement with cooldown-based repeat
        if (dx == 0 && dy == 0 && m_gamepadCursorCooldown <= 0.0f) {
            float sx, sy;
            pad->GetLeftStick(sx, sy);
            if (fabsf(sx) > 0.5f || fabsf(sy) > 0.5f) {
                if (fabsf(sx) > 0.5f) dx = (sx > 0.0f) ? 1 : -1;
                if (fabsf(sy) > 0.5f) dy = (sy > 0.0f) ? 1 : -1;
                m_gamepadActive = true;
                m_gamepadCursorCooldown = 0.15f;
            }
        }

        if (dx != 0 || dy != 0) {
            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            int nodesW = coords.GetNodesWidth();
            int nodesH = coords.GetNodesHeight();

            m_gamepadCursor.x += dx;
            m_gamepadCursor.y += dy;

            if (m_gamepadCursor.x < 0) m_gamepadCursor.x = 0;
            if (m_gamepadCursor.x >= nodesW) m_gamepadCursor.x = nodesW - 1;
            if (m_gamepadCursor.y < 0) m_gamepadCursor.y = 0;
            if (m_gamepadCursor.y >= nodesH) m_gamepadCursor.y = nodesH - 1;

            m_cursorTileX = m_gamepadCursor.x;
            m_cursorTileY = m_gamepadCursor.y;
        }
    }

    // ─── Gamepad button handler (edge-triggered bitmask) ────────────────────
    void GameScene::OnGamepadButton(uint32_t buttons)
    {
        if (!m_map) return;

        // A button: interact with tile under gamepad cursor
        if (buttons & XINPUT_GAMEPAD_A) {
            const World::Tile& objTile = m_map->GetTile(World::Objects, m_gamepadCursor.x, m_gamepadCursor.y);
            if (objTile.type == World::Mountain || objTile.type == World::MountainOnWater || objTile.type == World::Rock) {
                // Use the same geologist flow as mouse A-press
                if (m_geologistState == GEOLOGIST_CONFIRM) {
                    StartGeologistSurvey();
                } else if (m_geologistState == GEOLOGIST_NONE) {
                    const World::ResourceNode& node = m_map->GetResourceNode(m_gamepadCursor.x, m_gamepadCursor.y);
                    if (!node.surveyed) {
                        ShowGeologistConfirm(m_gamepadCursor.x, m_gamepadCursor.y);
                    }
                }
            }
        }
    }

    // ─── Geologist popup (fixed buffers, no heap, O(1) swap-and-pop pool) ──
    void GameScene::SpawnGeologistPopup(int tx, int ty)
    {
        if (!m_map) return;

        // Deduplicate: same tile & visible → refresh timer
        for (int i = 0; i < m_popupCount; ++i) {
            World::PopupUiData& w = m_popups[i];
            if (w.tileX == tx && w.tileY == ty && w.isVisible) {
                w.timer = 5.0f;
                return;
            }
        }

        // Pool full → FIFO evict (shift left)
        if (m_popupCount >= (int)World::MAX_UI_POPUPS) {
            for (int i = 1; i < m_popupCount; ++i)
                m_popups[i - 1] = m_popups[i];
            m_popupCount--;
        }

        // Fill new slot
        World::PopupUiData& win = m_popups[m_popupCount++];
        win.tileX = tx;
        win.tileY = ty;
        win.timer = 5.0f;
        win.isVisible = true;

        strcpy(win.title, "GEOLOGIST REPORT");

        World::ResourceNode& node = m_map->GetResourceNode(tx, ty);
        if (node.type != World::ResourceType_None && node.amount > 0) {
            switch (node.type) {
                case World::ResourceType_Coal:    strcpy(win.line1, "Coal vein found");       break;
                case World::ResourceType_IronOre: strcpy(win.line1, "Iron ore found");        break;
                case World::ResourceType_GoldOre: strcpy(win.line1, "Gold vein found");       break;
                case World::ResourceType_Stone:   strcpy(win.line1, "Stone deposit found");   break;
                case World::ResourceType_Marble:  strcpy(win.line1, "Marble deposit found");  break;
                case World::ResourceType_Granite: strcpy(win.line1, "Granite deposit found"); break;
                default:                          strcpy(win.line1, "Minerals detected");      break;
            }
            _snprintf(win.line2, sizeof(win.line2), "Rich seam: %d units", node.amount);
        } else {
            strcpy(win.line1, "Barren rock");
            strcpy(win.line2, "No minerals found");
        }
    }

    // ─── Popup timer + gamepad cooldown management ─────────────────────────
    void GameScene::UpdateGamepadUI(float dt)
    {
        if (m_gamepadCursorCooldown > 0.0f)
            m_gamepadCursorCooldown -= dt;

        if (m_popupCount <= 0) return;

        for (int i = 0; i < m_popupCount; ++i) {
            World::PopupUiData& win = m_popups[i];
            if (!win.isVisible) continue;

            win.timer -= dt;
            if (win.timer <= 0.0f) {
                win.isVisible = false;
                m_popups[i] = m_popups[m_popupCount - 1];
                m_popupCount--;
                i--;
            }
        }
    }

    // ─── Push gamepad cursor + popups to RenderQueue ────────────────────────
    void GameScene::PushUiToQueue()
    {
        if (!m_textManager) return;

        TextureRegistry& reg = TextureRegistry::instance();

        // 1. Gamepad grid cursor sprite
        if (m_gamepadActive) {
            std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
            if (uiAtlas) {
                uint32_t cursorIdx = uiAtlas->GetIndex("cursor");
                if (cursorIdx != 0xFFFFFFFF) {
                    const SpriteRegion* cursorRegion = uiAtlas->GetRegion(cursorIdx);
                    if (cursorRegion) {
                        RenderQueue* rq = m_renderer ? m_renderer->GetRenderQueue() : NULL;
                        if (rq) {
                            float wx, wy;
                            CoordinateSystem::GetInstance().NodeTileToWorld(
                                m_gamepadCursor.x, m_gamepadCursor.y, wx, wy);

                            SpriteRenderer* sr = m_renderer->GetSpriteRenderer();
                            if (sr) {
                                LPDIRECT3DTEXTURE9 uiTex = uiAtlas->GetTexture();
                                if (uiTex) sr->SetTextureSlot(SLOT_UI_CURSOR, uiTex);
                            }

                            Graphics::RenderCommandBuilder()
                                .WorldSprite(wx - cursorRegion->pivotX, wy - cursorRegion->pivotY,
                                    (float)cursorRegion->width, (float)cursorRegion->height,
                                    cursorRegion->u0, cursorRegion->v0, cursorRegion->u1, cursorRegion->v1,
                                    SLOT_UI_CURSOR, static_cast<WORD>(0.99f * 65535.0f))
                                .Color(D3DCOLOR_ARGB(255, 0, 255, 0))
                                .Layer(LAYER_FOREGROUND)
                                .Submit(rq);
                        }
                    }
                }
            }
        }

        // NOTE: geologist popup was removed — results now show in banner
    }

    // ─── Job function implementations ───────────────────────────────────────
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