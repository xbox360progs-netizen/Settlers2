#pragma once
#include <vector>
#include "../../Core/EventBus.h"
#include "../Components/Building.h"

namespace Logic { class AISystem; }

namespace World {
    class Map;
    class FlagManager;
    class Flag;
}

namespace World {

class BuildingSystem : public Core::EventListener {
public:
    BuildingSystem();
    ~BuildingSystem();

    void Initialize(Map* map, FlagManager* flagManager, Core::EventBus* eventBus);

    void RegisterBuilding(Building* building);
    void UnregisterBuilding(Building* building);

    Building* CreateBuilding(BuildingType type, int posX, int posY, Flag* flag);
    void DestroyBuilding(Building* building);

    void Update(float dt);

    int GetBuildingCount() const { return (int)m_buildings.size(); }
    Building* GetBuilding(int index) const { return m_buildings[index]; }
    Building* FindBuilding(BuildingType type) const;

    void AddToLayer(Building* building);

    virtual void OnEvent(Core::EventType type, void* data);

private:
    std::vector<Building*> m_buildings;
    Map* m_map;
    FlagManager* m_flagManager;
    Core::EventBus* m_eventBus;
};

} // namespace World
