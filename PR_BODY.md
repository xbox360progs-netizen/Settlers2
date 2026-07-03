## SimulationCore Extraction (PR6–PR9)

### Summary

This is an infrastructure refactoring that extracts the transport simulation into a standalone SimulationCore library. No gameplay behavior is intentionally changed.

The migration introduces interface-based dependencies, keeps the legacy implementation operational, and adds an isolated test project to validate the new core independently from the game.

### Changes

**PR6 — SimulationCore Foundation**
- Created SimulationCore project.
- Moved core transport types into the new library.
- Added forward-declared TransportController interface.
- Updated solution and project files.

**PR7 — Interface Layer**
- Introduced abstraction interfaces: IRoadGraph, IFlagInventory, ICargoRepository, IDemandService.
- Added adapter layer under `World/Adapters/` to bridge the existing game managers to the new interfaces.

**PR8 — Controller Migration**
- Implemented the new interface-based TransportController.
- Key points:
    - No dependency on World managers.
    - No dependency on scene classes.
    - Uses only SimulationCore interfaces.
- Legacy `World::TransportController` implementation remains unchanged for compatibility and telemetry.

**PR9 — Test Infrastructure**
- Added standalone SimulationCoreTests project.
- Coverage includes: Handle, TransportTask, TransportRoute, TransportController verification, DemandManager verification.
- The tests validate SimulationCore independently of the game executable.

### Architecture

**Before:**
```
TransportController
    ↓
Concrete World managers
```

**After:**
```
SimulationCore
    ↓
Interfaces
    ↓
World Adapters
    ↓
Existing World managers
```

World now depends on SimulationCore through interface adapters rather than SimulationCore depending on World.

### Compatibility
- Existing game behavior is preserved.
- Legacy TransportController remains available.
- Adapter layer enables gradual migration.
- No gameplay functionality is intentionally changed.

### Benefits
- Dependency inversion.
- Standalone simulation library.
- Independent unit testing.
- Clear separation between simulation and game layers.
- Foundation for future multithreading.
- Foundation for platform-independent simulation.

### Verification
- Solution builds successfully.
- Legacy implementation preserved.
- SimulationCore builds independently.
- All SimulationCoreTests pass.
- Interface bridge validated through adapter layer.

### Follow-up
The same extraction pattern can be applied to additional simulation systems in future work.
