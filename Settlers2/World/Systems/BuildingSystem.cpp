#include "stdafx.h"
#include "BuildingSystem.h"
#include "../Map.h"
#include "../FlagManager.h"
#include "../Flag.h"
#include "../Components/BuildingFactory.h" // provides World::CreateBuilding()
#include "../TileLayer.h"
#include "../../Core/EventBus.h"

namespace World {

BuildingSystem::BuildingSystem()
    : m_map(NULL)
    , m_flagManager(NULL)
    , m_eventBus(NULL)
{
}

BuildingSystem::~BuildingSystem()
{
    if (m_eventBus) {
        m_eventBus->UnregisterAll(this);
    }
}

void BuildingSystem::Initialize(Map* map, FlagManager* flagManager, Core::EventBus* eventBus)
{
    m_map = map;
    m_flagManager = flagManager;
    m_eventBus = eventBus;

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
    (void)data;
    (void)type;
}

} // namespace World
