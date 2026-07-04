#include "../../Testing/ISimulationScenario.h"
#include "../../Testing/Assertions/ConstructionAssertions.h"
#include "../../Testing/Assertions/TransportAssertions.h"
#include "../../Testing/Assertions/WorldAssertions.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Definitions/BuildingDefinition.h"
#include "../../Definitions/ProductionDefinition.h"
#include <stdio.h>

namespace World {

class T33DefinitionQueryTest : public ISimulationScenario {
public:
    const char* GetName() const { return "T33"; }

    void Configure(SimulationConfig& config) const
    {
    }

    void Initialize(Simulation& sim)
    {
        WorldModel world;
        world.width = 10;
        world.height = 10;
        sim.LoadWorld(world);
    }

    bool Tick(Simulation& sim)
    {
        return Verify(sim);
    }

    bool Verify(Simulation& /*sim*/)
    {
        bool ok = true;

        // ---- Check 1: GetProducer for known resources ----
        struct ResourceProducerPair {
            ResourceType resource;
            ProductionType expected;
        };

        static const ResourceProducerPair kKnown[] = {
            { ResourceType_Wood,     PT_Woodcutter },
            { ResourceType_Planks,   PT_Sawmill },
            { ResourceType_Stone,    PT_Stonemason },
            { ResourceType_Fish,     PT_Fisher },
            { ResourceType_Meat,     PT_Hunter },
            { ResourceType_Wheat,    PT_Farm },
            { ResourceType_Flour,    PT_Mill },
            { ResourceType_Bread,    PT_Bakery },
            { ResourceType_Coal,     PT_CoalMine },
            { ResourceType_IronOre,  PT_IronMine },
            { ResourceType_IronBar,  PT_IronSmelter },
            { ResourceType_Tools,    PT_Toolmaker },
            { ResourceType_Water,    PT_Well },
        };

        static const int kKnownCount = sizeof(kKnown) / sizeof(kKnown[0]);

        for (int i = 0; i < kKnownCount; ++i) {
            ProductionType result = GetProducer(kKnown[i].resource);
            if (result != kKnown[i].expected) {
                printf("[FAIL][T33.A] GetProducer(resource=%d) = %d (expected %d)\n",
                    static_cast<int>(kKnown[i].resource),
                    static_cast<int>(result),
                    static_cast<int>(kKnown[i].expected));
                ok = false;
            }
        }
        if (ok) {
            printf("[PASS][T33.A] GetProducer: all %d known resources map correctly\n", kKnownCount);
        }

        // ---- Check 2: GetProducer for unknown resource returns PT_None ----
        ProductionType unknown = GetProducer(ResourceType_None);
        if (unknown != PT_None) {
            printf("[FAIL][T33.B] GetProducer(None) = %d (expected PT_None)\n", static_cast<int>(unknown));
            ok = false;
        } else {
            printf("[PASS][T33.B] GetProducer(ResourceType_None) returns PT_None\n");
        }

        // ---- Check 3: GetBuildingTypeForProduction round-trip ----
        struct ProdBuildingPair {
            ProductionType prodType;
            BuildingType expected;
        };

        static const ProdBuildingPair kProdToBld[] = {
            { PT_Woodcutter,  BuildingType_Woodcutter },
            { PT_Sawmill,     BuildingType_Sawmill },
            { PT_Stonemason,  BuildingType_Stonemason },
            { PT_Toolmaker,   BuildingType_Toolmaker },
            { PT_CoalMine,    BuildingType_CoalMine },
            { PT_Fisher,      BuildingType_Fisher },
            { PT_Bakery,      BuildingType_Bakery },
        };

        static const int kProdToBldCount = sizeof(kProdToBld) / sizeof(kProdToBld[0]);

        for (int i = 0; i < kProdToBldCount; ++i) {
            BuildingType result = GetBuildingTypeForProduction(kProdToBld[i].prodType);
            if (result != kProdToBld[i].expected) {
                printf("[FAIL][T33.C] GetBuildingTypeForProduction(prod=%d) = %d (expected %d)\n",
                    static_cast<int>(kProdToBld[i].prodType),
                    static_cast<int>(result),
                    static_cast<int>(kProdToBld[i].expected));
                ok = false;
            }
        }
        if (ok) {
            printf("[PASS][T33.C] GetBuildingTypeForProduction: all %d mappings correct\n", kProdToBldCount);
        }

        // ---- Check 4: GetBuildingTypeForProduction(PT_None) returns BuildingType_None ----
        BuildingType noneResult = GetBuildingTypeForProduction(PT_None);
        if (noneResult != BuildingType_None) {
            printf("[FAIL][T33.D] GetBuildingTypeForProduction(PT_None) = %d (expected BuildingType_None)\n",
                static_cast<int>(noneResult));
            ok = false;
        } else {
            printf("[PASS][T33.D] GetBuildingTypeForProduction(PT_None) returns BuildingType_None\n");
        }

        // ---- Check 5: End-to-end: GetProducer → GetBuildingTypeForProduction chain ----
        {
            ResourceType target = ResourceType_Stone;
            ProductionType prodType = GetProducer(target);
            BuildingType bldType = GetBuildingTypeForProduction(prodType);

            if (prodType == PT_None) {
                printf("[FAIL][T33.E] GetProducer(Stone) returned PT_None — broken chain\n");
                ok = false;
            } else if (bldType == BuildingType_None) {
                printf("[FAIL][T33.E] GetBuildingTypeForProduction(prod=%d) returned None — broken chain\n",
                    static_cast<int>(prodType));
                ok = false;
            } else if (bldType != BuildingType_Stonemason) {
                printf("[FAIL][T33.E] Chain Stone → %d → %d (expected Stonemason=%d)\n",
                    static_cast<int>(prodType), static_cast<int>(bldType),
                    static_cast<int>(BuildingType_Stonemason));
                ok = false;
            } else {
                printf("[PASS][T33.E] Chain: Stone → PT_Stonemason → BuildingType_Stonemason\n");
            }
        }

        if (ok) {
            printf("[PASS] T33: Definition Query API — GetProducer + GetBuildingTypeForProduction verified\n");
        }
        return ok;
    }
};

static T33DefinitionQueryTest g_t33DefinitionQueryTest;

} // namespace World
