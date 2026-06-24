#pragma once

namespace World {

class FlagManager;
class RoadManager;
class DemandManager;
class CargoManager;
class Flag;

// BuildContext bundles all world managers that ConstructionSystem needs.
// Passed as a single argument to shrink signatures and reduce coupling.
// IMMUTABLE after construction — pointer members are const.
struct BuildContext {
    FlagManager* const flags;
    RoadManager* const roads;
    DemandManager* const demand;
    CargoManager* const cargo;
    Flag* const warehouse;

    BuildContext()
        : flags(NULL)
        , roads(NULL)
        , demand(NULL)
        , cargo(NULL)
        , warehouse(NULL)
    {
    }

    BuildContext(FlagManager* f, RoadManager* r, DemandManager* d, CargoManager* c, Flag* w)
        : flags(f)
        , roads(r)
        , demand(d)
        , cargo(c)
        , warehouse(w)
    {
    }
};

} // namespace World
