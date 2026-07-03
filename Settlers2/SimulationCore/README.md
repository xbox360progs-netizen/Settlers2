# SimulationCore

Headless simulation engine for Settlers2. Extracted from the main game as a
standalone static library with zero rendering, input, or audio dependencies.

## Public API

| Entry point | Role |
|---|---|
| `Simulation::Simulation(SimulationConfig)` | Create simulation instance |
| `Simulation::LoadWorld(WorldModel)` | Load world data |
| `Simulation::Tick()` | Advance one simulation tick |
| `Simulation::GetState()` | Read-only state snapshot |

## Guarantees

- **Headless** — no graphics, input, or audio dependencies
- **Deterministic** — identical config + world → identical state sequence
- **No exceptions** — compiled with `/EH-`
- **C++03** — compatible with Xbox 360 compiler toolchain
- **STL only** — depends only on standard library and internal components

## Dependencies

- C++ standard library (`<vector>`, `<cassert>`, `<cstdio>`, `<stdint.h>`)
- Internal: `Core/`, `Interfaces/`, `Transport/`, `World/`, `Stubs/`

No third-party, DirectX, XDK, or game engine dependencies.

## Build

Builds for Win32 (MSVC v100) and Xbox 360. Output is `SimulationCore.lib`.

```
SimulationCore/          → static library
SimulationCoreTests/     → test executable (Win32 only, uses GTest-style runner)
```

## Architecture

```
Simulation (coordinator)
  └── TransportController — cargo routing (migrated from game)
  └── EconomySystem       — demand generation (planned)
  └── ConstructionSystem  — building lifecycle (planned)
  └── WorkerSystem        — worker tasks (planned)

Subsystems are migrated from the main game via:
  Interface → Adapter → SimulationCore
pattern proven in PR6–PR9.
```
