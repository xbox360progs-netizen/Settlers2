#include "stdafx.h"
#include "BuildingSystem.h"
#include "../Map.h"
#include "../FlagManager.h"
#include "../Flag.h"
#include "../WorkerManager.h"
#include "../Components/BuildingFactory.h" // provides World::CreateBuilding()
#include "../Warehouse.h"
#include "../TileLayer.h"
#include "../../Core/EventBus.h"
#include "../../Logic/EconomyManager.h"

namespace World {

BuildingSystem::BuildingSystem()
    : m_map(NULL)
    , m_flagManager(NULL)
    , m_eventBus(NULL)
    , m_commandBus(NULL)
    , m_economyManager(NULL)
    , m_workerManager(NULL)
{
}

BuildingSystem::~BuildingSystem()
{
    if (m_eventBus) {
        m_eventBus->UnregisterAll(this);
    }
}

void BuildingSystem::Initialize(Map* map, FlagManager* flagManager, Core::EventBus* eventBus, Core::CommandBus* commandBus)
{
    m_map = map;
    m_flagManager = flagManager;
    m_eventBus = eventBus;
    m_commandBus = commandBus;

    if (m_eventBus) {
        m_eventBus->Register(Core::Event_ConstructionComplete, this);
    }
}

void BuildingSystem::RegisterBuilding(Building* building)
{
    m_buildings.push_back(building);
}

void BuildingSystem::UnregisterBuilding(Building* building)
{
    for (size_t i = 0; i < m_buildings.size(); ++i) {
        if (m_buildings[i] == building) {
            m_buildings.erase(m_buildings.begin() + i);
            return;
        }
    }
}

Building* BuildingSystem::CreateBuilding(BuildingType type, int posX, int posY, Flag* flag)
{
    Building* building = World::CreateBuilding(type, posX, posY, 0, m_map);
    if (!building) return NULL;

    building->connectedFlag = flag;
    building->map = m_map;

    if (flag) {
        flag->building = building;
        flag->hasBuilding = true;
    }

    m_buildings.push_back(building);
    return building;
}

void BuildingSystem::DestroyBuilding(Building* building)
{
    if (!building) return;

    if (building->connectedFlag) {
        building->connectedFlag->building = NULL;
        building->connectedFlag->hasBuilding = false;
    }

    for (size_t i = 0; i < m_buildings.size(); ++i) {
        if (m_buildings[i] == building) {
            m_buildings.erase(m_buildings.begin() + i);
            break;
        }
    }

    delete building;
}

void BuildingSystem::AddToLayer(Building* building)
{
    if (!m_map || !building) return;

    TileLayer* buildingsLayer = m_map->GetLayer(Buildings);
    if (!buildingsLayer) return;

    int tx = building->pos.x;
    int ty = building->pos.y;
    if (tx < 0 || tx >= buildingsLayer->GetWidth() || ty < 0 || ty >= buildingsLayer->GetHeight()) return;

    Tile& tile = buildingsLayer->GetTile(tx, ty);
    tile.type = Tile_Building;
    tile.atlasName = "Buildings";
    tile.walkable = false;
}

void BuildingSystem::Update(float dt)
{
    (void)dt;
}

Building* BuildingSystem::FindBuilding(BuildingType type) const
{
    for (size_t i = 0; i < m_buildings.size(); ++i) {
        if (m_buildings[i]->type == type) return m_buildings[i];
    }
    return NULL;
}

void BuildingSystem::OnEvent(Core::EventType type, void* data)
{
    if (type == Core::Event_ConstructionComplete) {
        Core::ConstructionCompleteData* evt = static_cast<Core::ConstructionCompleteData*>(data);
        if (evt) {
            HandleConstructionComplete(*evt);
        }
    }
}

void BuildingSystem::HandleConstructionComplete(const Core::ConstructionCompleteData& evt)
{
    if (!m_flagManager || !m_map) return;

    Flag* flag = m_flagManager->GetFlagById(evt.flagId);
    if (!flag) return;

    // Guard: if flag already has a building, it was already completed
    if (flag->hasBuilding) return;

    // Create the building
    Building* building = CreateBuilding(
        static_cast<BuildingType>(evt.buildingType),
        evt.siteX, evt.siteY, flag);
    if (!building) return;

    building->state = State_Finished;
    flag->pendingBuilding = Building_None;

    // Basic tile setup
    AddToLayer(building);

    // Register with economy
    if (m_economyManager) {
        m_economyManager->AddBuilding(building);
    }

    // Spawn worker from warehouse
    if (m_workerManager && m_economyManager) {
        Warehouse* wh = m_economyManager->GetWarehouse();
        if (wh && wh->connectedFlag && building->m_maxPopulation > 0) {
            m_workerManager->SpawnWorker(
                building,
                static_cast<float>(wh->connectedFlag->pos.x),
                static_cast<float>(wh->connectedFlag->pos.y));
        }
    }

    // Notify the world via event
    if (m_eventBus) {
        Core::BuildingPlacedData bd;
        bd.buildingType = evt.buildingType;
        bd.posX = evt.siteX;
        bd.posY = evt.siteY;
        bd.flagId = evt.flagId;
        m_eventBus->Post(Core::Event_BuildingPlaced, bd);
    }

    // Remove the construction site
    if (m_commandBus) {
        Core::RemoveConstructionSiteCmd rm;
        rm.siteId = evt.siteId;
        m_commandBus->Post(Core::Cmd_RemoveConstructionSite, rm);
    }
}

} // namespace World
