#include "../../Testing/ISimulationScenario.h"
#include "../../Testing/SoakHarness.h"
#include <stdio.h>

namespace World {

    class Soak50k : public SoakTestBase {
    public:
        Soak50k() : SoakTestBase("T38", 50000) {}
    };
    static Soak50k g_soak50k;

    class Soak100k : public SoakTestBase {
    public:
        Soak100k() : SoakTestBase("T39", 100000, 2000) {}
    };
    static Soak100k g_soak100k;

    class Soak250k : public SoakTestBase {
    public:
        Soak250k() : SoakTestBase("T40", 250000, 5000) {}
    };
    static Soak250k g_soak250k;

    class Soak500k : public SoakTestBase {
    public:
        Soak500k() : SoakTestBase("T41", 500000, 10000) {}
    };
    static Soak500k g_soak500k;

}
