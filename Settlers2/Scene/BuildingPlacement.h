#pragma once

#include "../World/Map.h"
#include "../World/FlagManager.h"
#include "../World/RoadManager.h"
#include "../World/CarrierManager.h"
#include "../Logic/EconomyManager.h"
#include "../World/DemandManager.h"
#include "../Graphics/SpriteAtlas.h"
#include <string>
#include <vector>

namespace Scene {

struct PlacementData {
    bool valid;
    int buildX, buildY;
    int flagX, flagY;
    int entranceX, entranceY;
    int footOffX, footOffY;
    int footW, footH;
    std::vector<std::pair<int,int>> footMask;
    std::vector<std::pair<int,int>> footprintTiles;
    const char* errorMsg;
    int spriteIdx;
    const SpriteRegion* spriteRegion;
    int constrIdx;

    PlacementData()
        : valid(false), buildX(0), buildY(0), flagX(0), flagY(0)
        , entranceX(0), entranceY(0), footOffX(0), footOffY(0)
        , footW(1), footH(1)
        , errorMsg(NULL), spriteIdx(-1), spriteRegion(NULL), constrIdx(-1) {}
};

class BuildingPlacementManager {
public:
    BuildingPlacementManager(
        World::Map* map,
        World::FlagManager* flagManager,
        World::RoadManager* roadManager,
        World::CarrierManager* carrierManager,
        Logic::EconomyManager* economyManager,
        World::DemandManager* demandManager
    );

    void SelectBuilding(World::BuildingType type);
    void Deselect();
    bool IsSelected() const;
    World::BuildingType GetSelectedType() const;

    PlacementData GetPlacementData(int cursorX, int cursorY);

    int GetSpriteIdx() const;
    int GetConstrIdx() const;
    int GetEntranceX() const;
    int GetEntranceY() const;
    int GetFootOffX() const;
    int GetFootOffY() const;
    int GetFootW() const;
    int GetFootH() const;
    const std::vector<std::pair<int,int>>& GetFootMask() const;
    const std::string& GetSpriteName() const;

    static const char* GetBuildingSpriteName(World::BuildingType type);
    static World::ResourceType GetResourceTypeForMine(World::BuildingType type);
    static bool IsMineType(World::BuildingType type);
    static void GetEntranceOffset(const std::string& buildingName, int& outX, int& outY);

private:
    World::Map* m_map;
    World::FlagManager* m_flagManager;
    World::RoadManager* m_roadManager;
    World::CarrierManager* m_carrierManager;
    Logic::EconomyManager* m_economyManager;
    World::DemandManager* m_demandManager;

    World::BuildingType m_selectedType;
    bool m_isSelected;

    int m_entranceX, m_entranceY;
    int m_footOffX, m_footOffY;
    int m_footW, m_footH;
    std::vector<std::pair<int,int>> m_footMask;
    int m_spriteIdx;
    int m_constrIdx;
    std::string m_spriteName;

    void LoadBuildingMetadata();
    int FindBuildingSpriteIdx(const std::string& spriteName);
};

} // namespace Scene
