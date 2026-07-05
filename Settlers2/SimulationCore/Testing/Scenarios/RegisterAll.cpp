#include "../../Testing/ScenarioRegistry.h"
#include "T1SingleConstruction.cpp"
#include "T2ConcurrentConstruction.cpp"
#include "T6MassConstruction.cpp"
#include "T7LongSoak.cpp"
#include "T8ProductionTest.cpp"
#include "T9RegressionHardening.cpp"
#include "T10PriorityTest.cpp"
#include "T11FairnessSoak.cpp"
#include "T12ProductionPipeline.cpp"
#include "T13ProductionSoak.cpp"
#include "T14MultiInput.cpp"
#include "T15WarehouseTest.cpp"
#include "T16EconomyTest.cpp"
#include "T17WarehouseSoak.cpp"
#include "T18WorkerTest.cpp"
#include "T19WorkerReleaseTest.cpp"
#include "T20WorkerExecutionTest.cpp"
#include "T21WorkerSequentialTest.cpp"
#include "T22SettlementTest.cpp"
#include "T23SettlementConstructionCycle.cpp"
#include "T24BuildingCompletionCycle.cpp"
#include "T25CrossBuildingAI.cpp"
#include "T26QuarryTest.cpp"
#include "T27ForestryFlowTest.cpp"
#include "T28AmbiguityTest.cpp"
#include "T29AmbiguityResolvedTest.cpp"
#include "T30SettlementStorehouseTest.cpp"
#include "T31WoodEconomyTest.cpp"
#include "T32ObservationAPITest.cpp"
#include "T33DefinitionQueryTest.cpp"
#include "T34StoneEconomyTest.cpp"
#include "T35ToolsEconomyTest.cpp"
#include "T36BootstrapTest.cpp"
#include "T37FullAutonomousTest.cpp"
#include "T38SoakTests.cpp"
#include "T42ForestStabilityTest.cpp"
#include "T43HunterTest.cpp"
#include "T44FisherTest.cpp"
#include "T45FoodMineTest.cpp"
#include "T46AgricultureTest.cpp"
#include "T47MiningMetallurgyTest.cpp"
#include "T48MultiInputArchitecturalTest.cpp"
#include "T49EconomicReachabilityTest.cpp"
#include "T50EconomicIndependenceTest.cpp"
#include "T51ClosedWoodLoopTest.cpp"
#include "T52MilitaryTest.cpp"
#include "T53FullEconomySoak.cpp"
#include "T54FullEconomyBootstrapTest.cpp"

namespace World {

    void RegisterAllScenarios()
    {
        static T1SingleConstruction s_t1;
        static T2ConcurrentConstruction s_t2;
        static T6MassConstruction s_t6;
        static T7LongSoak s_t7;
        extern T8ProductionTest g_t8ProductionTest;
        extern T9RegressionHardening g_t9RegressionHardening;
        extern T10PriorityTest g_t10PriorityTest;
        extern T11FairnessSoak g_t11FairnessSoak;
        extern T12ProductionPipeline g_t12ProductionPipeline;
        extern T13ProductionSoak g_t13ProductionSoak;
        extern T14MultiInput g_t14MultiInput;
        extern T15WarehouseTest g_t15WarehouseTest;
        extern T16EconomyTest g_t16EconomyTest;
        extern T17WarehouseSoak g_t17WarehouseSoak;
        extern T18WorkerTest g_t18WorkerTest;
        extern T19WorkerReleaseTest g_t19WorkerReleaseTest;
        extern T20WorkerExecutionTest g_t20WorkerExecutionTest;
        extern T21WorkerSequentialTest g_t21WorkerSequentialTest;
        extern T22SettlementTest g_t22SettlementTest;
        extern T23SettlementConstructionCycle g_t23SettlementConstructionCycle;
        extern T24BuildingCompletionCycle g_t24BuildingCompletionCycle;
        extern T25CrossBuildingAI g_t25CrossBuildingAI;
        extern T26QuarryTest g_t26QuarryTest;
        extern T27ForestryFlowTest g_t27ForestryFlowTest;
        extern T28AmbiguityTest g_t28AmbiguityTest;
        extern T29AmbiguityResolvedTest g_t29AmbiguityResolvedTest;
        extern T30SettlementStorehouseTest g_t30SettlementStorehouseTest;
        extern T31WoodEconomyTest g_t31WoodEconomyTest;
        extern T32ObservationAPITest g_t32ObservationAPITest;
        extern T33DefinitionQueryTest g_t33DefinitionQueryTest;
        extern T34StoneEconomyTest g_t34StoneEconomyTest;
        extern T35ToolsEconomyTest g_t35ToolsEconomyTest;
        extern T36BootstrapTest g_t36BootstrapTest;
        extern T37FullAutonomousTest g_t37FullAutonomousTest;
        extern Soak50k g_soak50k;
        extern Soak100k g_soak100k;
        extern Soak250k g_soak250k;
        extern Soak500k g_soak500k;
        extern T42ForestStabilityTest g_t42;
        extern T43HunterTest g_t43;
        extern T44FisherTest g_t44;
        extern T45FoodMineTest g_t45;
        extern T46AgricultureTest g_t46;
        extern T47MiningMetallurgyTest g_t47;
        extern T48MultiInputArchitecturalTest g_t48;
        extern T49EconomicReachabilityTest g_t49;
        extern T50EconomicIndependenceTest g_t50;
        extern T51ClosedWoodLoopTest g_t51;
        extern T52MilitaryTest g_t52;
        extern T53FullEconomySoak g_t53;
        extern T54FullEconomyBootstrapTest g_t54;

        ScenarioRegistry::Register(&s_t1);
        ScenarioRegistry::Register(&s_t2);
        ScenarioRegistry::Register(&s_t6);
        ScenarioRegistry::Register(&s_t7);
        ScenarioRegistry::Register(&g_t8ProductionTest);
        ScenarioRegistry::Register(&g_t9RegressionHardening);
        ScenarioRegistry::Register(&g_t10PriorityTest);
        ScenarioRegistry::Register(&g_t11FairnessSoak);
        ScenarioRegistry::Register(&g_t12ProductionPipeline);
        ScenarioRegistry::Register(&g_t13ProductionSoak);
        ScenarioRegistry::Register(&g_t14MultiInput);
        ScenarioRegistry::Register(&g_t15WarehouseTest);
        ScenarioRegistry::Register(&g_t16EconomyTest);
        ScenarioRegistry::Register(&g_t17WarehouseSoak);
        ScenarioRegistry::Register(&g_t18WorkerTest);
        ScenarioRegistry::Register(&g_t19WorkerReleaseTest);
        ScenarioRegistry::Register(&g_t20WorkerExecutionTest);
        ScenarioRegistry::Register(&g_t21WorkerSequentialTest);
        ScenarioRegistry::Register(&g_t22SettlementTest);
        ScenarioRegistry::Register(&g_t23SettlementConstructionCycle);
        ScenarioRegistry::Register(&g_t24BuildingCompletionCycle);
        ScenarioRegistry::Register(&g_t25CrossBuildingAI);
        ScenarioRegistry::Register(&g_t26QuarryTest);
        ScenarioRegistry::Register(&g_t27ForestryFlowTest);
        ScenarioRegistry::Register(&g_t28AmbiguityTest);
        ScenarioRegistry::Register(&g_t29AmbiguityResolvedTest);
        ScenarioRegistry::Register(&g_t30SettlementStorehouseTest);
        ScenarioRegistry::Register(&g_t31WoodEconomyTest);
        ScenarioRegistry::Register(&g_t32ObservationAPITest);
        ScenarioRegistry::Register(&g_t33DefinitionQueryTest);
        ScenarioRegistry::Register(&g_t34StoneEconomyTest);
        ScenarioRegistry::Register(&g_t35ToolsEconomyTest);
        ScenarioRegistry::Register(&g_t36BootstrapTest);
        ScenarioRegistry::Register(&g_t37FullAutonomousTest);
        ScenarioRegistry::Register(&g_soak50k);
        ScenarioRegistry::Register(&g_soak100k);
        ScenarioRegistry::Register(&g_soak250k);
        ScenarioRegistry::Register(&g_soak500k);
        ScenarioRegistry::Register(&g_t42);
        ScenarioRegistry::Register(&g_t43);
        ScenarioRegistry::Register(&g_t44);
        ScenarioRegistry::Register(&g_t45);
        ScenarioRegistry::Register(&g_t46);
        ScenarioRegistry::Register(&g_t47);
        ScenarioRegistry::Register(&g_t48);
        ScenarioRegistry::Register(&g_t49);
        ScenarioRegistry::Register(&g_t50);
        ScenarioRegistry::Register(&g_t51);
        ScenarioRegistry::Register(&g_t52);
        ScenarioRegistry::Register(&g_t53);
        ScenarioRegistry::Register(&g_t54);
    }

}
