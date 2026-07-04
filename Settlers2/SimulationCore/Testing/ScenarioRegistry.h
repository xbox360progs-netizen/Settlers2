#pragma once
#include "ISimulationScenario.h"

namespace World {

    class ScenarioRegistry {
    public:
        static void Register(ISimulationScenario* scenario);
        static ISimulationScenario* Find(const char* name);
        static int GetCount();
        static ISimulationScenario* GetAt(int index);
        static void ListAll();

    private:
        static const int kMaxScenarios = 64;
        static int s_count;
        static ISimulationScenario* s_scenarios[kMaxScenarios];
    };

}
