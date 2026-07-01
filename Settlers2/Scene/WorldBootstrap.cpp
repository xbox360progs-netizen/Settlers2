#include "stdafx.h"
#include "WorldBootstrap.h"
#include "WorldRestorer.h"
#include <queue>
#include "../Core/EventBus.h"
#include "../Core/CommandBus.h"
#include "../Core/JobManager.h"
#include "../World/EntityManager.h"
#include "../World/AnimalManager.h"
#include "../World/Systems/AnimalSystem.h"
#include "../World/WildlifeSystem.h"
#include "../World/CarrierManager.h"
#include "../World/Systems/CarrierSystem.h"
#include "../World/WorkerManager.h"
#include "../World/FlagManager.h"
#include "../World/RoadManager.h"
#include "../World/TransportController.h"
#include "../World/TransportJobManager.h"
#include "../World/CargoManager.h"
#include "../World/DemandManager.h"
#include "../World/StorehouseManager.h"
#include "../World/ConstructionManager.h"
#include "../World/ObjectLifecycleManager.h"
#include "../World/Systems/RoadNetworkRelinker.h"
#include "../World/Systems/SimulationSystem.h"
#include "../World/Map.h"
#include "../World/Components/Building.h"
#include "../Logic/EconomyManager.h"
#include "../Logic/AISystem.h"
#include "../Graphics/SpriteAtlas.h"
#include "../Graphics/TextureRegistry.h"
#include "../World/Warehouse.h"
#include "BuildingPlacement.h"
#include "ConstructionVisualizer.h"
#include "RoadController.h"
#include "PlacementController.h"

using namespace Scene;

namespace WorldBootstrap {

void SetupSystems(World::Map* map,
                  const std::vector<World::FlagData>& flagData,
                  const std::vector<World::RoadData>& roadData,
                  World::SimulationSystem& simulation,
                   Scene::WorldRestorer& restorer,
                  WorldBootstrapCtx& ctx)
{
    (void)flagData;
    (void)roadData;

    // ConstructionVisualizer
    ctx.visualizer = new ConstructionVisualizer(map);
    OutputDebugStringA("[WorldBootstrap] ConstructionVisualizer ready\n");

    // EventBus + CommandBus
    OutputDebugStringA("[WorldBootstrap] Creating EventBus\n");
    ctx.eventBus = new Core::EventBus();
    ctx.commandBus = new Core::CommandBus();
    ctx.commandBus->SetEventBus(ctx.eventBus);
    OutputDebugStringA("[WorldBootstrap] EventBus + CommandBus ready\n");

    // ECS + wildlife
    OutputDebugStringA("[WorldBootstrap] Creating ECS wildlife\n");
    ctx.entityManager = new World::EntityManager();
    ctx.animalSystem = new World::AnimalSystem(ctx.entityManager, map);
    ctx.animalManager = new World::AnimalManager(ctx.entityManager, ctx.animalSystem);
    ctx.animalManager->Init(&map->GetHabitatRegistry());
    ctx.wildlife = new World::WildlifeSystem(map, ctx.animalManager, ctx.animalSystem);
    map->SetWildlifeSystem(ctx.wildlife);
    OutputDebugStringA("[WorldBootstrap] ECS wildlife ready\n");

    // Economy
    OutputDebugStringA("[WorldBootstrap] Creating EconomyManager\n");
    ctx.economy = new Logic::EconomyManager();
    map->SetResourceRegistry(&ctx.economy->GetRegistry());
    map->GenerateWildlife();

    // Backfill tree resource nodes
    {
        World::TileLayer* objLayer = map->GetLayer(World::Objects);
        int count = 0;
        if (objLayer) {
            for (int y = 0; y < objLayer->GetHeight(); ++y) {
                for (int x = 0; x < objLayer->GetWidth(); ++x) {
                    const World::Tile& tile = objLayer->GetTile(x, y);
                    World::ResourceNode& rn = map->GetResourceNode(x, y);
                    if (rn.type != World::ResourceType_None) continue;
                    World::ResourceType rt = World::TileTypeToResourceType(tile.type);
                    if (rt != World::ResourceType_None) {
                        rn.type = rt;
                        rn.amount = World::TreeState_Mature;
                        rn.isVisible = true;
                        ctx.economy->GetRegistry().RegisterWorldResource(rt, x, y);
                        count++;
                    }
                }
            }
        }
        if (count > 0) {
            char dbg[128];
            _snprintf(dbg, sizeof(dbg), "[WorldBootstrap] Backfilled %d tree resource nodes\n", count);
            OutputDebugStringA(dbg);
        }
    }

    // Assign ore deposits to mountains
    {
        WorldRestorerContext oreCtx;
        oreCtx.map = map;
        oreCtx.economy = ctx.economy;
        restorer.SetContext(oreCtx);
        restorer.AssignOreDepositsToMountains();
    }

    OutputDebugStringA("[WorldBootstrap] EconomyManager ready\n");

    // Carrier system + manager
    OutputDebugStringA("[WorldBootstrap] Creating CarrierSystem\n");
    ctx.carrierSystem = new World::CarrierSystem(ctx.entityManager);
    OutputDebugStringA("[WorldBootstrap] Creating CarrierManager\n");
    ctx.carrierManager = new World::CarrierManager();
    ctx.carrierManager->SetCarrierSystem(ctx.carrierSystem);
    OutputDebugStringA("[WorldBootstrap] CarrierManager ready\n");

    // WorkerManager
    OutputDebugStringA("[WorldBootstrap] Creating WorkerManager\n");
    ctx.workerManager = new World::WorkerManager();
    OutputDebugStringA("[WorldBootstrap] WorkerManager ready\n");

    // AI
    OutputDebugStringA("[WorldBootstrap] Creating AISystem\n");
    ctx.aiSystem = new Logic::AISystem(0, map, ctx.economy);
    OutputDebugStringA("[WorldBootstrap] AISystem ready\n");

    // FlagManager
    OutputDebugStringA("[WorldBootstrap] Creating FlagManager\n");
    World::FlagManager* flagMgr = new World::FlagManager();
    if (!flagData.empty()) {
        flagMgr->LoadFromData(flagData);
        char dbg[256];
        _snprintf(dbg, sizeof(dbg), "[WorldBootstrap] Loaded %u flags from save\n", (unsigned)flagData.size());
        OutputDebugStringA(dbg);
        for (size_t fi = 0; fi < flagMgr->GetCount(); ++fi) {
            World::Flag* ff = flagMgr->GetFlag(fi);
            if (ff) {
                _snprintf(dbg, sizeof(dbg), "[FlagLoaded] idx=%zu id=%u type=%d pos=(%d,%d) handle=(%u,%u) hasBuilding=%d\n",
                    fi, ff->id, (int)ff->type, ff->pos.x, ff->pos.y,
                    ff->handle.index, ff->handle.generation, (int)ff->hasBuilding);
                OutputDebugStringA(dbg);
            }
        }
    }
    ctx.flagManager = flagMgr;

    // RoadManager
    OutputDebugStringA("[WorldBootstrap] Creating RoadManager\n");
    World::RoadManager* roadMgr = new World::RoadManager();
    roadMgr->SetFlagManager(flagMgr);

    if (!roadData.empty()) {
        roadMgr->LoadFromData(roadData, flagMgr);
        char dbg[256];
        _snprintf(dbg, sizeof(dbg), "[WorldBootstrap] Loaded %u roads from save\n", (unsigned)roadData.size());
        OutputDebugStringA(dbg);
    } else if (flagMgr->GetCount() > 0) {
        World::TileLayer* roadsLayer = map ? map->GetLayer(World::Roads) : NULL;
        if (roadsLayer) {
            int rw = roadsLayer->GetWidth();
            int rh = roadsLayer->GetHeight();
            int roadsCreated = 0;
            for (size_t fi = 0; fi < flagMgr->GetCount(); ++fi) {
                World::Flag* f = flagMgr->GetFlag(fi);
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
                    World::Flag* other = (cx == f->pos.x && cy == f->pos.y) ? NULL : flagMgr->GetFlagAt(cx, cy);
                    if (other) {
                        if (!roadMgr->GetRoadBetween(f, other)) {
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
                            roadMgr->CreateRoad(f, other, tilePath);
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
                _snprintf(dbg, sizeof(dbg), "[WorldBootstrap] Reconstructed %d roads from road tile BFS\n", roadsCreated);
                OutputDebugStringA(dbg);
            }
        }
    }
    ctx.roadManager = roadMgr;

    // Wire flag manager + road manager to carrier
    ctx.carrierManager->SetFlagManager(flagMgr);
    ctx.carrierManager->SetRoadManager(roadMgr);

    // Wire economy
    ctx.economy->SetFlagManager(flagMgr);
    ctx.economy->SetRoadManager(roadMgr);

    // TransportJobManager
    OutputDebugStringA("[WorldBootstrap] Creating TransportJobManager\n");
    World::TransportJobManager* tjm = new World::TransportJobManager();
    tjm->SetFlagManager(flagMgr);
    tjm->SetRoadManager(roadMgr);
    tjm->SetCarrierManager(ctx.carrierManager);
    if (ctx.economy && ctx.economy->GetWarehouse()) {
        tjm->SetWarehouse(ctx.economy->GetWarehouse());
    }
    ctx.carrierManager->SetJobManager(tjm);
    ctx.transportJobs = tjm;
    OutputDebugStringA("[WorldBootstrap] TransportJobManager ready\n");

    // CargoManager + DemandManager
    OutputDebugStringA("[WorldBootstrap] Creating CargoManager\n");
    ctx.cargo = new World::CargoManager();
    OutputDebugStringA("[WorldBootstrap] CargoManager ready\n");
    ctx.demand = new World::DemandManager();
    ctx.demand->SetFlagManager(flagMgr);
    OutputDebugStringA("[WorldBootstrap] DemandManager ready\n");

    // TransportController (Phase 7)
    {
        OutputDebugStringA("[WorldBootstrap] Creating TransportController\n");
        World::TransportController* tc = new World::TransportController();
        tc->SetRoadManager(roadMgr);
        tc->SetFlagManager(flagMgr);
        tc->SetCarrierManager(ctx.carrierManager);
        tc->SetCargoManager(ctx.cargo);
        tc->SetDemandManager(ctx.demand);
        ctx.demand->SetTransportController(tc);
        ctx.transportController = tc;
        OutputDebugStringA("[WorldBootstrap] TransportController ready\n");
    }

    if (map) {
        map->SetCargoManager(ctx.cargo);
        map->SetDemandManager(ctx.demand);
    }

    // StorehouseManager
    OutputDebugStringA("[WorldBootstrap] Creating StorehouseManager\n");
    ctx.storehouse = new World::StorehouseManager();
    ctx.storehouse->Init();
    if (ctx.economy) {
        ctx.economy->SetStorehouseManager(ctx.storehouse);
    }
    if (ctx.cargo) {
        ctx.cargo->SetStorehouseManager(ctx.storehouse);
    }
    OutputDebugStringA("[WorldBootstrap] StorehouseManager ready\n");

    // ConstructionManager
    OutputDebugStringA("[WorldBootstrap] Creating ConstructionManager\n");
    World::ConstructionManager* cm = new World::ConstructionManager();
    cm->SetFlagManager(flagMgr);
    cm->SetRoadManager(roadMgr);
    if (ctx.economy && ctx.economy->GetWarehouse() && ctx.economy->GetWarehouse()->connectedFlag) {
        cm->SetWarehouseFlag(ctx.economy->GetWarehouse()->connectedFlag);
        ctx.carrierManager->SetWarehouseFlag(ctx.economy->GetWarehouse()->connectedFlag);
    }
    if (ctx.demand) {
        cm->SetDemandManager(ctx.demand);
        ctx.carrierManager->SetDemandManager(ctx.demand);
    }
    if (ctx.cargo) {
        ctx.carrierManager->SetCargoManager(ctx.cargo);
        if (ctx.economy) {
            ctx.economy->SetCargoManager(ctx.cargo);
        }
    }
    if (ctx.workerManager) {
        ctx.workerManager->SetRoadManager(roadMgr);
    }
    ctx.construction = cm;
    OutputDebugStringA("[WorldBootstrap] ConstructionManager ready\n");

    // Relinker + RoadController
    ctx.relinker->SetManagers(map, flagMgr, roadMgr, ctx.carrierManager);
    ctx.roadController->SetExternalManagers(
        map, flagMgr, roadMgr, ctx.carrierManager,
        ctx.eventBus, ctx.lifecycle, cm);
    ctx.roadController->SetRelinker(ctx.relinker);

    // SimulationSystem
    {
        simulation.SetExternalManagers(
            ctx.construction,
            ctx.economy,
            ctx.carrierManager,
            ctx.carrierSystem,
            ctx.workerManager,
            ctx.transportJobs,
            ctx.cargo,
            ctx.demand,
            ctx.storehouse,
            ctx.transportController);

        World::Flag* whFlag = NULL;
        if (ctx.economy && ctx.economy->GetWarehouse()) {
            whFlag = ctx.economy->GetWarehouse()->connectedFlag;
        }
        simulation.Initialize(
            map,
            ctx.entityManager,
            ctx.flagManager,
            ctx.roadManager,
            whFlag,
            ctx.economy ? ctx.economy->GetWarehouse() : NULL,
            ctx.eventBus,
            ctx.commandBus);

        // Set up JobManager for parallel AI planning
        {
            JobManager* jm = new JobManager();
            int processors[] = { 1, 2 };
            jm->Initialize(2, processors);
            simulation.SetJobManager(jm);
            simulation.SetAISystem(ctx.aiSystem);
            OutputDebugStringA("[WorldBootstrap] JobManager passed to SimulationSystem\n");
        }

        // Wire up legacy EconomyManager pointers into SimulationSystem
        World::EconomySystem& ecoSys = simulation.GetEconomy();
        ecoSys.SetFlagManager(ctx.flagManager);
        ecoSys.SetRoadManager(ctx.roadManager);

        {
            char dbg[256];
            _snprintf(dbg, sizeof(dbg),
                "[WorldBootstrap] SimulationSystem initialized: construction=%d economy=%d transport=%d workforce=%d\n",
                ctx.construction ? (int)ctx.construction->GetCount() : 0,
                ctx.economy ? ctx.economy->GetBuildingCount() : 0,
                ctx.carrierManager ? ctx.carrierManager->GetCarrierCount() : 0,
                ctx.workerManager ? ctx.workerManager->GetActiveCount() : 0);
            OutputDebugStringA(dbg);
        }
    }

    // ObjectLifecycleManager
    ctx.lifecycle = new World::ObjectLifecycleManager(
        flagMgr, roadMgr, ctx.carrierManager, ctx.cargo,
        tjm, cm, ctx.economy, map);
    if (ctx.lifecycle) {
        ctx.lifecycle->SetEventBus(ctx.eventBus);
        ctx.commandBus->Register(Core::Cmd_DeleteFlag, ctx.lifecycle);
        ctx.commandBus->Register(Core::Cmd_DeleteBuilding, ctx.lifecycle);
    }
    OutputDebugStringA("[WorldBootstrap] ObjectLifecycleManager ready\n");

    // BuildingPlacementManager
    ctx.placement = new BuildingPlacementManager(
        map, flagMgr, roadMgr, ctx.carrierManager,
        ctx.economy, ctx.demand);
    OutputDebugStringA("[WorldBootstrap] BuildingPlacementManager ready\n");
}

void WorldBootstrap::InitializeMapSprites(World::Map* map)
{
    TextureRegistry& reg = TextureRegistry::instance();

    // Stump sprites
    {
        std::tr1::shared_ptr<SpriteAtlas> maptiles = reg.getAtlas("maptiles");
        if (maptiles) {
            uint32_t s1 = maptiles->GetIndex("stump_01");
            uint32_t s2 = maptiles->GetIndex("stump_02");
            uint32_t s3 = maptiles->GetIndex("stump_03");
            const SpriteRegion* r1 = (s1 != 0xFFFFFFFF) ? maptiles->GetRegion(s1) : NULL;
            const SpriteRegion* r2 = (s2 != 0xFFFFFFFF) ? maptiles->GetRegion(s2) : NULL;
            const SpriteRegion* r3 = (s3 != 0xFFFFFFFF) ? maptiles->GetRegion(s3) : NULL;
            World::Map::SpriteData d1 = { (int)s1, r1 ? r1->u0 : 0, r1 ? r1->v0 : 0, r1 ? r1->u1 : 1, r1 ? r1->v1 : 1 };
            World::Map::SpriteData d2 = { (int)s2, r2 ? r2->u0 : 0, r2 ? r2->v0 : 0, r2 ? r2->u1 : 1, r2 ? r2->v1 : 1 };
            World::Map::SpriteData d3 = { (int)s3, r3 ? r3->u0 : 0, r3 ? r3->v0 : 0, r3 ? r3->u1 : 1, r3 ? r3->v1 : 1 };
            map->SetStumpSprites(d1, d2, d3);
            OutputDebugStringA("[WorldBootstrap] Stump sprites initialized\n");
        }
    }

    // Tree sprites
    {
        std::tr1::shared_ptr<SpriteAtlas> maptiles = reg.getAtlas("maptiles");
        if (maptiles) {
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
                        map->AddTreeSprite(sd);
                        ++loaded;
                    }
                }
            }
            char dbg[128];
            _snprintf(dbg, sizeof(dbg), "[WorldBootstrap] Tree sprites initialized: %d loaded\n", loaded);
            OutputDebugStringA(dbg);
        }
    }
}

void WorldBootstrap::CreateStartingHQ(
    World::Map* map,
    Logic::EconomyManager* economy,
    World::FlagManager* flagManager,
    World::CarrierManager* carrierManager,
    World::StorehouseManager* storehouse,
    World::TransportJobManager* transportJobs,
    World::ConstructionManager* construction,
    World::DemandManager* demand,
    World::RoadNetworkRelinker& relinker)
{
    int hqFlagX = 10, hqFlagY = 10;
    int hqBuildX = 10, hqBuildY = 8;

    // Mark the building tile on Buildings layer with b_townhall sprite
    World::TileLayer* buildingsLayer = map->GetLayer(World::Buildings);
    if (buildingsLayer && hqBuildX >= 0 && hqBuildX < buildingsLayer->GetWidth() &&
        hqBuildY >= 0 && hqBuildY < buildingsLayer->GetHeight())
    {
        World::Tile& bt = buildingsLayer->GetTile(hqBuildX, hqBuildY);
        bt.type = World::Decoration;
        bt.atlasName = "Buildings";
        bt.walkable = false;
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
    World::Flag* hqFlag = flagManager->GetFlagAt(hqFlagX, hqFlagY);
    if (!hqFlag) {
        hqFlag = flagManager->CreateFlag(hqFlagX, hqFlagY);
    }
    hqFlag->type = World::FLAG_WAREHOUSE;
    hqFlag->hasBuilding = true;

    // Create warehouse and link it
    World::Warehouse* warehouse = new World::Warehouse(hqBuildX, hqBuildY, 0);
    warehouse->connectedFlag = hqFlag;
    hqFlag->building = warehouse;
    warehouse->map = map;
    if (storehouse) {
        warehouse->SetStorehouseManager(storehouse);
    }

    // Connect HQ flag to any existing road network
    relinker.RebuildFromFlag(hqFlag);

    // Seed warehouse with starting resources
    warehouse->AddResource(World::ResourceType_Wood, 500);
    warehouse->AddResource(World::ResourceType_Stone, 500);
    warehouse->AddResource(World::ResourceType_Planks, 200);
    warehouse->AddResource(World::ResourceType_Fish, 100);
    warehouse->AddResource(World::ResourceType_Meat, 100);
    warehouse->AddResource(World::ResourceType_Coal, 100);

    economy->SetWarehouse(warehouse);
    economy->AddBuilding(warehouse);
    if (transportJobs) {
        transportJobs->SetWarehouse(warehouse);
    }
    if (construction) {
        construction->SetWarehouseFlag(hqFlag);
    }
    if (carrierManager) {
        carrierManager->SetWarehouseFlag(hqFlag);
    }
    // Set warehouse demand for all resource types
    if (demand && hqFlag) {
        World::ResourceType allTypes[] = {
            World::ResourceType_Wood, World::ResourceType_Stone, World::ResourceType_Planks,
            World::ResourceType_Fish, World::ResourceType_Meat, World::ResourceType_Coal,
            World::ResourceType_BronzeBar
        };
        for (int ri = 0; ri < sizeof(allTypes)/sizeof(allTypes[0]); ++ri) {
            demand->SetDemand(allTypes[ri], 9999, hqFlag->handle, 10);
        }
    }

    // Sync carriers and startup diagnostics
    relinker.SyncCarriers(hqFlag);

    {
        char buf[256];
        _snprintf(buf, sizeof(buf),
            "[Startup] Warehouse Wood=%u Stone=%u Planks=%u Fish=%u Meat=%u Coal=%u\n",
            storehouse ? storehouse->GetStoredCount(World::ResourceType_Wood) : 0,
            storehouse ? storehouse->GetStoredCount(World::ResourceType_Stone) : 0,
            storehouse ? storehouse->GetStoredCount(World::ResourceType_Planks) : 0,
            storehouse ? storehouse->GetStoredCount(World::ResourceType_Fish) : 0,
            storehouse ? storehouse->GetStoredCount(World::ResourceType_Meat) : 0,
            storehouse ? storehouse->GetStoredCount(World::ResourceType_Coal) : 0);
        OutputDebugStringA(buf);
    }

    {
        World::Warehouse* wh = economy ? economy->GetWarehouse() : NULL;
        if (wh) {
            char buf[512];
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
    }

    // Flag dump for diagnostics
    {
        char buf[256];
        _snprintf(buf, sizeof(buf), "[Startup] FlagManager has %u flags:\n", (unsigned)flagManager->GetCount());
        OutputDebugStringA(buf);
        for (size_t fi = 0; fi < flagManager->GetCount(); ++fi) {
            World::Flag* ff = flagManager->GetFlag(fi);
            if (ff) {
                _snprintf(buf, sizeof(buf), "[Startup] Flags[%u]: id=%u pos=(%d,%d) type=%d roads=%u",
                    (unsigned)fi, ff->id, ff->pos.x, ff->pos.y, ff->type, (unsigned)ff->roads.size());
                OutputDebugStringA(buf);
                for (size_t ri = 0; ri < ff->roads.size(); ++ri) {
                    World::Flag* rra = flagManager->ResolveFlag(ff->roads[ri]->a);
                    World::Flag* rrb = flagManager->ResolveFlag(ff->roads[ri]->b);
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

    char buf[256];
    _snprintf(buf, sizeof(buf), "[Startup] Warehouse starting resources seeded at (%d,%d)\n", hqBuildX, hqBuildY);
    OutputDebugStringA(buf);
}

} // namespace WorldBootstrap
