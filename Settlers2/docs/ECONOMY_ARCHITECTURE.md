# Economy Architecture — Contract

## Three Fundamental Relationship Types

```
Renewable     (World → Resource)      ✅ Trees, Animals, Fish
Transformation (Resource → Resource)  ✅ Sawmill, Toolmaker, Bakery...
Consumption   (Resource → Enables)    ✅ Mines ← Food
```

Every economic activity in Settlers II decomposes into one or more of these types.
No fourth type has been required from T31 to T45. No fourth type is expected.

## BuildingDefinition — Composition of Aspects

```
Building(BuildingType)
  ├── ConstructionDefinition   (build cost, build time)
  ├── ProductionDefinition     (inputs → outputs, cycleTime)
  ├── ConsumptionDefinition    (food requirements — mines only)
  ├── RenewableResourceDef     (tree/animal/fish interaction)
  └── EmploymentDefinition     (worker slots — future)
```

Each aspect is defined in a separate Definition table, keyed by `BuildingType`.
A building may participate in zero or more aspects.
Systems read only their own aspect. No system reads BuildingType.

## System Responsibility Map

| System | Reads | Writes | Never |
|--------|-------|--------|-------|
| ProductionSystem | `ProductionDefinition`, `BuildingDefinition` | `outputBuffer`, `totalOutput` | switch on `BuildingType` |
| ConsumptionSystem | `ConsumptionDefinition`, `DeliveryEvent` | `fed`, `foodStored` | touch production logic |
| RenewableResourceSystem | `RenewableResourceDefinition` | `tree*`, `animalCount`, `fishCount` | know which building type called it |
| DemandManager | `TransportRequest` | demand lifecycle | interpret request content |
| WarehouseSystem | `TransportNode.buffer` | stockpile | read `outputBuffer`, write `outputBuffer` |
| EconomySystem | `totalOutput`, definitions | metrics | mutate world state |
| SettlementSystem | definitions, economy state | `Job`, `Demand` | create `TransportTask` directly |

## Economy Core Freeze — Invariants

1. **No system switches on `BuildingType`.** Building-specific behaviour is expressed through Definition data, not code.
2. **ProductionSystem contains zero special cases.** No `if (Woodcutter)`, no `if (Forester)`, no `if (Hunter)`. All renewable resource interaction goes through `RenewableResourceSystem::OnProductionCycle()`.
3. **ConsumptionSystem is data-driven.** Mine → food mapping lives in `ConsumptionDefinition` table. Adding a new mine or food type = data change, not code change.
4. **RenewableResourceSystem is data-driven.** Population model (`StagedTrees` / `SimplePopulation`), capacity, regeneration rate live in `RenewableResourceDefinition`.
5. **BuildingType is an identifier, not logic.** No system derives behaviour from the numeric value of `BuildingType`.

## Adding a New Production Building — the Checklist

If the checklist requires opening a core system file, the freeze is violated.

1. Add `ResourceType` (if new) to `Core/ResourceTypes.h`
2. Add `ProductionType` (if new) to `Core/ProductionTypes.h`
3. Add `BuildingType` to `Core/BuildingTypes.h`
4. Add row to `BuildingDefinition` table (`Definitions/BuildingDefinition.cpp`)
5. Add row to `ProductionDefinition` table (`Definitions/ProductionDefinition.cpp`)
6. Add row to `ConsumptionDefinition` table if mine (`Definitions/ConsumptionDefinition.cpp`)
7. Add row to `RenewableResourceDefinition` if renewable (`Definitions/RenewableResourceDefinition.cpp`)
8. Write integration test for the new circuit
9. Register test in `RegisterAll.cpp`

**Violation alert:** If any file outside `Definitions/`, `Core/` (enums), or `Testing/` is modified, the freeze has been broken.

## Complete Economic Graph — Definition

The economy is a directed graph:
- Nodes = `ResourceType`
- Edges = `ProductionDefinition` (consumes → produces)
- Source nodes = renewable resources (Trees, Animals, Fish)
- Sink nodes = final consumption (Weapons, Soldiers, Bread)

**Complete** means: every non-renewable resource is reachable from at least one renewable source through a path of `ProductionDefinition` edges.

## Verified Architectural Properties (T46–T50)

The following properties have been proven through integration tests and static analysis.
No changes to ProductionSystem, ConsumptionSystem, or RenewableResourceSystem were
required for any of these tests.

| Property | Test | What It Proves |
|----------|------|----------------|
| New industry (linear) | T46 | Farm→Mill→Bakery→Bread via Definitions only |
| New mine type | T47 | GoldMine added via ConsumptionDefinition — third mine (Coal, Iron, Gold) |
| Multi-input production | T48 | IronBar + Coal → Weapons — `allDelivered` gate handles >1 input atomically |
| Static reachability | T49 | Every produced resource reachable from renewables; no orphan cycles; unique producer; no dangling resources |
| Economic independence | T50 | Removing any single industry degrades only its transitive dependencies |

### Multi-Input Invariant (T48 Verified)

```
Partial input → no production
Other partial → no production
Both inputs  → produces output
After cycle  → both inputs consumed atomically
```

`ProductionSystem::ProcessProduction` handles 0, 1, or 2+ inputs through the same
`allDelivered` gate. No special cases, no per-input count checks, no conditional logic.
The `consumes[4]` array in `ProductionDefinition` is the single source of truth.

### Reachability Invariants (T49 Verified)

1. **Producer invariant**: each produced resource has `GetProducer ≠ PT_None`
2. **Reachability invariant**: each chain is reachable from a renewable source (transitive closure)
3. **Cycle invariant**: no cycle exists without a renewable source (detected by visited[] guard)
4. **Dangling invariant**: every intermediate resource is consumed by at least one definition
5. **Uniqueness invariant**: each resource has exactly one `GetProducer` result
6. **Usage invariant**: every `ProductionDefinition` entry with a `BuildingDefinition` mapping produces at least one non-None resource (Forester is the documented exception — it produces None and only plants trees)

This is a **static graph analysis** — it runs without a simulation. Any addition to
`ProductionDefinition` is verified before a single tick executes.

### Independence Invariants (T50 Verified)

For each industry group (Forestry, Quarrying, Hunting, Fishing, Tools, Mining,
Agriculture, Metallurgy):

1. Removing the industry degrades only its transitive dependencies
2. Resources with remaining producers outside the removed set stay reachable
3. No new orphan cycles appear
4. The remaining graph satisfies all Reachability invariants

**Architectural consequence:** industries are modular. Adding or removing an entire
production chain cannot corrupt the economic graph outside its dependency cone.

## Development Model — Post Freeze

Before freeze:
```
Architecture → Architecture → Architecture
```

After freeze:
```
Gameplay → Gameplay → Gameplay
```

Each new industry (Agriculture, Mining, Metallurgy, Military) follows the same pattern:

1. Add Definition entries
2. Write circuit test
3. Run Economic Reachability Test — graph remains complete
4. No core system changes

## Existing Industries

| Industry | Buildings | Status | Verified By |
|----------|-----------|--------|-------------|
| Forestry | Woodcutter, Forester, Sawmill | ✅ | T42, T49, T50 |
| Quarrying | Stonemason | ✅ | T26, T49, T50 |
| Hunting | Hunter | ✅ | T43, T49, T50 |
| Fishing | Fisher | ✅ | T44, T49, T50 |
| Tools | Toolmaker | ✅ | T14, T49, T50 |
| Mining | CoalMine, IronMine, GoldMine | ✅ (with Consumption) | T45, T47, T49, T50 |
| Agriculture | Farm, Mill, Bakery | ✅ | T46, T49, T50 |
| Metallurgy | IronSmelter, WeaponSmith | ✅ (multi-input) | T48, T49, T50 |
| Military | — | □ | — |
