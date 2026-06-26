#pragma once

namespace World {

class FlagManager;
class RoadManager;
class DemandManager;
class CargoManager;
class CarrierManager;
class ObjectLifecycleManager;
class Map;
class Flag;

// BuildContext bundles all world managers that ConstructionSystem needs.
// Passed as a single argument to shrink signatures and reduce coupling.
// IMMUTABLE after construction — pointer members are const.
struct BuildContext {
    FlagManager* const flags;
    RoadManager* const roads;
    DemandManager* const demand;
    CargoManager* const cargo;
    CarrierManager* const carriers;
    Map* const map;
    Flag* const warehouse;

    BuildContext()
        : flags(NULL)
        , roads(NULL)
        , demand(NULL)
        , cargo(NULL)
        , carriers(NULL)
        , map(NULL)
        , warehouse(NULL)
    {
    }

    BuildContext(FlagManager* f, RoadManager* r, DemandManager* d, CargoManager* c,
                 CarrierManager* cr, Map* mp, Flag* w)
        : flags(f)
        , roads(r)
        , demand(d)
        , cargo(c)
        , carriers(cr)
        , map(mp)
        , warehouse(w)
    {
    }
};

} // namespace World
