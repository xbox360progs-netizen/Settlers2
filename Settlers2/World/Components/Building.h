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
    CoalMine, IronMine, GoldMine, IronSmelter, GoldSmelter, Farm, Mill, Bakery, Fisher, Hunter, Baker, Brewer, ToolWorkshop, Storehouse,
    Residence, Stronghold, Well, BronzeMine, ToolMaker, Barracks
};

enum BuildingState {
    State_Ghost, State_MaterialsNeeded, State_BuilderWorking, State_Construction, State_Finished
};

enum BuildingFSM {
    BuildingFSM_Idle,
    BuildingFSM_Producing,
    BuildingFSM_OutputFull
};

struct ProductionRule {
    ResourceType input[2];
    int inputAmount[2];
    ResourceType output[2];
    int outputAmount[2];
    int numInputs;
    int numOutputs;
    int outputCap;

    ProductionRule()
        : numInputs(0), numOutputs(0), outputCap(5)
    {
        input[0] = ResourceType_None; input[1] = ResourceType_None;
        inputAmount[0] = 0; inputAmount[1] = 0;
        output[0] = ResourceType_None; output[1] = ResourceType_None;
        outputAmount[0] = 0; outputAmount[1] = 0;
    }

    void AddInput(ResourceType t, int amount = 1) {
        if (numInputs < 2) {
            input[numInputs] = t;
            inputAmount[numInputs] = amount;
            numInputs++;
        }
    }

    void AddOutput(ResourceType t, int amount = 1) {
        if (numOutputs < 2) {
            output[numOutputs] = t;
            outputAmount[numOutputs] = amount;
            numOutputs++;
        }
    }
};

class Building {
public:
    BuildingType type;
    BuildingState state;
    Vector2i pos;
    uint8_t owner;
    Flag* connectedFlag;
    Map* map;

    // Legacy vectors — used by simple buildings (Woodcutter, Farm, etc.)
    std::vector<ResourceType> inputResources;
    std::vector<ResourceType> outputResources;

    // Production rules
    ProductionRule m_rules[4];
    int m_numRules;

    // Array-based storage (replaces std::map inventory)
    int m_storage[ResourceType_Count];

     // Extraction caching
     bool m_hasTarget;
     Vector2i m_target;
 
     // Production FSM
     BuildingFSM m_fsmState;
     float m_productionTimer;
     float m_productionInterval;

     // Worker population
     int m_population;
     int m_maxPopulation;
 
     // Construction tracking (used infrequently, keep as map for now)
     std::map<ResourceType, int> constructionMaterials;
     std::map<ResourceType, int> deliveredMaterials;
 
     Building(BuildingType t, int x, int y, uint8_t o, Map* m)
         : type(t), state(State_Ghost), owner(o), connectedFlag(NULL), map(m), m_numRules(0), 
           m_hasTarget(false), m_fsmState(BuildingFSM_Idle), m_productionTimer(0.0f),
           m_productionInterval(3.0f), m_population(0), m_maxPopulation(0)
     {
         pos.x = x;
         pos.y = y;
         m_target.x = 0;
         m_target.y = 0;
         for (int i = 0; i < ResourceType_Count; ++i)
             m_storage[i] = 0;

         // Set max population based on building type
         switch (type) {
             case Woodcutter:
             case Forester:
                 m_maxPopulation = 1;
                 break;
             case Sawmill:
                 m_maxPopulation = 2;
                 break;
             case Stonemason:
                 m_maxPopulation = 1;
                 break;
             case CoalMine:
             case IronMine:
             case GoldMine:
                 m_maxPopulation = 3;
                 break;
             case IronSmelter:
             case GoldSmelter:
                 m_maxPopulation = 2;
                 break;
             case Farm:
                 m_maxPopulation = 1;
                 break;
             case Mill:
                 m_maxPopulation = 2;
                 break;
             case Bakery:
                 m_maxPopulation = 2;
                 break;
             case Fisher:
                 m_maxPopulation = 1;
                 break;
             case Hunter:
                 m_maxPopulation = 1;
                 break;
             case ToolWorkshop:
                 m_maxPopulation = 2;
                 break;
             default:
                 m_maxPopulation = 0;
         }
     }

    virtual ~Building() {}

    // ── Central FSM dispatch ──────────────────────────────────────────
    // Subclasses should NOT override Update() unless they have an entirely
    // different production model (e.g. ProductionBuilding).
    // Instead, override the virtual hooks below.
    virtual void Update(float dt);

    // Override to define per-building production. Return false if output
    // is full and the building should stop.
    virtual bool ProduceOne() { return false; }

    // Override to define when production can start (target found, inputs
    // available, etc.). Default: true.
    virtual bool CanProduce() { return true; }

    // Override to customize the output-full check. Default: >= 5 units of
    // any resource in outputResources.
    virtual bool IsOutputFull() const;

protected:
    // State handlers — override to inject per-state logic.
    virtual void UpdateIdle(float dt);
    virtual void UpdateProducing(float dt);
    virtual void UpdateOutputFull(float dt);

    // Helper: add to storage, returns false if output is full (caller
    // should transition to OutputFull state).
    bool AddOutput(ResourceType type, int amount = 1);

public:

    // Worker rendering info for moving workers (Hunter, etc.)
    // Returns false if the worker is not visible (inside building).
    // When true, fills outX/outY (float node coords) and outSpriteIdx (flat Units atlas index).
    virtual bool GetWorkerRenderInfo(float& outX, float& outY, int& outSpriteIdx) const { return false; }

    virtual bool IsWarehouse() const { return false; }

    int GetStorage(ResourceType type) const { return m_storage[type]; }
    void SetStorage(ResourceType type, int val) { m_storage[type] = val; }
    void AddStorage(ResourceType type, int val) { m_storage[type] += val; }
    bool HasStorage(ResourceType type, int min = 1) const { return m_storage[type] >= min; }

    int ConsumeFood() {
        int varietyBonus = 0;

        if (m_storage[ResourceType_Bread] > 0) { m_storage[ResourceType_Bread]--; varietyBonus++; }
        if (m_storage[ResourceType_Meat] > 0)  { m_storage[ResourceType_Meat]--;  varietyBonus++; }
        if (m_storage[ResourceType_Fish] > 0)  { m_storage[ResourceType_Fish]--;  varietyBonus++; }

        return varietyBonus;
    }

	bool NeedsResource(ResourceType type) const {

        for (size_t i = 0; i < outputResources.size(); ++i) {
            if (outputResources[i] == type) return false;
        }
        
        for (size_t i = 0; i < inputResources.size(); ++i) {
            if (inputResources[i] == type) {
                return m_storage[type] < MaxStoragePerType(type);
            }
        }
        
        return false;  
    }

	int MaxStoragePerType(ResourceType type) const {
        return 5;  
    }
};

} // namespace World

#endif
