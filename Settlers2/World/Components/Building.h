#ifndef WORLD_COMPONENTS_BUILDING_H
#define WORLD_COMPONENTS_BUILDING_H

#include "../ResourceNode.h"
#include "../../Core/Vector2i.h"
#include <vector>
#include <map>

namespace World {

class Flag;
class Map;

enum BuildingType {
    Building_None, Hut, Tower, Fortress, Castle, Forester, Woodcutter, Sawmill, Stonemason,
    CoalMine, IronMine, GoldMine, IronSmelter, GoldSmelter, Farm, Mill, Bakery, Fisher, Hunter, Baker, Brewer, ToolWorkshop
};

enum BuildingState {
    State_Ghost, State_MaterialsNeeded, State_BuilderWorking, State_Construction, State_Finished
};

class Building {
public:
    BuildingType type;
    BuildingState state;
    Vector2i pos;
    uint8_t owner;
    Flag* connectedFlag;
    Map* map;
    
    std::vector<ResourceType> inputResources;
    std::vector<ResourceType> outputResources;
    std::map<ResourceType, int> inventory;
    std::map<ResourceType, int> constructionMaterials;
    std::map<ResourceType, int> deliveredMaterials;

    Building(BuildingType t, int x, int y, uint8_t o, Map* m) 
        : type(t), state(State_Ghost), owner(o), connectedFlag(NULL), map(m) 
    {
        pos.x = x;
        pos.y = y;
    }

    virtual ~Building() {}
    
    virtual void Update() = 0;

    int ConsumeFood() {
        int varietyBonus = 0;
        
        std::map<ResourceType, int>::iterator itBread = inventory.find(ResourceType_Bread);
        if (itBread != inventory.end() && itBread->second > 0) { 
            itBread->second--; 
            varietyBonus++; 
        }

        std::map<ResourceType, int>::iterator itMeat = inventory.find(ResourceType_Meat);
        if (itMeat != inventory.end() && itMeat->second > 0) { 
            itMeat->second--; 
            varietyBonus++; 
        }

        std::map<ResourceType, int>::iterator itFish = inventory.find(ResourceType_Fish);
        if (itFish != inventory.end() && itFish->second > 0) { 
            itFish->second--; 
            varietyBonus++; 
        }

        return varietyBonus;
    }
};

} // namespace World

#endif
