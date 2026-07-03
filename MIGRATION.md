# Phase 8 — Migration

## 8.1 — Demand → TransportTask adapter (bridge)
- [ ] DemandManager creates TransportTasks (old pipeline still alive)
- [ ] Invariant: one Demand = at most one active TransportTask
- [ ] CargoManager reports delivery completion to DemandManager

## 8.2 — Resource ownership migration
- [ ] Ownership chain enforced: Ground → Flag → Task → Carrier → Building
- [ ] Runtime audit: `[Resource] id=712 owner=TransportTask(17)` (debug)
- [ ] Remove old ownership paths (Reserve/Allocate)

## 8.3 — Parallel validation mode
- [ ] old TransportJobManager = observe only
- [ ] new TransportController = execute
- [ ] Log: `[MIGRATION] demand=81 old=flag12 new=flag12 OK`

## 8.3.5 — Soak tests (destructive first)
- [ ] T4: Road break — retry blocked tasks, no orphaned TransportTask/Cargo
- [ ] T5: Flag deletion during active transport — cancel task, restore demand, no Cargo leak
- [ ] T6: Cancel during PickUp
- [ ] T7: Cancel during Move
- [ ] T2: 10 simultaneous construction sites (producer + consumer scaling)
- [ ] T3: Long road chains — multiple AdvanceHop across 5+ flags
- [ ] T8: 30–60 min gameplay — telemetry clean (mask/res/ownHash/blocked)
- [ ] T1: Full regression — single resource, single carrier (baseline sanity)

## 8.4 — Remove legacy transport
- [ ] All scenarios pass: wood→warehouse, warehouse→construction, mine→smelter, food→worker, blocked recovery, flag deletion
- [ ] Same save → old and new produce equivalent resource distribution (± delivery timing)
- [ ] Remove TransportJobManager, DemandTicket, `kUseTransportJobs`, old Carrier routing
- [ ] Transport tests green

---

## Current Status — Full Cycle Verified ✅

### What works (end-to-end confirmed via log analysis)

```
ConstructionSite → DemandManager → TransportController → Carrier → Flag → CheckDeliveries → ConstructionSite → Builder → Completed
```

Full single-site cycle confirmed: Woodcutter at (20,33) — dispatch, walk, build, resource delivery (3/3 Wood), completion, builder return, site removal.

Multi-site with shared roads: Hunter at (18,38) received all 3 Wood via road 2 carrier. Fisher at (24,30) transport chain functional after Reserve fix.

### Recent fixes

| Fix | File | What |
|-----|------|------|
| `HasDemandFromOtherFlag` | `DemandManager.h:41`, `DemandManager.cpp:355` | Carrier idle check skips demands targeting the same flag |
| `Reserve` same-flag filter | `DemandManager.cpp:184-196` | Prevents warehouse demand (priority 10) from blocking construction demand (priority 5) at the same flag |
| Carrier idle uses `HasDemandFromOtherFlag` | `Carrier.h:250` | Prevents wasted wake → walk → Reserve → Release cycles |

### Remaining issues

- **OVERDELIVER telemetry** (`delivered=3/1`): artifact of `SetDemand(woodMissing)` overwriting `requested` downward each frame. Harmless — delivery and ticket release complete normally.
- **Builder dispatch timing**: Builder waits at `"no road to site"` until player builds road. This is correct Settlers behavior.

---

## Stabilization Checklist

### Logistics
- [x] Building receives all required resources (confirmed: Woodcutter 3/3, Hunter 3/3)
- [ ] Production buildings get input resources
- [x] Warehouses collect only truly free resources
- [ ] Flag deletion leaves no orphaned resources
- [ ] No DemandTicket pool asserts triggered
- [ ] No DemandTicket leaks on map clear / return to menu

### Construction
- [x] Open build menu
- [x] Select any building
- [x] Place building
- [x] Wood delivery
- [ ] Stone delivery
- [x] Construction completion (confirmed: Woodcutter, Hunter)

---

## Equivalence Criteria (for 8.4)

**Strong invariants** (must be identical):
- Final resource counts at each building/flag
- Destination inventories (what arrived where)
- Construction completion (same buildings finish)
- Blocked recovery result (same resources reachable)

**Weak invariants** (allowed to differ):
- Delivery timestamps (timing shifts are fine)
- Carrier identity (who carried is irrelevant)
- Hop count (new route may differ)
- Queue order (priority dispatching reorders fairly)

**Conservation invariant** (global — applies to whole economy):
```
Σ(world resources) before migration == Σ(world resources) after migration
```

**Migration complete** ⇔ new transport produces economically equivalent world state without ownership violations.
