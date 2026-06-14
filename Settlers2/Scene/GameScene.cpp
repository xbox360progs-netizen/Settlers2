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
#include "../World/ObjectDeletionRules.h"
#include "../World/Warehouse.h"
#include "../World/Components/BuildingFactory.h"
#include "../Logic/CoordinateSystem.h"
#include "../Logic/AStar.h"
#include <queue>

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
        , m_menuActive(false)
        , m_roadMenuActive(false)
        , m_cursorOnTownHall(false)
        , m_townHallPanelOpen(false)
        , m_logisticsDebug(false)
        , m_buildState(BUILDSTATE_NONE)
        , m_selectedBuilding(World::Building_None)
        , m_placementIconIdx(-1)
        , m_placementConstrIdx(-1)
        , m_flagManager(NULL)
        , m_roadManager(NULL)
        , m_constructionManager(NULL)
        , m_objectLifecycleManager(NULL)
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
        , m_wildlifeRegenTimer(0.0f)
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
        m_wildlife = new World::WildlifeSystem(m_map, m_animalManager, m_animalSystem);
        m_map->SetWildlifeSystem(m_wildlife);
        OutputDebugStringA("[GameScene::Load] ECS wildlife ready\n");

        // Set up economy manager and link resource registry to map
        OutputDebugStringA("[GameScene::Load] Creating EconomyManager\n");
        m_economyManager = new Logic::EconomyManager();
        m_map->SetResourceRegistry(&m_economyManager->GetRegistry());
        m_map->GenerateWildlife();
        OutputDebugStringA("[GameScene::Load] EconomyManager ready\n");

        // Set up ECS carrier system and carrier manager
        OutputDebugStringA("[GameScene::Load] Creating CarrierSystem\n");
        m_carrierSystem = new World::CarrierSystem(m_entityManager);
        OutputDebugStringA("[GameScene::Load] Creating CarrierManager\n");
        m_carrierManager = new World::CarrierManager();
        m_carrierManager->SetCarrierSystem(m_carrierSystem);
        OutputDebugStringA("[GameScene::Load] CarrierManager ready\n");

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

        // Create construction manager
        OutputDebugStringA("[GameScene::Load] Creating ConstructionManager\n");
        m_constructionManager = new World::ConstructionManager();
        m_constructionManager->SetFlagManager(m_flagManager);
        m_constructionManager->SetRoadManager(m_roadManager);
        if (m_economyManager && m_economyManager->GetWarehouse() && m_economyManager->GetWarehouse()->connectedFlag) {
            m_constructionManager->SetWarehouseFlag(m_economyManager->GetWarehouse()->connectedFlag);
            m_carrierManager->SetWarehouseFlag(m_economyManager->GetWarehouse()->connectedFlag);
        }
        OutputDebugStringA("[GameScene::Load] ConstructionManager ready\n");

        // Create lifecycle manager
        m_objectLifecycleManager = new World::ObjectLifecycleManager(
            m_flagManager, m_roadManager, m_carrierManager,
            m_transportJobManager, m_constructionManager, m_economyManager);
        OutputDebugStringA("[GameScene::Load] ObjectLifecycleManager ready\n");

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

        // Restore any buildings placed in the editor from the Buildings layer
        RestoreBuildingsFromLayer();

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

            {
                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[Startup] Warehouse Wood=%d Stone=%d Planks=%d Fish=%d Meat=%d Coal=%d\n",
                    warehouse->resources[World::ResourceType_Wood],
                    warehouse->resources[World::ResourceType_Stone],
                    warehouse->resources[World::ResourceType_Planks],
                    warehouse->resources[World::ResourceType_Fish],
                    warehouse->resources[World::ResourceType_Meat],
                    warehouse->resources[World::ResourceType_Coal]);
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

    // ─── Camera movement and zoom (disabled during menu or town hall panel) ──
    if (!m_menuActive && !m_roadMenuActive && !m_townHallPanelOpen && m_camera && m_inputManager) {
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

    // ─── Update status text ──────────────────────────────────────────
    if (m_statusTextTimer > 0.0f) {
        m_statusTextTimer -= deltaTime;
        if (m_statusTextTimer <= 0.0f) m_statusText = "";
    }
    if (m_statusText.empty() || m_statusTextTimer <= 0.0f) {
        switch (m_buildState) {
            case BUILDSTATE_NONE:
                if (m_menuActive) {
                    m_statusText = "BUILD MENU: select a building";
                } else if (m_roadMenuActive) {
                    m_statusText = "ROAD MENU: choose action";
                } else {
                    m_statusText = "RB=Menu  A=interact  B=cancel";
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

    // ─── Input handling ──────────────────────────────────────────────
    if (m_inputManager) {
        Input::Gamepad* pad = m_inputManager->GetGamepad();
        if (pad) {
            bool rbPressed = pad->IsButtonPressed(Input::GP_RB);
            bool bPressed = pad->IsButtonPressed(Input::GP_B);
            bool aPressed = pad->IsButtonPressed(Input::GP_A);

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
                                    else if (lowerName == "toolmaker") bt = World::ToolMaker;
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
            } else if (m_roadMenuActive) {
                // Road/flag menu is open
                if (m_roadMenu) {
                    m_roadMenu->Update(pad, deltaTime);

                    if (bPressed) {
                        m_roadMenuActive = false;
                        m_roadMenu->Hide();
                        OutputDebugStringA("[GameScene] Road menu closed\n");
                    }

                    if (m_roadMenu->HasSelection()) {
                        int selIdx = m_roadMenu->GetSelectedSpriteIndex();
                        if (selIdx >= 0) {
                            std::tr1::shared_ptr<SpriteAtlas> uiAtlas = TextureRegistry::instance().getAtlas("ui");
                            if (uiAtlas) {
                                const SpriteRegion* reg = uiAtlas->GetRegion(selIdx);
                                if (reg) {
                                    std::string iconName = reg->name;
                                    if (iconName == "icon_create_road") {
                                        StartRoad(m_confirmTargetX, m_confirmTargetY);
                                    } else if (iconName == "icon_set_flag") {
                                        // Place flag at cursor position (not snapped flag position)
                                        PlaceFreeFlag(m_cursorTileX, m_cursorTileY);
                                        m_statusText = "Flag placed!";
                                        m_statusTextTimer = 2.0f;
                                    } else if (iconName == "icon_delete_flag") {
                                        OutputDebugStringA("[RoadMenu] Deleting flag...\n");
                                        if (m_flagManager) {
                                            World::Flag* f = m_flagManager->GetFlagAt(m_confirmTargetX, m_confirmTargetY);
                                            if (f) {
                                                char dbg[128];
                                                _snprintf(dbg, sizeof(dbg), "[RoadMenu] Found flag %p at (%d,%d)\n", f, m_confirmTargetX, m_confirmTargetY);
                                                OutputDebugStringA(dbg);
                                                
                                                // Cannot delete town hall / warehouse flag
                                                if (f->type == World::FLAG_WAREHOUSE) {
                                                    m_statusText = "Cannot delete town hall flag!";
                                                    m_statusTextTimer = 2.0f;
                                                } else if (f->building || f->pendingBuilding != World::Building_None) {
                                                    // Building attached — ask for confirmation
                                                    m_confirmAction = CONFIRM_DELETE_FLAG;
                                                    m_confirmTargetX = f->pos.x;
                                                    m_confirmTargetY = f->pos.y;
                                                    m_buildState = BUILDSTATE_CONFIRM;
                                                    m_statusText = "Delete building and flag? A=Yes B=No";
                                                    m_statusTextTimer = 3.0f;
                                                    m_roadMenuActive = false;
                                                    m_roadMenu->Hide();
                                                    m_roadMenu->ResetSelection();
                                                } else {
                                                    ClearRoadTilesForFlag(f);
                                                    m_objectLifecycleManager->ForceDeleteFlag(f);
                                                    m_statusText = "Flag removed!";
                                                    m_statusTextTimer = 2.0f;
                                                }
                                            } else {
                                                OutputDebugStringA("[RoadMenu] Flag not found at confirm target!\n");
                                            }
                                        } else {
                                            OutputDebugStringA("[RoadMenu] FlagManager is null!\n");
                                        }
                                    } else if (iconName == "icon_delete_Streets") {
                                        // Remove road tile at cursor
                                        if (m_map) {
                                            World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
                                            if (roadsLayer) {
                                                int rx = m_confirmTargetX, ry = m_confirmTargetY;
                                                if (rx >= 0 && rx < roadsLayer->GetWidth() && ry >= 0 && ry < roadsLayer->GetHeight()) {
                                                    World::Tile& rt = roadsLayer->GetTile(rx, ry);
                                                    if (rt.atlasName == "streets") {
                                                        rt.atlasName = "";
                                                        rt.regionIndex = -1;
                                                        rt.walkable = false;
                                                        m_statusText = "Road removed!";
                                                        m_statusTextTimer = 2.0f;
                                                        UpdateRoadNeighbors(rx, ry);
                                                    }
                                                }
                                            }
                                        }
                                    } else if (iconName == "icon_Streets") {
                                        // Open build menu instead
                                        m_roadMenuActive = false;
                                        m_roadMenu->Hide();
                                        if (m_buildMenu) {
                                            m_menuActive = true;
                                            m_buildMenu->ResetSelection();
                                            m_buildMenu->Show(640.0f, 360.0f);
                                            OutputDebugStringA("[GameScene] Switching from road menu to build menu\n");
                                        }
                                        m_roadMenu->ResetSelection();
                                    }
                                    // For other selections, skip the closing block below by using continue
                                    // (we want to close the menu normally)
                                }
                            }
                        }
                        m_roadMenuActive = false;
                        m_roadMenu->Hide();
                        m_roadMenu->ResetSelection();
                    }
                }
            } else if (m_townHallPanelOpen) {
                // Town hall resource panel is open → B to close
                if (bPressed) {
                    m_townHallPanelOpen = false;
                    m_statusText = "RB=Menu  A=interact  B=cancel";
                    m_statusTextTimer = 0.0f;
                }
            } else {
                // Normal mode: A to open town hall or road menu, RB to open build menu
                if (aPressed) {
                    if (m_cursorOnTownHall) {
                        m_townHallPanelOpen = true;
                        m_statusText = "Town Hall resources  B=close";
                        m_statusTextTimer = 0.0f;
                        OutputDebugStringA("[GameScene] Town hall panel opened\n");
                    } else {
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
                            // Open road/flag menu on flag
                            m_confirmTargetX = flagX;
                            m_confirmTargetY = flagY;
                            m_roadMenuActive = true;
                            m_roadMenu->ResetSelection();
                            m_roadMenu->Show(640.0f, 360.0f);
                            OutputDebugStringA("[GameScene] Road menu opened on flag\n");
                        } else {
                            // Check if cursor is on a road tile
                            bool cursorOnRoad = false;
                            if (m_map) {
                                World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
                                if (roadsLayer) {
                                    int rx = m_cursorTileX, ry = m_cursorTileY;
                                    if (rx >= 0 && rx < roadsLayer->GetWidth() && ry >= 0 && ry < roadsLayer->GetHeight()) {
                                        const World::Tile& rt = roadsLayer->GetTile(rx, ry);
                                        if (rt.atlasName == "streets") cursorOnRoad = true;
                                    }
                                }
                            }
                            if (cursorOnRoad) {
                                m_confirmTargetX = m_cursorTileX;
                                m_confirmTargetY = m_cursorTileY;
                                m_roadMenuActive = true;
                                m_roadMenu->ResetSelection();
                                m_roadMenu->Show(640.0f, 360.0f);
                                OutputDebugStringA("[GameScene] Road menu opened on road\n");
                            } else {
                                // Place flag on empty ground
                                m_buildState = BUILDSTATE_CONFIRM;
                                m_confirmAction = CONFIRM_PLACE_FLAG;
                                m_confirmTargetX = m_cursorTileX;
                                m_confirmTargetY = m_cursorTileY;
                            }
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
        }

        // ─── Phase B: TransportJob management ──────────────────────────
        if (m_transportJobManager) {
            m_transportJobManager->ScanFlagsForCargo(m_flagManager);
            m_transportJobManager->Update();
        }

        // Sync leg targets from all carriers to ECS (needed by ECS movement system)
        if (m_carrierManager && m_carrierSystem) {
            for (int i = 0; i < m_carrierManager->GetCarrierCount(); ++i) {
                World::Carrier* c = m_carrierManager->GetCarrier(i);
                if (c && c->ecsEntity != World::INVALID_ENTITY)
                    m_carrierSystem->SyncLegTargets(c->ecsEntity, c);
            }
        }

        // ─── Phase D: Carrier updates (per-segment walking) ──────────────
        if (m_carrierSystem)
            m_carrierSystem->UpdateMovement(deltaTime);

        if (m_carrierManager)
            m_carrierManager->Update(deltaTime);

        // ─── Phase D: Warehouse collects from its flag (AFTER carriers) ──
        if (m_economyManager)
            m_economyManager->CollectWarehouse();

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

        // ─── Deferred route recalculation (batched after all network changes) ──
        if (m_transportJobManager)
            m_transportJobManager->FlushRecalculate();

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
                                Graphics::RenderCommand cmd = {};
                                cmd.x = cx - dx * 0.5f;
                                cmd.y = cy - 3.0f;
                                cmd.width = dx;
                                cmd.height = 6.0f;
                                cmd.u0 = ewRegion->u0;
                                cmd.v0 = ewRegion->v0;
                                cmd.u1 = ewRegion->u1;
                                cmd.v1 = ewRegion->v1;
                                cmd.color = 0xFFFFFFFF;
                                cmd.textureID = SLOT_STREETS;
                                cmd.shaderID = SHADER_TERRAIN;
                                cmd.blendMode = 1;
                                cmd.layer = LAYER_WORLD;
                                cmd.depth = static_cast<WORD>(30000 + y * 400);
                                renderQueue->Submit(cmd);
                            }
                        }
                    }
                }
            }
        }
    }

    // ─── Render cursor or placement preview ─────────────────────────────
    if (m_buildState == BUILDSTATE_PLACE_FLAG && m_selectedBuilding != World::Building_None) {
        // Calculate building footprint position (offset from cursor using entrance offset)
        const char* spriteName = GetBuildingSpriteName(m_selectedBuilding);
        int entranceX = 0, entranceY = 0;
        GetEntranceOffset(spriteName ? spriteName : "", entranceX, entranceY);
        int buildY = m_cursorTileY - entranceY;
        bool buildingEvenY = (buildY % 2 == 0);
        AdjustEntranceForParity(buildingEvenY, entranceX, entranceY);
        int buildX = m_cursorTileX - entranceX;

        // Render the actual building sprite from Buildings atlas as a preview
        std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
        if (buildingsAtlas) {
            const char* spriteName = GetBuildingSpriteName(m_selectedBuilding);
            if (spriteName && spriteName[0]) {
                uint32_t idx = buildingsAtlas->GetIndex(spriteName);
                if (idx != 0xFFFFFFFF) {
                    const SpriteRegion* r = buildingsAtlas->GetRegion(idx);
                    if (r) {
                        float wx, wy;
                        CoordinateSystem::GetInstance().NodeTileToWorld(buildX, buildY, wx, wy);

                        if (spriteRenderer) {
                            LPDIRECT3DTEXTURE9 buildingsTex = buildingsAtlas->GetTexture();
                            if (buildingsTex) spriteRenderer->SetTextureSlot(SLOT_BUILDINGS_HIGHLIGHT, buildingsTex);
                        }

                        Graphics::RenderCommand pcmd = {};
                        pcmd.x = wx - r->pivotX;
                        pcmd.y = wy - r->pivotY;
                        pcmd.width = (float)r->width;
                        pcmd.height = (float)r->height;
                        pcmd.u0 = r->u0;
                        pcmd.v0 = r->v0;
                        pcmd.u1 = r->u1;
                        pcmd.v1 = r->v1;
                        pcmd.color = 0xAAFFFFFF;
                        pcmd.textureID = SLOT_BUILDINGS_HIGHLIGHT;
                        pcmd.shaderID = SHADER_TERRAIN;
                        pcmd.blendMode = 1;
                        pcmd.layer = LAYER_FOREGROUND;
                        pcmd.depth = static_cast<WORD>(0.98f * 65535.0f);
                        renderQueue->Submit(pcmd);
                    }
                }
            }
        }
//        OutputDebugStringA("[GameScene::Render] Placement preview rendered\n");
    } else if (!m_menuActive && !m_roadMenuActive) {
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
                    cmd.textureID = SLOT_BUILDINGS_HIGHLIGHT;
                    cmd.shaderID = SHADER_TERRAIN;
                    cmd.blendMode = 1;
                    cmd.layer = LAYER_WORLD;
                    cmd.depth = static_cast<WORD>(30010 + fy * 400);
                    renderQueue->Submit(cmd);
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

                    Graphics::RenderCommand cmd = {};
                    cmd.x = fx - r->pivotX * 0.5f;
                    cmd.y = fy - r->pivotY * 0.5f - 30.0f + iconY * -16.0f;
                    cmd.width = r->width * 0.5f;
                    cmd.height = r->height * 0.5f;
                    cmd.u0 = r->u0; cmd.v0 = r->v0;
                    cmd.u1 = r->u1; cmd.v1 = r->v1;
                    cmd.color = 0xFFFFFFFF;
                    cmd.textureID = SLOT_FLAG_RESOURCES;
                    cmd.shaderID = SHADER_TERRAIN;
                    cmd.blendMode = 1;
                    cmd.layer = LAYER_WORLD;
                    cmd.depth = static_cast<WORD>(30011 + flag->pos.y * 400 + iconY);
                    renderQueue->Submit(cmd);
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
            if (m_carrierManager) {
                for (int ci = 0; ci < m_carrierManager->GetCarrierCount(); ++ci) {
                    World::Carrier* carrier = m_carrierManager->GetCarrier(ci);
                    if (!carrier) continue;

                    const std::vector<Vector2i>* pathPtr = NULL;
                    float ep = 0.0f;
                    float walkDir = carrier->walkDir;

                    if (World::IsTransitState(carrier->state)) {
                        if (carrier->transitTiles.size() < 2) continue;
                        pathPtr = &carrier->transitTiles;
                        ep = carrier->transitProgress;
                    } else {
                        if (!carrier->road || carrier->road->tiles.size() < 2) continue;
                        pathPtr = &carrier->road->tiles;
                        ep = carrier->ep;
                    }

                    const std::vector<Vector2i>& path = *pathPtr;
                    int pathLen = (int)path.size() - 1;
                    if (ep < 0.0f) ep = 0.0f;
                    if (ep > (float)pathLen) ep = (float)pathLen;
                    int idx = (int)ep;
                    float frac = ep - (float)idx;
                    if (idx >= pathLen) { idx = pathLen - 1; frac = 1.0f; }
                    if (idx < 0) { idx = 0; frac = 0.0f; }

                    const Vector2i& tileA = path[idx];
                    const Vector2i& tileB = path[idx + 1];

                    int dx = (walkDir > 0.0f) ? (tileB.x - tileA.x) : (tileA.x - tileB.x);
                    int dy = (walkDir > 0.0f) ? (tileB.y - tileA.y) : (tileA.y - tileB.y);
                    bool hasCargo = (carrier->cargo.type != World::ResourceType_None && carrier->cargo.amount > 0);
                    int spriteIdx = unitsSpriteIndex(true, hasCargo, dx, dy);

                    float wx0, wy0, wx1, wy1;
                    coords.NodeTileToWorld(tileA.x, tileA.y, wx0, wy0);
                    coords.NodeTileToWorld(tileB.x, tileB.y, wx1, wy1);
                    float wx = wx0 + (wx1 - wx0) * frac;
                    float wy = wy0 + (wy1 - wy0) * frac;

                    const SpriteRegion* r = unitsAtlas->GetRegion(spriteIdx);
                    if (!r) continue;

                    Graphics::RenderCommand cmd = {};
                    cmd.x = wx - r->pivotX;
                    cmd.y = wy - r->pivotY;
                    cmd.width = (float)r->width;
                    cmd.height = (float)r->height;
                    cmd.u0 = r->u0; cmd.v0 = r->v0;
                    cmd.u1 = r->u1; cmd.v1 = r->v1;
                    cmd.color = 0xFFFFFFFF;
                    cmd.textureID = SLOT_UNITS;
                    cmd.shaderID = SHADER_TERRAIN;
                    cmd.blendMode = 1;
                    cmd.layer = LAYER_WORLD;
                    cmd.depth = static_cast<WORD>(30020 + tileA.y * 400);
                    renderQueue->Submit(cmd);

                    // Render cargo icon above carrier
                    if (carrier->cargo.type != World::ResourceType_None && carrier->cargo.amount > 0) {
                        const char* cargoIconName = World::ResourceTypeToIconName(carrier->cargo.type);
                        if (cargoIconName && cargoIconName[0]) {
                            std::tr1::shared_ptr<SpriteAtlas> cargoAtlas = reg.getAtlas("Icon");
                            if (cargoAtlas) {
                                uint32_t cargoIdx = cargoAtlas->GetIndex(cargoIconName);
                                if (cargoIdx != 0xFFFFFFFF) {
                                    const SpriteRegion* cargoR = cargoAtlas->GetRegion(cargoIdx);
                                    if (cargoR) {
                                        float cargoSize = 16.0f;
                                        Graphics::RenderCommand ccmd = {};
                                        ccmd.x = wx - cargoSize * 0.5f;
                                        ccmd.y = wy - r->pivotY - cargoSize;
                                        ccmd.width = cargoSize;
                                        ccmd.height = cargoSize;
                                        ccmd.u0 = cargoR->u0;
                                        ccmd.v0 = cargoR->v0;
                                        ccmd.u1 = cargoR->u1;
                                        ccmd.v1 = cargoR->v1;
                                        ccmd.color = 0xFFFFFFFF;
                                        ccmd.textureID = SLOT_UI_MENU_ICON;
                                        ccmd.shaderID = SHADER_TERRAIN;
                                        ccmd.blendMode = 1;
                                        ccmd.layer = LAYER_WORLD;
                                        ccmd.depth = static_cast<WORD>(30030 + tileA.y * 400);
                                        renderQueue->Submit(ccmd);
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
                    } else if (flag->building->type == World::Fisher) {
                        bool se = (flag->building->pos.x % 2 == 0);
                        wSpriteIdx = se ? 16 : 17; // fisher_work_SE / fisher_work_SW
                        coords.NodeTileToWorld(flag->building->pos.x, flag->building->pos.y, wx, wy);
                    }
                    if (wSpriteIdx < 0) continue;
                    const SpriteRegion* wr = unitsAtlas->GetRegion(wSpriteIdx);
                    if (!wr) continue;

                    Graphics::RenderCommand wcmd = {};
                    wcmd.x = wx - wr->pivotX;
                    wcmd.y = wy - wr->pivotY;
                    wcmd.width = (float)wr->width;
                    wcmd.height = (float)wr->height;
                    wcmd.u0 = wr->u0; wcmd.v0 = wr->v0;
                    wcmd.u1 = wr->u1; wcmd.v1 = wr->v1;
                    wcmd.color = 0xFFFFFFFF;
                    wcmd.textureID = SLOT_UNITS;
                    wcmd.shaderID = SHADER_TERRAIN;
                    wcmd.blendMode = 1;
                    wcmd.layer = LAYER_WORLD;
                    wcmd.depth = static_cast<WORD>(30020 + (moving ? (int)(wy + 0.5f) : flag->building->pos.y) * 400);
                    renderQueue->Submit(wcmd);
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
                        if (site->builderRoute.size() < 2) continue;
                        uint32_t fromIdx = site->builderRouteIndex;
                        uint32_t toIdx = fromIdx + 1;
                        if (fromIdx >= site->builderRoute.size() - 1) {
                            size_t lastIdx = site->builderRoute.size() - 1;
                            World::Flag* f = site->builderRoute[lastIdx];
                            coords.NodeTileToWorld(f->pos.x, f->pos.y, wx, wy);
                        } else {
                            World::Flag* fromFlag = site->builderRoute[fromIdx];
                            World::Flag* toFlag = site->builderRoute[toIdx];
                            World::Road* road = m_roadManager ? m_roadManager->GetRoadBetween(fromFlag, toFlag) : NULL;
                            if (road && road->tiles.size() >= 2) {
                                int tileCount = (int)road->tiles.size();
                                float pathLen = (float)(tileCount - 1);
                                float pos = site->builderEp;
                                if (pos < 0.0f) pos = 0.0f;
                                if (pos > pathLen) pos = pathLen;
                                int tileIdx = (int)pos;
                                float frac = pos - (float)tileIdx;
                                if (tileIdx >= tileCount - 1) { tileIdx = tileCount - 2; frac = 1.0f; }
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

                    Graphics::RenderCommand cmd = {};
                    cmd.x = wx - r->pivotX;
                    cmd.y = wy - r->pivotY;
                    cmd.width = (float)r->width;
                    cmd.height = (float)r->height;
                    cmd.u0 = r->u0; cmd.v0 = r->v0;
                    cmd.u1 = r->u1; cmd.v1 = r->v1;
                    cmd.color = 0xFFFFFFFF;
                    cmd.textureID = SLOT_UNITS;
                    cmd.shaderID = SHADER_TERRAIN;
                    cmd.blendMode = 1;
                    cmd.layer = LAYER_WORLD;
                    cmd.depth = static_cast<WORD>(30020 + site->flag->pos.y * 400);
                    renderQueue->Submit(cmd);
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
                        Graphics::RenderCommand cmd = {};
                        cmd.x = wx - r->pivotX;
                        cmd.y = wy - r->pivotY;
                        cmd.width = (float)r->width;
                        cmd.height = (float)r->height;
                        cmd.u0 = r->u0; cmd.v0 = r->v0;
                        cmd.u1 = r->u1; cmd.v1 = r->v1;
                        cmd.color = 0xFFFFFFFF;
                        cmd.textureID = SLOT_UNITS;
                        cmd.shaderID = SHADER_TERRAIN;
                        cmd.blendMode = 1;
                        cmd.layer = LAYER_WORLD;
                        cmd.depth = static_cast<WORD>(30005 + (int)(a.y + 0.5f) * 400);
                        renderQueue->Submit(cmd);
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

            Graphics::RenderCommand cmd = {};
            cmd.x = wx - region->pivotX + flagAlignOffsetX;
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
            cmd.layer = LAYER_FOREGROUND;
            cmd.depth = static_cast<WORD>(0.98f * 65535.0f);
            renderQueue->Submit(cmd);
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
                        Graphics::RenderCommand cmd = {};
                        cmd.x = cx - dx * 0.5f + flagAlignOffsetX;
                        cmd.y = cy - 3.0f;
                        cmd.width = dx;
                        cmd.height = 6.0f;
                        cmd.u0 = ewRegion->u0;
                        cmd.v0 = ewRegion->v0;
                        cmd.u1 = ewRegion->u1;
                        cmd.v1 = ewRegion->v1;
                        cmd.color = D3DCOLOR_ARGB(160, 255, 255, 255);
                        cmd.textureID = SLOT_STREETS;
                        cmd.shaderID = SHADER_TERRAIN;
                        cmd.blendMode = 1;
                        cmd.layer = LAYER_FOREGROUND;
                        cmd.depth = static_cast<WORD>(0.98f * 65535.0f);
                        renderQueue->Submit(cmd);
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
                        Graphics::RenderCommand cmd = {};
                        cmd.x = wx - region->pivotX + flagAlignOffsetX;
                        cmd.y = wy - region->pivotY;
                        cmd.width = (float)region->width;
                        cmd.height = (float)region->height;
                        cmd.u0 = region->u0;
                        cmd.v0 = region->v0;
                        cmd.u1 = region->u1;
                        cmd.v1 = region->v1;
                        cmd.color = D3DCOLOR_ARGB(160, 100, 200, 255);
                        cmd.textureID = SLOT_STREETS;
                        cmd.shaderID = SHADER_TERRAIN;
                        cmd.blendMode = 1;
                        cmd.layer = LAYER_FOREGROUND;
                        cmd.depth = static_cast<WORD>(0.98f * 65535.0f);
                        renderQueue->Submit(cmd);
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
                            Graphics::RenderCommand cmd = {};
                            cmd.x = cx - dx * 0.5f + flagAlignOffsetX;
                            cmd.y = cy - 3.0f;
                            cmd.width = dx;
                            cmd.height = 6.0f;
                            cmd.u0 = ewRegion->u0;
                            cmd.v0 = ewRegion->v0;
                            cmd.u1 = ewRegion->u1;
                            cmd.v1 = ewRegion->v1;
                            cmd.color = D3DCOLOR_ARGB(160, 100, 200, 255);
                            cmd.textureID = SLOT_STREETS;
                            cmd.shaderID = SHADER_TERRAIN;
                            cmd.blendMode = 1;
                            cmd.layer = LAYER_FOREGROUND;
                            cmd.depth = static_cast<WORD>(0.98f * 65535.0f);
                            renderQueue->Submit(cmd);
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
                        Graphics::RenderCommand cmd = {};
                        cmd.x = wx - region->pivotX + flagAlignOffsetX;
                        cmd.y = wy - region->pivotY;
                        cmd.width = (float)region->width;
                        cmd.height = (float)region->height;
                        cmd.u0 = region->u0;
                        cmd.v0 = region->v0;
                        cmd.u1 = region->u1;
                        cmd.v1 = region->v1;
                        cmd.color = D3DCOLOR_ARGB(120, 255, 100, 100);
                        cmd.textureID = SLOT_STREETS;
                        cmd.shaderID = SHADER_TERRAIN;
                        cmd.blendMode = 1;
                        cmd.layer = LAYER_FOREGROUND;
                        cmd.depth = static_cast<WORD>(0.99f * 65535.0f);
                        renderQueue->Submit(cmd);
                    }
                }
            }
        }
    }

    // ─── Render build menu (if active) ───────────────────────────────────
    if (m_buildMenu && m_menuActive) {
        m_buildMenu->Render();
    }

    // ─── Render road/flag menu (if active) ───────────────────────────────
    if (m_roadMenu && m_roadMenuActive) {
        m_roadMenu->Render();
    }

    // ─── Hunting spots overlay when hunter building is selected ─────────
    if (m_roadMenuActive && m_flagManager && m_map && m_textManager) {
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
                            Graphics::RenderCommand icmd = {};
                            icmd.x = wx - iconSize * 0.5f;
                            icmd.y = wy - iconSize;
                            icmd.width = iconSize;
                            icmd.height = iconSize;
                            icmd.u0 = deerR->u0;
                            icmd.v0 = deerR->v0;
                            icmd.u1 = deerR->u1;
                            icmd.v1 = deerR->v1;
                            icmd.color = D3DCOLOR_ARGB(200, 255, 255, 255);
                            icmd.textureID = SLOT_UI_MENU_ICON;
                            icmd.shaderID = SHADER_TERRAIN;
                            icmd.blendMode = 1;
                            icmd.layer = LAYER_FOREGROUND;
                            icmd.depth = static_cast<WORD>(0.99f * 65535.0f);
                            renderQueue->Submit(icmd);
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

    if (!m_menuActive && !m_roadMenuActive && !m_townHallPanelOpen) {
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
                        Graphics::RenderCommand cmd = {};
                        cmd.x = wx - r->pivotX;
                        cmd.y = wy - r->pivotY;
                        cmd.width = (float)r->width;
                        cmd.height = (float)r->height;
                        cmd.u0 = r->u0;
                        cmd.v0 = r->v0;
                        cmd.u1 = r->u1;
                        cmd.v1 = r->v1;
                        cmd.color = D3DCOLOR_ARGB(80, 255, 255, 255);
                        cmd.textureID = SLOT_BUILDINGS_HIGHLIGHT;
                        cmd.shaderID = SHADER_TERRAIN;
                        cmd.blendMode = 1;
                        cmd.layer = LAYER_FOREGROUND;
                        cmd.depth = static_cast<WORD>(0.99f * 65535.0f);
                        renderQueue->Submit(cmd);
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

            Graphics::RenderCommand cmd = {};
            cmd.x = panelLeft;
            cmd.y = panelTop;
            cmd.width = m_townHallPanelW;
            cmd.height = m_townHallPanelH;
            cmd.u0 = m_townHallPanelU0;
            cmd.v0 = m_townHallPanelV0;
            cmd.u1 = m_townHallPanelU1;
            cmd.v1 = m_townHallPanelV1;
            cmd.color = 0xFFFFFFFF;
            cmd.textureID = SLOT_UI_TOWNHALL_PANEL;
            cmd.shaderID = SHADER_UI;
            cmd.blendMode = 1;
            cmd.layer = LAYER_UI;
            cmd.depth = 10;
            cmd.sortKey = Graphics::BuildSortKey(LAYER_UI, 1, SHADER_UI, SLOT_UI_TOWNHALL_PANEL, 10);
            renderQueue->Submit(cmd);

            // Render resource counts on the panel
            if (m_textManager && m_economyManager) {
                World::Warehouse* wh = m_economyManager->GetWarehouse();
                if (wh) {
                    float tx = panelLeft + 40.0f;
                    float ty = panelTop + 30.0f;
                    float lineH = 28.0f;
                    char buf[64];

                    _snprintf(buf, sizeof(buf), "Wood: %d", wh->resources[World::ResourceType_Wood]);
                    m_textManager->DrawString(buf, tx, ty, 0xFFFFFFFF, 0.08f); ty += lineH;
                    _snprintf(buf, sizeof(buf), "Planks: %d", wh->resources[World::ResourceType_Planks]);
                    m_textManager->DrawString(buf, tx, ty, 0xFFFFFFFF, 0.08f); ty += lineH;
                    _snprintf(buf, sizeof(buf), "Stone: %d", wh->resources[World::ResourceType_Stone]);
                    m_textManager->DrawString(buf, tx, ty, 0xFFFFFFFF, 0.08f); ty += lineH;
                    _snprintf(buf, sizeof(buf), "Fish: %d", wh->resources[World::ResourceType_Fish]);
                    m_textManager->DrawString(buf, tx, ty, 0xFFFFFFFF, 0.08f); ty += lineH;
                    _snprintf(buf, sizeof(buf), "Meat: %d", wh->resources[World::ResourceType_Meat]);
                    m_textManager->DrawString(buf, tx, ty, 0xFFFFFFFF, 0.08f); ty += lineH;
                    _snprintf(buf, sizeof(buf), "Coal: %d", wh->resources[World::ResourceType_Coal]);
                    m_textManager->DrawString(buf, tx, ty, 0xFFFFFFFF, 0.08f); ty += lineH;
                }
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
                const char* spriteName = GetBuildingSpriteName(flag->building->type);
                if (!spriteName || !*spriteName) continue;
                uint32_t sprIdx = buildingsAtlas->GetIndex(spriteName);
                if (sprIdx == 0xFFFFFFFF) continue;
                const SpriteRegion* r = buildingsAtlas->GetRegion(sprIdx);
                if (!r) continue;
                int bldX = flag->building->pos.x;
                int bldY = flag->building->pos.y;
                float wx, wy;
                coords.NodeTileToWorld(bldX, bldY, wx, wy);
                Graphics::RenderCommand cmd = {};
                cmd.x = wx - r->pivotX;
                cmd.y = wy - r->pivotY;
                cmd.width = (float)r->width;
                cmd.height = (float)r->height;
                cmd.u0 = r->u0;
                cmd.v0 = r->v0;
                cmd.u1 = r->u1;
                cmd.v1 = r->v1;
                cmd.color = D3DCOLOR_ARGB(80, 255, 255, 255);
                cmd.textureID = SLOT_BUILDINGS_HIGHLIGHT;
                cmd.shaderID = SHADER_TERRAIN;
                cmd.blendMode = 1;
                cmd.layer = LAYER_FOREGROUND;
                cmd.depth = static_cast<WORD>(0.99f * 65535.0f);
                renderQueue->Submit(cmd);
            }
        }
    }

    // ─── Resource HUD bar at top of screen ──────────────────────────────
    if (m_resourceHudLoaded) {
        TextureRegistry& reg2 = TextureRegistry::instance();
        std::tr1::shared_ptr<SpriteAtlas> resIconAtlas = reg2.getAtlas("Icon");
        if (resIconAtlas) {
            float barX = 10.0f;
            float barY = 6.0f;
            float iconSize = 28.0f;
            float spacing = 60.0f;

            for (int i = 0; i < RESOURCE_HUD_COUNT; ++i) {
                if (m_resourceHud[i].iconIdx < 0) continue;

                const SpriteRegion* r = resIconAtlas->GetRegion(m_resourceHud[i].iconIdx);
                if (!r) continue;

                // Render icon
                Graphics::RenderCommand icmd = {};
                icmd.x = barX;
                icmd.y = barY;
                icmd.width = iconSize;
                icmd.height = iconSize;
                icmd.u0 = r->u0;
                icmd.v0 = r->v0;
                icmd.u1 = r->u1;
                icmd.v1 = r->v1;
                icmd.color = 0xFFFFFFFF;
                icmd.textureID = SLOT_UI_MENU_ICON;
                icmd.shaderID = SHADER_UI;
                icmd.blendMode = 1;
                icmd.layer = LAYER_FOREGROUND;
                icmd.depth = 200;
                renderQueue->Submit(icmd);

                // Render resource count
                if (m_economyManager) {
                    World::Warehouse* wh = m_economyManager->GetWarehouse();
                    if (wh) {
                        char buf[32];
                        _snprintf(buf, sizeof(buf), "%d", wh->resources[m_resourceHud[i].type]);
                        float textX = barX + iconSize + 4.0f;
                        float textY = barY + (iconSize - 14.0f) * 0.5f;
                        m_textManager->DrawString(buf, textX, textY, 0xFFFFFFFF, 0.07f);
                    }
                }

                barX += spacing;
            }
        }
    }

    // ─── Render status text ─────────────────────────────────────────────
    if (m_textManager && !m_statusText.empty()) {
        float screenW = 1280.0f;
        float screenH = 720.0f;
        m_textManager->DrawTextCenteredToScreen(m_statusText, screenW * 0.5f, screenH - 40.0f, 0xFFFFFFFF, 0.08f);
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
                m_textManager->DrawString(buf, wx - 30.0f, ty, D3DCOLOR_ARGB(220, 255, 255, 200), 0.06f, FONT_DEBUG, false);
            }
        }

        // Carriers: cargo (per-segment walking)
        if (m_carrierManager) {
            char buf[64];
            for (int ci = 0; ci < m_carrierManager->GetCarrierCount(); ++ci) {
                World::Carrier* carrier = m_carrierManager->GetCarrier(ci);
                if (!carrier) continue;

                const std::vector<Vector2i>* pathPtr = NULL;
                float ep = 0.0f;

                 if (World::IsTransitState(carrier->state)) {

                    if (carrier->transitTiles.size() < 2) continue;
                    pathPtr = &carrier->transitTiles;
                    ep = carrier->transitProgress;
                } else {
                    if (!carrier->road || carrier->road->tiles.size() < 2) continue;
                    pathPtr = &carrier->road->tiles;
                    ep = carrier->ep;
                }

                const std::vector<Vector2i>& path = *pathPtr;
                int pathLen = (int)path.size() - 1;
                if (ep < 0.0f) ep = 0.0f;
                if (ep > (float)pathLen) ep = (float)pathLen;
                int idx = (int)ep;
                float frac = ep - (float)idx;
                if (idx >= pathLen) { idx = pathLen - 1; frac = 1.0f; }
                if (idx < 0) { idx = 0; frac = 0.0f; }

                const Vector2i& tileA = path[idx];
                const Vector2i& tileB = path[idx + 1];

                float cx, cy, nx, ny;
                coords.NodeTileToWorld(tileA.x, tileA.y, cx, cy);
                coords.NodeTileToWorld(tileB.x, tileB.y, nx, ny);
                float wx = cx + (nx - cx) * frac;
                float wy = cy + (ny - cy) * frac;

                const char* cargoName = "Idle";
                if (carrier->cargo.type != World::ResourceType_None) {
                    cargoName = World::ResourceTypeToString(carrier->cargo.type);
                }

                if (carrier->road) {
                    _snprintf(buf, sizeof(buf), "%s %u<->%u", cargoName,
                        carrier->m_roadEndpointA ? carrier->m_roadEndpointA->id : 0,
                        carrier->m_roadEndpointB ? carrier->m_roadEndpointB->id : 0);
                } else {
                    _snprintf(buf, sizeof(buf), "%s (transit)", cargoName);
                }
                m_textManager->DrawString(buf, wx - 20.0f, wy - 20.0f, D3DCOLOR_ARGB(220, 200, 255, 200), 0.05f, FONT_DEBUG, false);
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
        Graphics::RenderCommand cmd = {};
        cmd.x = worldX - cursorRegion->pivotX;
        cmd.y = worldY - cursorRegion->pivotY;
        cmd.width = (float)cursorRegion->width;
        cmd.height = (float)cursorRegion->height;
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
        m_roadMenu->SetTextures(uiTex, uiTex, uiTex);

        GridMenu::TileUV bgUV = {0,0,1,1}, cellUV = {0,0,1,1};
        uint32_t bgIdx = uiAtlas->GetIndex("menu_Grid");
        if (bgIdx != 0xFFFFFFFF) {
            const SpriteRegion* r = uiAtlas->GetRegion(bgIdx);
            if (r) { bgUV.u0 = r->u0; bgUV.v0 = r->v0; bgUV.u1 = r->u1; bgUV.v1 = r->v1; }
        } else {
            OutputDebugStringA("[GameScene] WARNING: 'menu_Grid' NOT FOUND in UI atlas\n");
        }
        uint32_t cellIdx = uiAtlas->GetIndex("menu_cell3");
        if (cellIdx != 0xFFFFFFFF) {
            const SpriteRegion* r = uiAtlas->GetRegion(cellIdx);
            if (r) { cellUV.u0 = r->u0; cellUV.v0 = r->v0; cellUV.u1 = r->u1; cellUV.v1 = r->v1; }
        } else {
            OutputDebugStringA("[GameScene] WARNING: 'menu_cell3' NOT FOUND in UI atlas\n");
        }
        m_roadMenu->SetBackgroundUV(bgUV);
        m_roadMenu->SetCellUV(cellUV);

        // Road/flag icons: icon_create_road, icon_set_flag, icon_delete_flag, icon_Streets, icon_delete_Streets
        const char* iconNames[] = {
            "icon_create_road",
            "icon_set_flag",
            "icon_delete_flag",
            "icon_delete_Streets",
            "icon_Streets",
        };
        const char* iconLabels[] = {
            "Build Road",
            "Set Flag",
            "Delete Flag",
            "Delete Road",
            "Buildings",
        };
        std::vector<GridMenu::TileUV> tileUVs;
        std::vector<int> spriteIndices;
        std::vector<std::string> cellLabels;

        for (int i = 0; i < 5; ++i) {
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
        }

        m_roadMenu->SetCellLabels(cellLabels);
        m_roadMenu->SetCellSpacing(80.0f, 80.0f);
        m_roadMenu->SetCellPadding(4.0f);
        m_roadMenu->SetCellVisualSize(64.0f, 64.0f);
        m_roadMenu->SetTileData(tileUVs, spriteIndices);

        OutputDebugStringA("[GameScene::InitRoadMenu] DONE\n");
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
            for (size_t t = 0; t < road->tiles.size(); ++t) {
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

        if (!footMask.empty()) {
            for (size_t i = 0; i < footMask.size(); ++i) {
                int tx = buildX + footOffX + footMask[i].first;
                int ty = buildY + footOffY + footMask[i].second;
                if (!checkTile(tx, ty)) return false;
            }
        } else {
            for (int dy = 0; dy < footH; ++dy) {
                for (int dx = 0; dx < footW; ++dx) {
                    int tx = buildX + footOffX + dx;
                    int ty = buildY + footOffY + dy;
                    if (!checkTile(tx, ty)) return false;
                }
            }
        }

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
        const char* spriteName = GetBuildingSpriteName(m_selectedBuilding);
        int entranceX = 0, entranceY = 0;
        GetEntranceOffset(spriteName ? spriteName : "", entranceX, entranceY);

        // Calculate building footprint position (flag position minus entrance offset)
        int buildY = tileY - entranceY;
        bool buildingEvenY = (buildY % 2 == 0);
        AdjustEntranceForParity(buildingEvenY, entranceX, entranceY);
        int buildX = tileX - entranceX;

        // Validate building footprint with CanPlaceBuilding
        bool canPlace = CanPlaceBuilding(m_selectedBuilding, buildX, buildY);
        {
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[GameScene] CanPlaceBuilding(%d,%d) type=%d result=%d\n",
                buildX, buildY, (int)m_selectedBuilding, canPlace ? 1 : 0);
            OutputDebugStringA(dbg);
        }
        if (!canPlace) {
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

        // Split any road that passes through this flag position (BEFORE linking)
        SplitRoadAtFlag(flag);

        // Link flag to road network so carriers can reach it
        LinkFlagToRoadNetwork(flag);
        SyncCarriersForFlag(flag);
        if (m_transportJobManager) m_transportJobManager->MarkRoutesDirty();
        if (m_constructionManager) m_constructionManager->MarkBuilderRoutesDirty();

        // Reset build state
        m_buildState = BUILDSTATE_NONE;
        m_placementIconIdx = -1;
        m_placementConstrIdx = -1;
        m_selectedBuilding = World::Building_None;

        m_statusText = "Building construction started!";
        m_statusTextTimer = 2.0f;

        _snprintf(dbg, sizeof(dbg), "[GameScene] Flag placed at (%d,%d) -> building at (%d,%d), entrance offset (%d,%d)\n",
            tileX, tileY, buildX, buildY, entranceX, entranceY);
        OutputDebugStringA(dbg);
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
        if (m_transportJobManager) m_transportJobManager->MarkRoutesDirty();
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

        // Mark all footprint tiles on the Buildings layer
        auto markTile = [&](int tx, int ty) {
            if (tx < 0 || tx >= nodesW || ty < 0 || ty >= nodesH) return;
            World::Tile& bTile = buildingsLayer->GetTile(tx, ty);
            if (bTile.type == World::Tile_None) {
                bTile.atlasName = "Buildings";
                bTile.type = World::Decoration;
                bTile.regionIndex = buildingSpriteIdx;
                bTile.walkable = true;
                // Show construction tent/site sprite on the building footprint
                bTile.u0 = CONSTRUCTION_U0; bTile.v0 = CONSTRUCTION_V0;
                bTile.u1 = CONSTRUCTION_U1; bTile.v1 = CONSTRUCTION_V1;
            }
        };

        if (!footMask.empty()) {
            for (size_t i = 0; i < footMask.size(); ++i) {
                markTile(siteX + footOffX + footMask[i].first,
                         siteY + footOffY + footMask[i].second);
            }
        } else {
            for (int dy = 0; dy < footH; ++dy) {
                for (int dx = 0; dx < footW; ++dx) {
                    markTile(siteX + footOffX + dx, siteY + footOffY + dy);
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
            case World::CoalMine:     return "b_coalmine";
            case World::IronMine:     return "b_ironmine";
            case World::GoldMine:     return "b_goldmine";
            case World::IronSmelter:  return "b_ironsmelter";
            case World::GoldSmelter:  return "b_goldsmelter";
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
                bool uvMismatch = (tile.u0 != region->u0 || tile.v0 != region->v0 ||
                    tile.u1 != region->u1 || tile.v1 != region->v1);
                if (isConstruction || uvMismatch)
                { skipped++; continue; }

                // Determine building type from sprite name
                bool isBuildingSprite = region->isBuilding;
                World::BuildingType type = GetBuildingTypeFromSpriteName(spriteName);
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
                } else {
                    building = World::CreateBuilding(type, x, y, 0, m_map);
                    if (!building) { skipped++; continue; }
                    building->state = World::State_Finished;
                    building->connectedFlag = flag;
                    building->map = m_map;
                    flag->building = building;
                    flag->hasBuilding = true;
                    flag->pendingBuilding = World::Building_None;
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
                if (spriteIdx != 0xFFFFFFFF) {
                    const SpriteRegion* r = buildingsAtlas->GetRegion(spriteIdx);
                    if (r) {
                        // Get footprint dimensions from the atlas region
                        int footOffX = r->collOffX;
                        int footOffY = r->collOffY;
                        int footW = (int)r->collWidth;
                        int footH = (int)r->collHeight;
                        if (footW < 1) footW = 1;
                        if (footH < 1) footH = 1;
                        CoordinateSystem& coords = CoordinateSystem::GetInstance();
                        int nodesW = coords.GetNodesWidth();
                        int nodesH = coords.GetNodesHeight();
                        // Update all footprint tiles
                        for (int dy = 0; dy < footH; ++dy) {
                            for (int dx = 0; dx < footW; ++dx) {
                                int tx = site->x + footOffX + dx;
                                int ty = site->y + footOffY + dy;
                                if (tx >= 0 && tx < nodesW && ty >= 0 && ty < nodesH) {
                                    World::Tile& tile = buildingsLayer->GetTile(tx, ty);
                                    tile.atlasName = "Buildings";
                                    tile.type = World::Decoration;
                                    tile.regionIndex = (int)spriteIdx;
                                    tile.u0 = r->u0; tile.v0 = r->v0;
                                    tile.u1 = r->u1; tile.v1 = r->v1;
                                    tile.walkable = true;
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
            building->connectedFlag = flag;
            flag->building = building;
            flag->hasBuilding = true;
            flag->pendingBuilding = World::Building_None;

            if (m_economyManager) {
                m_economyManager->AddBuilding(building);
            }
        }

        m_statusText = "Building completed!";
        m_statusTextTimer = 2.0f;

//        OutputDebugStringA("[GameScene] ConfirmConstruction - building completed\n");
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
                            CoordinateSystem& coords = CoordinateSystem::GetInstance();
                            int nodesW = coords.GetNodesWidth();
                            int nodesH = coords.GetNodesHeight();
                            for (int dy = 0; dy < footH; ++dy) {
                                for (int dx = 0; dx < footW; ++dx) {
                                    int tx = buildX + footOffX + dx;
                                    int ty = buildY + footOffY + dy;
                                    if (tx >= 0 && tx < nodesW && ty >= 0 && ty < nodesH) {
                                        World::Tile& t = buildingsLayer->GetTile(tx, ty);
                                        t.atlasName = "";
                                        t.type = World::Tile_None;
                                        t.regionIndex = -1;
                                        t.walkable = false;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Remove the building from EconomyManager and delete the object
            if (flag->building) {
                m_objectLifecycleManager->ForceDeleteBuilding(flag->building);
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
                    char dbg[128];
                    _snprintf(dbg, sizeof(dbg), "[Road] Dir %d: (%d,%d) blocked by building\n", i, tx, ty);
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
                        road->id, m_roadStartX, m_roadStartY, endX, endY, (unsigned)road->tiles.size());
                    OutputDebugStringA(dbg);
                }

                SyncCarriersForFlag(startFlag);
                SyncCarriersForFlag(endFlag);
                if (m_transportJobManager) m_transportJobManager->MarkRoutesDirty();
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
            for (size_t t = 1; t + 1 < road->tiles.size(); ++t) {
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
            for (size_t t = 0; t <= (size_t)splitIdx; ++t)
                pathAX.push_back(road->tiles[t]);
            for (size_t t = (size_t)splitIdx; t < road->tiles.size(); ++t)
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
            if (road->tiles.size() < 2) continue;
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
                (unsigned)road->tiles.size());
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