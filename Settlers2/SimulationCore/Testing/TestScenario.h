#pragma once

namespace World {

    class Simulation;
    struct WorldModel;

    void ListScenarios();
    bool RunScenario(const char* name, Simulation& sim, WorldModel& world);

}
