#ifndef WORLD_COMPONENTS_BUILDING_H
#define WORLD_COMPONENTS_BUILDING_H

#include "../ResourceNode.h"
#include "../../Core/Vector2i.h"
#include <vector>

namespace World {

class Flag;
class Map;

enum BuildingType {
    Building_None, Hut, Tower, Fortress, Castle, Forester, Woodcutter, Sawmill, Stonemason,
    CoalMine, IronMine, GoldMine, IronSmelter, GoldSmelter, Farm, Mill, Bakery, Fisher, Hunter, Baker, Brewer, ToolWorkshop, Storehouse,
    Residence, Stronghold, Well, BronzeMine, ToolMaker, Barracks, BronzeSmelter
};

// Building instances always represent completed, functional buildings.
// Construction lifecycle is managed by ConstructionSite.
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

     // Extraction caching — cached resource node for O(1) FindTarget
     bool m_hasTarget;
     Vector2i m_target;

     // Resource cache: up to 8 pre-computed candidate resource positions within range
     Vector2i m_cachedResourceNodes[8];
     int m_cachedNodeCount;
     float m_cacheTimer;            // cooldown before refreshing cache
 
     // Production FSM
     BuildingFSM m_fsmState;
     float m_productionTimer;
     float m_productionInterval;

     // Worker population
     int m_population;
     int m_maxPopulation;
 
     // Depletion tracking
     bool m_isDepleted;
     int m_depletedSpriteIdx;
     int m_totalProduced;
     int m_maxProduction;

     // Footprint dimensions (set on construction, used for tile cleanup on delete)
     int m_footprintX;
     int m_footprintY;
     int m_footprintW;
     int m_footprintH;
 
     // Refresh cached resource nodes. Override in production buildings for O(1) FindTarget.
     virtual void RefreshResourceCache() { m_cachedNodeCount = 0; }

     
          Building(BuildingType t, int x, int y, uint8_t o, Map* m)
               : type(t), owner(o), connectedFlag(NULL), map(m), m_numRules(0), 
                m_hasTarget(false), m_fsmState(BuildingFSM_Idle), m_productionTimer(0.0f),
                m_productionInterval(3.0f), m_population(0), m_maxPopulation(0),
                m_cachedNodeCount(0), m_cacheTimer(0.0f),
                m_isDepleted(false), m_depletedSpriteIdx(-1), m_totalProduced(0), m_maxProduction(0),
              m_footprintX(0), m_footprintY(0), m_footprintW(1), m_footprintH(1)
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
            case BronzeMine:
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
             case BronzeSmelter:
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

    // Returns true if there's a work-site sprite to render (e.g., mine framework at resource node)
    virtual bool GetWorkSiteRenderInfo(Vector2i& outPosition, const char*& outSpriteName) const { return false; }

    virtual bool IsWarehouse() const { return false; }

    // Polymorphic resource-mode switching (overridden by configurable buildings like Stonemason)
    virtual void SetActiveResourceMode(ResourceType /*type*/) {}
    virtual ResourceType GetActiveResourceMode() const { return ResourceType_None; }

    int GetStorage(ResourceType type) const { return m_storage[type]; }
    void SetStorage(ResourceType type, int val) { m_storage[type] = val; }
    void AddStorage(ResourceType type, int val) { m_storage[type] += val; }
    bool HasStorage(ResourceType type, int min = 1) const { return m_storage[type] >= min; }

    void SetDepleted() { m_isDepleted = true; }
    bool IsDepleted() const { return m_isDepleted; }
    BuildingFSM GetFsmState() const { return m_fsmState; }

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

	virtual int MaxStoragePerType(ResourceType type) const {
        return 5;  
    }
};

} // namespace World

#endif
