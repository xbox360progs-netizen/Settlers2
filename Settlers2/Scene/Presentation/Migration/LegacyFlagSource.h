#pragma once
#include "IFlagSource.h"

// Temporary — wraps legacy World::FlagManager for migration.
// Remove when all PresentationSystems read from SimulationCore.

namespace World {
    class FlagManager;
}

namespace Scene {

class LegacyFlagSource : public IFlagSource {
public:
    void SetFlagManager(World::FlagManager* flagManager);

    virtual uint32_t GetFlagCount() const;
    virtual bool GetFlag(uint32_t index, FlagView& out) const;

private:
    World::FlagManager* m_flagManager;
};

} // namespace Scene
