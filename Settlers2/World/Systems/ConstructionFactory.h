#pragma once
#include "../ConstructionSite.h"
#include "../Flag.h"

namespace World {

class FlagManager;

struct BuildCommand {
    BuildingType type;
    int tileX, tileY;
    Flag* entranceFlag;       // NULL = create new flag at calculated entrance position
    bool autoConnectRoad;     // reserved — road connection owned by GameScene for now

    BuildCommand() : type(Building_None), tileX(0), tileY(0), entranceFlag(NULL), autoConnectRoad(true) {}
};

// ConstructionFactory is the single place where BuildCommands become ConstructionSites.
// It owns the knowledge of how to turn a build request into an in-world entity:
//   - create the entrance flag if none provided
//   - instantiate the ConstructionSite
// ConstructionSystem coordinates the factory + manager + event dispatch.
class ConstructionFactory {
public:
    ConstructionFactory();

    // Optional: provide FlagManager after default construction (for member composition).
    explicit ConstructionFactory(FlagManager* flagManager);
    void SetFlagManager(FlagManager* flagManager);

    // Create a ConstructionSite from a BuildCommand.
    // Returns NULL and leaves the flag unmodified on failure.
    ConstructionSite* Create(const BuildCommand& cmd);

    // Set the default entrance offset for buildings without explicit flag placement.
    // Currently hardcoded to (1,0) — override via config or atlas data later.
    void SetDefaultEntranceOffset(int dx, int dy);

private:
    FlagManager* m_flagManager;
    int m_defaultEntranceDx;
    int m_defaultEntranceDy;
};

} // namespace World
