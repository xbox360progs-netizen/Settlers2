#include "stdafx.h"
#include "WorldRestorer.h"
#include "../Core/EventBus.h"
#include "../World/Map.h"
#include "../World/Flag.h"
#include "../World/FlagManager.h"
#include "../World/RoadManager.h"
#include "../World/CarrierManager.h"
#include "../World/DemandManager.h"
#include "../World/StorehouseManager.h"
#include "../World/TransportJobManager.h"
#include "../World/ConstructionManager.h"
#include "../World/Systems/RoadNetworkRelinker.h"
#include "../World/Components/Building.h"
#include "../World/Components/BuildingFactory.h"
#include "../Logic/EconomyManager.h"
#include "../Graphics/SpriteAtlas.h"
#include "../Graphics/TextureRegistry.h"

namespace Scene {

WorldRestorer::WorldRestorer()
{
}

void WorldRestorer::AdjustEntranceForParity(bool buildingEvenY, int& entranceX, int entranceY)
{
    if (!buildingEvenY && entranceY != 0 && entranceX > 0) {
        entranceX = entranceX - 1;
    }
}

World::BuildingType WorldRestorer::GetBuildingTypeFromSpriteName(const std::string& name) const
{
    std::string key = name;
    if (key.compare(0, 2, "b_") == 0)
        key = key.substr(2);
    if (key.compare(0, 3, "ib_") == 0)
        key = key.substr(3);

    struct { const char* name; World::BuildingType type; } entries[] = {
        { "woodcutter",     World::Woodcutter },
        { "sawmill",        World::Sawmill },
        { "coalmine",       World::CoalMine },
        { "ironmine",       World::IronMine },
        { "goldmine",       World::GoldMine },
        { "ironsmelter",    World::IronSmelter },
        { "goldsmelter",    World::GoldSmelter },
        { "farm",           World::Farm },
        { "mill",           World::Mill },
        { "bakery",         World::Bakery },
        { "fisher",         World::Fisher },
        { "hunter",         World::Hunter },
        { "toolworkshop",   World::ToolWorkshop },
        { "forester",       World::Forester },
        { "stonemason",     World::Stonemason },
        { "well",           World::Well },
        { "barracks",       World::Barracks },
        { "warehouse",      World::Storehouse },
        { "townhall",       World::Storehouse },
        { "bronzemine",     World::BronzeMine },
        { "bronzesmelter",  World::BronzeSmelter },
    };

    for (int i = 0; i < sizeof(entries)/sizeof(entries[0]); ++i) {
        if (key == entries[i].name)
            return entries[i].type;
    }
    return World::Building_None;
}

void WorldRestorer::AssignOreDepositsToMountains()
{
    World::Map* map = m_ctx.map;
    Logic::EconomyManager* economy = m_ctx.economy;
    if (!map) { OutputDebugStringA("[AssignOre] FAIL: no map\n"); return; }
    World::TileLayer* objLayer = map->GetLayer(World::Objects);
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

            World::ResourceNode& rn = map->GetResourceNode(x, y);
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

            if (economy) {
                economy->GetRegistry().RegisterWorldResource(oreType, x, y);
            }
            assigned++;
        }
    }

    {
        char dbg[512];
        int pos = _snprintf(dbg, sizeof(dbg), "[WorldRestorer] Mountains=%d assigned=%d map=(%dx%d)", mountainCount, assigned, w, h);
        if (assigned > 0) {
            pos += _snprintf(dbg + pos, sizeof(dbg) - pos, "\n");
            for (int y = 0; y < h && assigned > 0; ++y) {
                for (int x = 0; x < w; ++x) {
                    const World::Tile& tile = objLayer->GetTile(x, y);
                    if (tile.type != World::Mountain && tile.type != World::MountainOnWater) continue;
                    const World::ResourceNode& rn = map->GetResourceNode(x, y);
                    if (rn.type == World::ResourceType_None) continue;
                    pos += _snprintf(dbg + pos, sizeof(dbg) - pos, "  Mountain (%d,%d) -> %s (amount=%d)\n", x, y, World::ResourceTypeToString(rn.type), rn.amount);
                }
            }
        }
        OutputDebugStringA(dbg);
    }
}

void WorldRestorer::RestoreBuildingsFromLayer()
{
    World::Map* map = m_ctx.map;
    World::FlagManager* flagManager = m_ctx.flags;
    Logic::EconomyManager* economy = m_ctx.economy;
    if (!map || !flagManager || !economy) return;

    World::TileLayer* buildingsLayer = map->GetLayer(World::Buildings);
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

            const std::string& spriteName = region->name;
            bool isConstruction = (spriteName.find("construction") != std::string::npos ||
                spriteName.find("Construction") != std::string::npos);
            if (isConstruction)
            { skipped++; continue; }

            bool isBuildingSprite = region->isBuilding;
            World::BuildingType type = World::Building_None;
            if (tile.buildingType >= 0) {
                type = static_cast<World::BuildingType>(tile.buildingType);
            } else {
                type = GetBuildingTypeFromSpriteName(spriteName);
                if (type == World::Building_None) {
                    std::string key = spriteName;
                    if (key.compare(0, 2, "b_") == 0) key = key.substr(2);
                    if (key == "mine" && map) {
                        const World::ResourceNode& rn = map->GetResourceNode(x, y);
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
            if (type == World::Building_None) continue;

            int entranceX = region->entranceX;
            int entranceY = region->entranceY;

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

            World::Flag* flag = flagManager->GetFlagAt(flagX, flagY);
            if (!flag) {
                flag = flagManager->CreateFlag(flagX, flagY);
                flag->type = World::FLAG_BUILDING;
            }

            if (flag->building) { skipped++; continue; }

            if (isWarehouseType && economy->GetWarehouse()) { skipped++; continue; }

            World::Building* building = NULL;

            if (isWarehouseType) {
                World::Warehouse* wh = new World::Warehouse(x, y, 0);
                wh->connectedFlag = flag;
                wh->map = map;
                flag->building = wh;
                flag->hasBuilding = true;
                flag->pendingBuilding = World::Building_None;
                flag->type = World::FLAG_WAREHOUSE;
                building = wh;

                if (m_ctx.storehouse) {
                    wh->SetStorehouseManager(m_ctx.storehouse);
                }

                wh->AddResource(World::ResourceType_Wood, 500);
                wh->AddResource(World::ResourceType_Stone, 500);
                wh->AddResource(World::ResourceType_Planks, 200);
                wh->AddResource(World::ResourceType_Fish, 100);
                wh->AddResource(World::ResourceType_Meat, 100);
                wh->AddResource(World::ResourceType_Coal, 100);

                economy->SetWarehouse(wh);
                if (m_ctx.transportJobs) {
                    m_ctx.transportJobs->SetWarehouse(wh);
                }
                if (m_ctx.construction) {
                    m_ctx.construction->SetWarehouseFlag(flag);
                }
                if (m_ctx.carriers) {
                    m_ctx.carriers->SetWarehouseFlag(flag);
                }
                if (m_ctx.demand && flag) {
                    World::ResourceType allTypes[] = {
                        World::ResourceType_Wood, World::ResourceType_Stone, World::ResourceType_Planks,
                        World::ResourceType_Fish, World::ResourceType_Meat, World::ResourceType_Coal,
                        World::ResourceType_BronzeBar
                    };
                    for (int ri = 0; ri < sizeof(allTypes)/sizeof(allTypes[0]); ++ri) {
                        m_ctx.demand->SetDemand(allTypes[ri], 9999, flag->handle, 10);
                    }
                }
            } else {
                building = World::CreateBuilding(type, x, y, 0, map);
                if (!building) { skipped++; continue; }
                building->connectedFlag = flag;
                building->map = map;
                flag->building = building;
                flag->hasBuilding = true;
                flag->pendingBuilding = World::Building_None;

                {
                    const SpriteRegion* r2 = atlas->GetRegion(tile.regionIndex);
                    if (r2) {
                        building->m_footprintX = r2->collOffX;
                        building->m_footprintY = r2->collOffY;
                        building->m_footprintW = (int)r2->collWidth;
                        building->m_footprintH = (int)r2->collHeight;
                        if (building->m_footprintW < 1) building->m_footprintW = 1;
                        if (building->m_footprintH < 1) building->m_footprintH = 1;
                        bool is2x2 = (type == World::Stonemason || type == World::Sawmill || type == World::Farm || type == World::Mill);
                        if (is2x2 && (building->m_footprintW != 2 || building->m_footprintH != 2)) {
                            building->m_footprintW = 2;
                            building->m_footprintH = 2;
                        }
                    }
                }

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

            economy->AddBuilding(building);
            if (m_ctx.relinker) {
                m_ctx.relinker->RebuildFromFlag(flag);
                m_ctx.relinker->SyncCarriers(flag);
            }

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

} // namespace Scene
