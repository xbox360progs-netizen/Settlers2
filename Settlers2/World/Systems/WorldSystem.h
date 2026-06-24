#pragma once
#include "../../Core/EventBus.h"

namespace World {
    class Map;
    class FlagManager;
    class CargoManager;
}

namespace World {

class WorldSystem : public Core::EventListener {
public:
    WorldSystem();
    ~WorldSystem();

    void Initialize(Map* map, FlagManager* flagManager, CargoManager* cargoManager,
                    Core::EventBus* eventBus);

    void Update(float dt);

    float GetTreeGrowthTimer() const { return m_treeGrowthTimer; }
    float GetWildlifeRegenTimer() const { return m_wildlifeRegenTimer; }

    // Ground resource collection — moved from GameScene
    void CollectGroundResourcesToNearestFlag(uint32_t whFlagId);

    virtual void OnEvent(Core::EventType type, void* data);

private:
    Map* m_map;
    FlagManager* m_flagManager;
    CargoManager* m_cargoManager;
    Core::EventBus* m_eventBus;

    float m_wildlifeRegenTimer;
    float m_treeGrowthTimer;

    static const float WILDLIFE_REGEN_INTERVAL;
    static const float TREE_GROWTH_INTERVAL;
};

} // namespace World
