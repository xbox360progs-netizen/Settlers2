#include "ScenarioRegistry.h"
#include <stdio.h>
#include <string.h>

namespace World {

    int ScenarioRegistry::s_count = 0;
    ISimulationScenario* ScenarioRegistry::s_scenarios[kMaxScenarios];

    void ScenarioRegistry::Register(ISimulationScenario* scenario)
    {
        if (s_count < kMaxScenarios) {
            s_scenarios[s_count++] = scenario;
        }
    }

    ISimulationScenario* ScenarioRegistry::Find(const char* name)
    {
        for (int i = 0; i < s_count; ++i) {
            const char* a = s_scenarios[i]->GetName();
            const char* b = name;
            while (*a && *b && *a == *b) { ++a; ++b; }
            if (*a == 0 && *b == 0) return s_scenarios[i];
        }
        return NULL;
    }

    int ScenarioRegistry::GetCount()
    {
        return s_count;
    }

    ISimulationScenario* ScenarioRegistry::GetAt(int index)
    {
        if (index >= 0 && index < s_count)
            return s_scenarios[index];
        return NULL;
    }

    void ScenarioRegistry::ListAll()
    {
        printf("Available scenarios:\n");
        for (int i = 0; i < s_count; ++i) {
            printf("  %s\n", s_scenarios[i]->GetName());
        }
    }

}
