#include "stdafx.h"
#include "GameScene.h"
#include "../Core/JobManager.h"
#include "BuildingPlacement.h"
#include "ConstructionVisualizer.h"
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

    static void AdjustEntranceForParity(bool buildingEvenY, int& entranceX, int entranceY);

    static World::SimulationConfig MakeGameSimConfig()
    {
        World::SimulationConfig cfg;
        cfg.enableTransport = true;
        cfg.enableEconomy = true;
        cfg.enableConstruction = true;
        cfg.enableProduction = true;
        cfg.enableWarehouse = true;
        cfg.enableWorkers = true;
        cfg.enableSettlement = true;
        cfg.enableTreeDepletion = true;
        cfg.enableConsumption = true;
        return cfg;
    }

    GameScene::GameScene()
        : SceneBase("Game")
        , m_simulationCore(MakeGameSimConfig())
		, m_buildingCommandHandler(NULL)
		, m_resourceCommandHandler(NULL)
        , m_eventBus(NULL)
        , m_commandBus(NULL)
        , m_transportJobManager(NULL)
        , m_transportController(NULL)
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
        , m_buildMenu(NULL)
        , m_roadMenu(NULL)
        , m_flagMenu(NULL)
        , m_flagMenuItemCount(0)
        , m_geologistMenu(NULL)
        , m_placementManager(NULL)
        , m_storehouseManager(NULL)
        , m_flagManager(NULL)
        , m_roadManager(NULL)
        , m_constructionManager(NULL)
        , m_constructionVisualizer(NULL)
        , m_objectLifecycleManager(NULL)
        , m_workerManager(NULL)

        , m_textManager(NULL)
        , m_frameCount(0)
        , m_bannerTargetX(1280.0f)

        , m_gameRenderer(NULL)
        , m_inputController(NULL)
        , m_geologistController(NULL)
        , m_uiController(NULL)
    {
    }

    GameScene::~GameScene()
    {
		// Clean up command handlers
      if (m_buildingCommandHandler) {
          delete m_buildingCommandHandler;
          m_buildingCommandHandler = NULL;
      }

      if (m_resourceCommandHandler) {
          delete m_resourceCommandHandler;
          m_resourceCommandHandler = NULL;
      }
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
        if (m_constructionVisualizer) {
            delete m_constructionVisualizer;
            m_constructionVisualizer = NULL;
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
        if (m_gameRenderer) {
            delete m_gameRenderer;
            m_gameRenderer = NULL;
        }
        if (m_geologistController) {
            delete m_geologistController;
            m_geologistController = NULL;
        }
        if (m_inputController) {
            delete m_inputController;
            m_inputController = NULL;
        }
        if (m_uiController) {
            delete m_uiController;
            m_uiController = NULL;
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

        // Bootstrap all systems (EventBus, managers, simulation, lifecycle)
        {
            WorldBootstrapCtx ctx = {
                m_eventBus, m_commandBus, m_entityManager,
                m_animalSystem, m_animalManager, m_wildlife,
                m_economyManager, m_carrierSystem, m_carrierManager,
                m_workerManager, m_aiSystem, m_flagManager,
                m_roadManager, m_transportJobManager, m_transportController,
                m_cargoManager,
                m_demandManager, m_storehouseManager, m_constructionManager,
                m_objectLifecycleManager, m_placementManager, m_constructionVisualizer,
                &m_roadController, &m_relinker
            };
            WorldBootstrap::SetupSystems(m_map, loadedFlagData, loadedRoadData, m_simulation, m_restorer, ctx);
        }

        // Wire placement + register event (needs GameScene 'this')
        m_placement.SetPlacementManager(m_placementManager);
        if (m_eventBus) {
            m_eventBus->Register(Core::Event_FlagPlaced, this);
        }
        OutputDebugStringA("[GameScene::Load] BuildingPlacementManager ready\n");

        // Initialize tile renderer
        OutputDebugStringA("[GameScene::Load] Creating TileRenderer\n");
        if (m_tileRenderer) {
            delete m_tileRenderer;
            m_tileRenderer = NULL;
        }
        m_tileRenderer = new TileRenderer(m_renderer, m_map->GetWidth(), m_map->GetHeight());
        // Initialize coordinate system for world↔node conversions (needed before first render)
        CoordinateSystem::GetInstance().Initialize(m_map->GetWidth(), m_map->GetHeight());
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
            m_buildMenu->SetLocalizationService(&m_localization);
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
            m_roadMenu->SetLocalizationService(&m_localization);
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

        // ─── Create UiController ──────────────────────────────────────
        {
            m_uiController = new UI::UiController(m_eventBus, &m_localization);
            m_uiController->SetMap(m_map);
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[GameScene::Load] UiController created\n");
            OutputDebugStringA(dbg);
        }

        // ─── Create InputController ────────────────────────────────────
        {
            m_inputController = new InputController(
                m_inputManager, *m_commandBus, m_eventBus,
                m_map, m_flagManager,
                &m_placement, &m_roadController,
                m_buildMenu, m_flagMenu
            );
            m_inputController->SetHost(this);
            m_inputController->SetStatusManager(&m_statusManager);
            m_roadController.SetPlacementController(&m_placement);
            m_roadController.SetStatusManager(&m_statusManager);
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[GameScene::Load] InputController created\n");
            OutputDebugStringA(dbg);
        }

        // ─── Create GeologistController ────────────────────────────────
        {
            m_geologistController = new GeologistController();
            m_geologistController->Initialize(m_map, &m_statusManager, this, m_geologistMenu,
                                              &m_localization, m_uiController->GetNotificationManager());
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[GameScene::Load] GeologistController created\n");
            OutputDebugStringA(dbg);
        }

        // ─── Cache bunner_info sprite from UI atlas ────────────────────
        {
            TextureRegistry& reg = TextureRegistry::instance();
            std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
            if (uiAtlas) {
                uint32_t bIdx = uiAtlas->GetIndex("bunner_info");
                if (bIdx != 0xFFFFFFFF) {
                    const SpriteRegion* r = uiAtlas->GetRegion(bIdx);
                    if (r) {
                        m_frameContext.overlay.bannerU0 = r->u0; m_frameContext.overlay.bannerV0 = r->v0;
                        m_frameContext.overlay.bannerU1 = r->u1; m_frameContext.overlay.bannerV1 = r->v1;
                        m_frameContext.overlay.bannerW = (float)r->width;
                        m_frameContext.overlay.bannerH = (float)r->height;
                        m_frameContext.overlay.bannerLoaded = true;
                        m_frameContext.overlay.bannerSlideX = 1280.0f;
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
                    m_frameContext.overlay.townHallPanelBgIdx = (int)panelIdx;
                    const SpriteRegion* r = uiAtlas->GetRegion(panelIdx);
                    if (r) {
                        m_frameContext.overlay.townHallPanelU0 = r->u0;
                        m_frameContext.overlay.townHallPanelV0 = r->v0;
                        m_frameContext.overlay.townHallPanelU1 = r->u1;
                        m_frameContext.overlay.townHallPanelV1 = r->v1;
                        m_frameContext.overlay.townHallPanelW = (float)r->width;
                        m_frameContext.overlay.townHallPanelH = (float)r->height;
                    }
                }
            }
        }

        // ─── Initialize map sprites (stumps, trees) ────────────────────
        if (m_map) {
            WorldBootstrap::InitializeMapSprites(m_map);
        }

        // ─── Fix construction sprite UV on existing tiles ────────────────
        if (m_constructionVisualizer) {
            m_constructionVisualizer->FixConstructionTilesUV();
        }

        // Restore any buildings placed in the editor from the Buildings layer
        {
            WorldRestorerContext restorerCtx;
            restorerCtx.map = m_map;
            restorerCtx.flags = m_flagManager;
            restorerCtx.economy = m_economyManager;
            restorerCtx.storehouse = m_storehouseManager;
            restorerCtx.transportJobs = m_transportJobManager;
            restorerCtx.construction = m_constructionManager;
            restorerCtx.carriers = m_carrierManager;
            restorerCtx.demand = m_demandManager;
            restorerCtx.relinker = &m_relinker;
            m_restorer.SetContext(restorerCtx);
            m_restorer.RestoreBuildingsFromLayer();
        }

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

            // Propagate warehouse flag to internal ConstructionManager
            // (SetupSystems → SimulationSystem::Initialize runs before
            //  RestoreBuildingsFromLayer, so the internal CM got NULL)
            World::Flag* whFlag = wh->connectedFlag;
            if (whFlag) {
                m_simulation.GetConstruction().GetManager()->SetWarehouseFlag(whFlag);
                char _wdbg[128];
                _snprintf(_wdbg, sizeof(_wdbg),
                    "[GameScene] Internal CM warehouseFlag set: flagId=%u\n",
                    whFlag->id);
                OutputDebugStringA(_wdbg);
            }
        }

        // ─── Create starting warehouse + carriers (only if none restored) ──
        if (m_flagManager && m_economyManager && !m_economyManager->GetWarehouse()) {
            WorldBootstrap::CreateStartingHQ(
                m_map, m_economyManager, m_flagManager,
                m_carrierManager, m_storehouseManager,
                m_transportJobManager, m_constructionManager,
                m_demandManager, m_relinker);

            // Also set warehouse flag on the internal ConstructionManager used by
            // SimulationSystem phases (m_extConstruction was redirected to the
            // ConstructionSystem's internal manager in Initialize).
            World::Flag* hq = m_economyManager->GetWarehouse()
                ? m_economyManager->GetWarehouse()->connectedFlag
                : NULL;
            if (hq) {
                m_simulation.GetConstruction().GetManager()->SetWarehouseFlag(hq);
            }

            OutputDebugStringA("[GameScene::Load] Warehouse + carriers created\n");
        }

        // ─── Create GameRenderer ──────────────────────────────────────
        {
            m_gameRenderer = new GameRenderer(
                m_tileRenderer, m_renderer, m_camera,
                m_map,
                m_textManager
            );
            OutputDebugStringA("[GameScene::Load] GameRenderer created\n");
        }

        // ─── Load world into SimulationCore ─────────────────────────
        if (m_map) {
            World::WorldModel coreWorld;
            coreWorld.width = static_cast<uint32_t>(m_map->GetWidth());
            coreWorld.height = static_cast<uint32_t>(m_map->GetHeight());
            m_simulationCore.LoadWorld(coreWorld);
            OutputDebugStringA("[GameScene::Load] SimulationCore world loaded\n");
        }

        // ─── Wire ProjectionSystem with camera ──────────────────────
        m_projectionSystem.SetCamera(m_camera);

        // ─── Wire up Presentation Systems (simulation → DTO bridge) ──
        m_terrainPresentationSystem.SetMap(m_map, m_map->GetWidth(), m_map->GetHeight());
        m_geologistOverlayPresentationSystem.SetMap(m_map);
        m_huntingSpotPresentationSystem.SetManagers(m_flagManager, m_map, &m_placement);
        m_confirmationMenuPresentationSystem.SetGeologistMenu(m_geologistMenu);
        m_menuPresentationSystem.SetMenus(m_buildMenu, m_flagMenu);
        m_townHallPresentationSystem.SetManagers(m_flagManager, m_economyManager);
        m_logisticsDebugPresentationSystem.SetManagers(m_flagManager, m_carrierManager);
        m_buildingPresentationSystem.SetSources(
            &m_legacyFlagSource,
            &m_legacyBuildingSource,
            &m_legacyBuildingSource   // implements both IBuildingSource and IConstructionSiteSource
        );

        // ─── Wire migration adapter sources (temporary — remove after Legacy*Source deleted) ───
        m_legacyBuildingSource.SetManagers(m_flagManager, m_constructionManager);
        m_legacyFlagSource.SetFlagManager(m_flagManager);
        m_simulationCoreBuildingSource.SetWorldModel(&m_simulationCore.GetWorld());
        m_simulationCoreWorkerSource.SetWorldModel(&m_simulationCore.GetWorld());

        m_inspector.SetSources(
            &m_legacyFlagSource,
            &m_legacyBuildingSource,
            &m_legacyBuildingSource   // implements IConstructionSiteSource
        );

        m_flagResourcePresentationSystem.SetFlagSource(&m_legacyFlagSource);
        m_wildlifePresentationSystem.SetWildlifeSystem(m_wildlife);
        m_groundResourcePresentationSystem.SetMap(m_map);
        m_buildingWorkerPresentation.SetSources(
            &m_legacyBuildingSource,  // IBuildingSource
            &m_legacyBuildingSource); // IConstructionSiteSource
        m_legacyCarrierSource.SetCarrierManager(m_carrierManager);
        m_carrierPresentation.SetCarrierSource(&m_legacyCarrierSource);
        m_placementPreviewPresentationSystem.SetPlacementController(&m_placement);
        m_roadPreviewPresentationSystem.SetControllers(&m_roadController, &m_placement);
        m_roadConnectionPresentationSystem.SetMap(m_map);
        m_resourceHudPresentationSystem.SetEconomyManager(m_economyManager);
        m_workSitePresentationSystem.SetEconomyManager(m_economyManager);

		// Initialize command handlers after managers are created
  m_buildingCommandHandler = new Handlers::BuildingCommandHandler(
      m_map,
      m_flagManager,
      m_constructionManager
  );

  m_resourceCommandHandler = new Handlers::ResourceCommandHandler(m_map);

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
        if (m_constructionVisualizer) {
            delete m_constructionVisualizer;
            m_constructionVisualizer = NULL;
        }
        if (m_objectLifecycleManager) {
        if (m_commandBus) m_commandBus->UnregisterAll(m_objectLifecycleManager);
        delete m_objectLifecycleManager;
        m_objectLifecycleManager = NULL;
    }

    if (m_eventBus) {
        m_eventBus->UnregisterAll(this);
    }

    if (m_flagManager) {
        delete m_flagManager;
        m_flagManager = NULL;
    }

    if (m_eventBus) {
        delete m_eventBus;
        m_eventBus = NULL;
    }
    if (m_commandBus) {
        delete m_commandBus;
        m_commandBus = NULL;
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
    m_loaded = false;
}

void GameScene::Update(float deltaTime)
  {
  //    OutputDebugStringA("GameScene::Update START\n");
      if (!m_loaded) {
          OutputDebugStringA("GameScene::Update Not loaded, returning\n");
          return;
      }


      ++m_frameCount;

      UpdateCamera(deltaTime);
      UpdateCursor();
      UpdateBanner(deltaTime);
      m_statusManager.Update(deltaTime);
      m_inputController->Tick(deltaTime);
      m_inputController->HandleGamepadInput();
      m_inputController->HandleInput();

      //  GeologistController
      if (m_geologistController) {
          m_geologistController->Update(deltaTime);
          m_geologistController->FillFrameContext(m_frameContext.overlay);
      }

      //  UiController
      if (m_uiController) {
          m_uiController->Update(deltaTime);
          m_uiController->FillFrameContext(m_frameContext.ui);
      }

      //  Sync InputController → FrameContext
      if (m_inputController) {
          m_inputController->FillFrameContext(m_frameContext.input);
      }

      //  StatusManager resolves and writes statusText to FrameContext
      m_statusManager.FillFrameContext(m_frameContext.input, &m_localization);

        //  SimulationCore (new architecture — ticks independently)
        m_simulationCore.Tick();

        //  Simulation (legacy — drives presentation during migration)
        m_simulation.Update(deltaTime);

      // Build render snapshot into a fresh frame, then swap (double-buffer pattern)
      {
          RenderFrame next;
          m_terrainPresentationSystem.BuildRenderFrame(next);
          m_buildingPresentationSystem.BuildRenderFrame(next);
          // Suppress cursor during flag placement preview (replaced by preview sprite).
          m_cursorPresentationSystem.SetSuppressed(
              m_placement.GetState() == PLACESTATE_PLACE_FLAG && !m_placement.IsIdle());
          m_cursorPresentationSystem.BuildRenderFrame(m_frameContext, next.cursor);
          m_flagResourcePresentationSystem.BuildRenderFrame(next.flagResources);
          m_wildlifePresentationSystem.BuildRenderFrame(next.wildlife);
          m_groundResourcePresentationSystem.BuildRenderFrame(next);
          m_buildingWorkerPresentation.BuildRenderFrame(next);
          m_carrierPresentation.BuildRenderFrame(next);
          m_placementPreviewPresentationSystem.BuildRenderFrame(m_frameContext, next.preview);
          m_roadPreviewPresentationSystem.BuildRenderFrame(next.roadPreview);
            m_geologistOverlayPresentationSystem.BuildRenderFrame(m_frameContext, next.overlays);
            m_huntingSpotPresentationSystem.BuildRenderFrame(m_frameContext, next.overlays);
            m_workSitePresentationSystem.BuildRenderFrame(next.workSites);
            m_confirmationMenuPresentationSystem.BuildRenderFrame(next.ui);
            m_menuPresentationSystem.BuildRenderFrame(next.ui.menuPanel);
            m_notificationPresentationSystem.BuildRenderFrame(m_frameContext.ui, next.ui);
            m_townHallPresentationSystem.BuildRenderFrame(m_frameContext, next.ui.townHallPanel, next.highlights);
            m_resourceHudPresentationSystem.BuildRenderFrame(m_frameContext, next.ui.resourceHud);
            m_bannerPresentationSystem.BuildRenderFrame(m_frameContext, next.ui.banner);
            m_logisticsDebugPresentationSystem.BuildRenderFrame(m_frameContext, next.debugLabels);
            m_inspector.BuildDebugLabels(next.debugLabels);
            m_roadConnectionPresentationSystem.BuildRenderFrame(next.roadConnections);
            m_projectionSystem.Project(next);
          next.frameId = m_frameCount;
           std::swap(m_renderFrame, next);
      }

      // Flush any pending deletions at end of frame
      if (m_objectLifecycleManager) {
          m_objectLifecycleManager->FlushDeletions();
      }
  }

void GameScene::UpdateCamera(float dt)
{
    if (!m_inputController->IsMenuActive() && !m_inputController->IsRoadMenuActive() && !m_inputController->IsFlagMenuActive() && !m_inputController->IsGeologistMenuActive() && !m_inputController->IsTownHallPanelOpen() && m_camera && m_inputManager) {
        Input::Gamepad* gamepad = m_inputManager->GetGamepad();
        if (gamepad) {
            float moveSpeed = 2000.0f * dt;
            float stickX, stickY;
            gamepad->GetLeftStick(stickX, stickY);
            if (fabsf(stickX) > 0.1f || fabsf(stickY) > 0.1f) {
                m_camera->Move(stickX * moveSpeed, stickY * moveSpeed);
            }
            float rightX, rightY;
            gamepad->GetRightStick(rightX, rightY);
            if (fabsf(rightY) > 0.1f) {
                m_camera->Zoom(rightY * 0.3f * dt);
            }
        }
    }
    if (m_camera) {
        m_camera->Update(dt);
    }
}

void GameScene::UpdateBanner(float dt)
{
    if (m_frameContext.overlay.bannerLoaded) {
        if (!m_statusManager.IsActive()) {
            m_bannerTargetX = 1280.0f;
        } else {
            m_bannerTargetX = 1280.0f - m_frameContext.overlay.bannerW;
        }
        float speed = 1200.0f;
        if (m_frameContext.overlay.bannerSlideX > m_bannerTargetX) {
            m_frameContext.overlay.bannerSlideX -= speed * dt;
            if (m_frameContext.overlay.bannerSlideX < m_bannerTargetX) m_frameContext.overlay.bannerSlideX = m_bannerTargetX;
        } else if (m_frameContext.overlay.bannerSlideX < m_bannerTargetX) {
            m_frameContext.overlay.bannerSlideX += speed * dt;
            if (m_frameContext.overlay.bannerSlideX > m_bannerTargetX) m_frameContext.overlay.bannerSlideX = m_bannerTargetX;
        }
    }
}

void GameScene::HandleInput()
{
    m_inputController->HandleInput();
}

void GameScene::Render(Graphics::RenderQueue* renderQueue)
{
    if (!m_loaded || !m_gameRenderer) {
        OutputDebugStringA("[GameScene::Render] Not ready, returning\n");
        return;
    }

    m_gameRenderer->Render(renderQueue, m_renderFrame);
}
    void GameScene::UpdateCursor()
    {
        if (!m_camera || !m_map) return;

        float worldCX, worldCY;
        m_camera->GetWorldCenter(worldCX, worldCY);

        int cursorX, cursorY;
        CoordinateSystem::GetInstance().WorldToNodeTile(worldCX, worldCY, cursorX, cursorY);

        int nodesW = CoordinateSystem::GetInstance().GetNodesWidth();
        int nodesH = CoordinateSystem::GetInstance().GetNodesHeight();
        if (cursorX < 0) cursorX = 0;
        if (cursorX >= nodesW) cursorX = nodesW - 1;
        if (cursorY < 0) cursorY = 0;
        if (cursorY >= nodesH) cursorY = nodesH - 1;

        m_inputController->SetCursorTile(cursorX, cursorY);

        // Town hall hover detection
        bool onTownHall = false;
        World::TileLayer* buildingsLayer = m_map->GetLayer(World::Buildings);
        if (buildingsLayer) {
            int bx = cursorX;
            int by = cursorY;
            if (bx >= 0 && bx < buildingsLayer->GetWidth() && by >= 0 && by < buildingsLayer->GetHeight()) {
                const World::Tile& tile = buildingsLayer->GetTile(bx, by);
                if (tile.atlasName == "Buildings" && tile.regionIndex >= 0) {
                    TextureRegistry& reg = TextureRegistry::instance();
                    std::tr1::shared_ptr<SpriteAtlas> atlas = reg.getAtlas("Buildings");
                    if (atlas) {
                        const SpriteRegion* region = atlas->GetRegion(tile.regionIndex);
                        if (region) {
                            World::BuildingType type = GetBuildingTypeFromSpriteName(region->name);
                            if (type == World::Storehouse) onTownHall = true;
                        }
                    }
                }
            }
        }
        m_inputController->SetCursorOnTownHall(onTownHall);

        // Auto-update road preview during road building
        if (m_placement.GetState() == PLACESTATE_PLACE_ROAD) {
            m_roadController.UpdatePreview(cursorX, cursorY);
        }
    }

    void GameScene::InitBuildMenu()
    {
        if (!MenuBootstrap::SetupBuildMenu(m_buildMenu, m_renderer)) return;

        // Initialize resource HUD icons (r_* sprites from Icon atlas)
        TextureRegistry& reg = TextureRegistry::instance();
        std::tr1::shared_ptr<SpriteAtlas> iconAtlas = reg.getAtlas("Icon");
        if (iconAtlas) {
            m_frameContext.overlay.resourceHud[0].type = World::ResourceType_Wood;      m_frameContext.overlay.resourceHud[0].iconName = "r_wood";
            m_frameContext.overlay.resourceHud[1].type = World::ResourceType_Stone;     m_frameContext.overlay.resourceHud[1].iconName = "r_stone";
            m_frameContext.overlay.resourceHud[2].type = World::ResourceType_Planks;    m_frameContext.overlay.resourceHud[2].iconName = "r_planks";
            m_frameContext.overlay.resourceHud[3].type = World::ResourceType_Fish;      m_frameContext.overlay.resourceHud[3].iconName = "r_fish";
            m_frameContext.overlay.resourceHud[4].type = World::ResourceType_Meat;      m_frameContext.overlay.resourceHud[4].iconName = "r_meat";
            m_frameContext.overlay.resourceHud[5].type = World::ResourceType_Bread;     m_frameContext.overlay.resourceHud[5].iconName = "r_bread";
            m_frameContext.overlay.resourceHud[6].type = World::ResourceType_Coal;      m_frameContext.overlay.resourceHud[6].iconName = "r_coal";
            m_frameContext.overlay.resourceHud[7].type = World::ResourceType_IronOre;   m_frameContext.overlay.resourceHud[7].iconName = "r_ironore";
            m_frameContext.overlay.resourceHud[8].type = World::ResourceType_GoldOre;   m_frameContext.overlay.resourceHud[8].iconName = "r_goldore";
            m_frameContext.overlay.resourceHud[9].type = World::ResourceType_IronBar;   m_frameContext.overlay.resourceHud[9].iconName = "r_ironbar";
            m_frameContext.overlay.resourceHud[10].type = World::ResourceType_GoldBar;  m_frameContext.overlay.resourceHud[10].iconName = "r_goldbar";

            for (int i = 0; i < OverlayFrameState::RESOURCE_HUD_COUNT; ++i) {
                if (m_frameContext.overlay.resourceHud[i].iconName) {
                    uint32_t idx = iconAtlas->GetIndex(m_frameContext.overlay.resourceHud[i].iconName);
                    m_frameContext.overlay.resourceHud[i].iconIdx = (idx != 0xFFFFFFFF) ? (int)idx : -1;
                }
            }
            m_frameContext.overlay.resourceHudLoaded = true;
            OutputDebugStringA("[GameScene] Resource HUD initialized\n");
        }
    }

    void GameScene::InitRoadMenu()
    {
        MenuBootstrap::SetupRoadMenu(m_roadMenu, m_renderer);
    }

    void GameScene::InitFlagMenu()
    {
        MenuBootstrap::SetupFlagMenu(m_flagMenu, m_flagMenuItemData, 3, m_flagMenuItemCount);
    }

    void GameScene::InitGeologistMenu()
    {
        MenuBootstrap::SetupGeologistMenu(m_geologistMenu);
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
        const char* spriteNamePtr = BuildingPlacementManager::GetBuildingSpriteName(type);
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
                    BuildingPlacementManager::GetBuildingSpriteName(type));
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
        World::ResourceType requiredRes = BuildingPlacementManager::GetResourceTypeForMine(type);
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



    void GameScene::OnEvent(Core::EventType type, void* data)
    {
        if (type == Core::Event_FlagPlaced) {
            Core::FlagPlacedData* fd = static_cast<Core::FlagPlacedData*>(data);
            if (m_flagManager && m_constructionVisualizer && fd->buildingType != World::Building_None) {
                World::Flag* flag = m_flagManager->GetFlagById(fd->flagId);
                if (flag) {
                    m_constructionVisualizer->SetupConstructionSiteTiles(
                        flag, fd->buildX, fd->buildY,
                        static_cast<World::BuildingType>(fd->buildingType));
                }
            }
        }
    }

    World::BuildingType GameScene::GetBuildingTypeFromSpriteName(const std::string& name) const
    {
        std::string key = name;
        if (key.compare(0, 2, "b_") == 0)
            key = key.substr(2);
        if (key.compare(0, 3, "ib_") == 0)
            key = key.substr(3);

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
            { "forester",     World::Forester },
            { "stonemason",   World::Stonemason },
            { "well",         World::Well },
            { "barracks",     World::Barracks },
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



    void GameScene::ConfirmDeleteFlag(World::Flag* flag)
    {
        if (!flag) return;

        char dbg[256];
        _snprintf(dbg, sizeof(dbg), "[GameScene] ConfirmDeleteFlag at (%d,%d) flag type=%d\n",
            flag->pos.x, flag->pos.y, (int)flag->type);
        OutputDebugStringA(dbg);

        if (flag->type == World::FLAG_WAREHOUSE) {
            m_statusManager.SetStatus(UI::MSG_CANNOT_DELETE_TOWN_HALL, UI::UiFormatArgs(), 2.0f);
            return;
        }

        // Visual: clear building footprint from Buildings layer
        if (flag->building || flag->pendingBuilding != World::Building_None) {
            const char* buildingName = NULL;
            int buildX = flag->pos.x;
            int buildY = flag->pos.y;

            if (flag->building) {
                buildingName = BuildingPlacementManager::GetBuildingSpriteName(flag->building->type);
                buildX = flag->building->pos.x;
                buildY = flag->building->pos.y;
            } else {
                buildingName = BuildingPlacementManager::GetBuildingSpriteName(flag->pendingBuilding);
                std::string nameStr = buildingName ? buildingName : "";
                if (!nameStr.empty()) {
                    if (nameStr.compare(0, 3, "ib_") == 0)
                        nameStr = nameStr.substr(3);
                    int entranceX = 0, entranceY = 0;
                    GetEntranceOffset(nameStr, entranceX, entranceY);
                    buildY = flag->pos.y - entranceY;
                    bool buildingEvenY = (buildY % 2 == 0);
                    AdjustEntranceForParity(buildingEvenY, entranceX, entranceY);
                    buildX = flag->pos.x - entranceX;
                }
            }

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
                            World::BuildingType dtype = (flag->building) ? flag->building->type : flag->pendingBuilding;
                            bool is2x2 = (dtype == World::Stonemason || dtype == World::Sawmill || dtype == World::Farm || dtype == World::Mill);
                            if (is2x2 && (footW != 2 || footH != 2)) {
                                footW = 2;
                                footH = 2;
                            }
                            if (m_constructionVisualizer) {
                                m_constructionVisualizer->ClearBuildingFootprint(buildX + footOffX, buildY + footOffY, footW, footH);
                            }
                        }
                    }
                }
            }

            // Post commands — pipeline handles cleanup
            if (flag->building && m_commandBus) {
                Core::DeleteBuildingCmd bd;
                bd.flagId = flag->id;
                m_commandBus->Post(Core::Cmd_DeleteBuilding, bd);
            }

            if (flag->pendingBuilding != World::Building_None && m_commandBus && m_constructionManager) {
                World::ConstructionSite* site = m_constructionManager->GetSiteAt(buildX, buildY);
                if (site) {
                    Core::RemoveConstructionSiteCmd rd;
                    rd.siteId = site->id;
                    m_commandBus->Post(Core::Cmd_RemoveConstructionSite, rd);
                }
            }
        }

        if (m_commandBus) {
            Core::DeleteFlagCmd dfd;
            dfd.flagId = flag->id;
            m_commandBus->Post(Core::Cmd_DeleteFlag, dfd);
        }

        m_statusManager.SetStatus(UI::MSG_BUILDING_AND_FLAG_DELETED, UI::UiFormatArgs(), 2.0f);

        _snprintf(dbg, sizeof(dbg), "[GameScene] ConfirmDeleteFlag done at (%d,%d)\n", flag->pos.x, flag->pos.y);
        OutputDebugStringA(dbg);
    }

    // ─── Road building delegated to m_roadController

    // --- BFS road linking delegated to m_relinker (initialized in Load)

    void GameScene::SetGeologistMenuActive(bool active)
    {
        if (m_inputController) m_inputController->SetGeologistMenuActive(active);
    }

    void GameScene::CancelGeologist()
    {
        if (m_geologistController) m_geologistController->Cancel();
    }

    // IInputHost: delete a flag (with building) at given position
    void GameScene::DeleteFlagAt(int tileX, int tileY)
    {
        if (!m_flagManager) return;
        World::Flag* flag = m_flagManager->GetFlagAt(tileX, tileY);
        if (flag) {
            ConfirmDeleteFlag(flag);
        }
    }

    void GameScene::OnMountainTileAction(int tileX, int tileY)
    {
        if (m_geologistController) m_geologistController->OnTileAction(tileX, tileY);
    }

    void GameScene::InspectAt(int tileX, int tileY)
    {
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        float wx, wy;
        coords.NodeTileToWorld(tileX, tileY, wx, wy);
        m_inspector.OnClick(wx, wy);
    }

} // namespace Scene
