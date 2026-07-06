# Demand Layer — Domain Contract

## Scope

### In scope

- **DemandManager** — full demand lifecycle (publish, cancel, update, deduplicate, complete)
- **TransportController** — inter-node dispatch, route resolution, carrier assignment
- **Carrier** — spatial execution with source pickup and destination delivery
- **Inter-node routing** — resource movement between TransportNodes over road graph
- **AcceptingFlagInventory replacement** — proper source node buffer decrement on pickup

### Out of scope

- **Local distribution** — `LocalTransferSystem` is sealed (Phase 2 freeze). Demand Layer never decides which local building gets which resource.
- **TransportNode contract** — unchanged from Phase 2. Node is a passive buffer + attachment registry. It does not evaluate deficits (LTS owns that).
- **Pathfinding** — `IRoadGraph` is an external dependency. TransportController calls `FindRoute`, does not implement it.
- **WarehouseSystem** — unchanged from Phase 2. Observes TransportNode buffer via `ScanTransportBuffers`. Never reads or writes `outputBuffer`.

## Demand Lifecycle

```
LocalTransferSystem
       │
       │  (Tick: evaluate deficit from buffer + building.inputDelivered)
       ▼
TransportNode.pendingDemand[]
       │
       │  (DemandManager reads pendingDemand in its own Tick)
       ▼
DemandManager
       │
       │  (deduplicates, creates/updates/cancels demand entries)
       │
       ├── PublishTransportRequests() → WorldModel.pendingRequests[]
       │
       ▼
TransportController.CreateTask()
       │
       │  (builds route via IRoadGraph, creates TransportTask)
       │
       ▼
Task enqueued at source TransportNode
       │
       ▼
Dispatcher picks next task  (priority + age, same as v2)
       │
       ▼
Carrier assigned → Pickup → Travelling → Deliver
       │                                           │
       │    Carrier::Pickup:                        │
       │      sourceNode.buffer.Remove(res, amt)    │
       │      cargo = new Cargo(res, amt)           │
       │                                           │
       ▼                                           │
    Travelling  ←─── route hops ─────────→  Deliver
                                               │
                                  destNode.ReceiveCargo(res, amt)
                                  DemandManager.CompleteDemand(ticket)
                                  cargo deleted
                                  carrier returns to Idle
```

## Ownership Table

| State | Created by | Modified by | Completed by |
|-------|-----------|-------------|-------------|
| `pendingDemand[]` entry | LocalTransferSystem (Tick) | LocalTransferSystem (next Tick overwrite) | LocalTransferSystem (next Tick — clears, re-evaluates) |
| `Demand` entry | DemandManager (from pendingDemand) | DemandManager (UpdateDemand, partial fulfillment via CompleteDemand) | DemandManager (when remaining=0, after final CompleteDemand) |
| `TransportRequest` | DemandManager (PublishTransportRequests) | — | Simulation::ProcessTransportRequests (creates task, marks fulfilled) |
| `TransportTask` | TransportController (CreateTask) | TransportController (state transitions) | TransportController (completeDelivery → FreeTask) |
| `Carrier` | TransportController (on dispatch) | TransportController (Update, state machine) | TransportController (on delivery complete, carrier reclaimed) |
| `Cargo` | TransportController (on pickup) | — | TransportController (on delivery, deleted) |

## Ownership Boundary

After publication, Demand has exactly one owner at each lifecycle stage:

```
LocalTransferSystem
      │  writes pendingDemand[]  (Tick N)
      ▼
DemandManager
      │  reads pendingDemand, creates/updates Demand entries  (Tick N)
      │  publishes TransportRequest  (Tick N)
      ▼
TransportController
      │  creates TransportTask, dispatches Carrier  (Tick N)
      ▼
Carrier
      │  picks up, transports, delivers  (Tick N+1..N+k)
      ▼
DemandManager
      │  receives CompleteDemand(ticket) from TransportController
      ▼
Demand removed
```

### Rule

Once `LocalTransferSystem` writes `pendingDemand[]` and `DemandManager` reads it to create a `Demand` entry, **LocalTransferSystem no longer owns that demand**. It may clear and re-evaluate `pendingDemand[]` next tick (overwriting), but it never directly modifies `DemandManager`'s internal `Demand` state.

The handoff is one-directional:

```
LTS → pendingDemand[]  →  DemandManager reads  →  LTS may overwrite pendingDemand[]
                                                      ↑
                                            (only if LTS re-evaluates next tick;
                                             does NOT touch DemandManager internals)
```

DemandManager decides:
- when to create a `Demand` entry (first time pendingDemand appears)
- when to update it (accumulated via SetDemand from any source)
- when to cancel it (explicit ClearDemand, or node detached)
- when to remove it (remaining == 0 after CompleteDemand)

### Clarification: `pendingDemand` is a cache

`pendingDemand[]` is an **ephemeral buffer** owned by `TransportNode`. It is written by `LocalTransferSystem` every tick (overwritten, not accumulated) and read by `DemandManager` every tick.

```
pendingDemand is a cache. TransportNode owns it.
DemandManager owns Demand.
pendingDemand is NOT Demand.
```

**Consequences:**
- `pendingDemand` has no lifetime beyond one tick. If DemandManager doesn't read it (e.g., tick ordering changes), the data is silently overwritten.
- `pendingDemand` does not accumulate — LTS clears and re-evaluates the full deficit every tick.
- `DemandManager` is the authority on what demand actually exists. `pendingDemand` is merely the recommendation for the current tick.
- Systems must never inspect `TransportNode::pendingDemand` for decision-making outside of `LocalTransferSystem` (writer) and `DemandManager` (reader).

## DemandManager API

```cpp
class DemandManager : public ISimulationSystem {
    // Lifecycle
    void Tick(WorldModel& world);

    // Demand management — called by LocalTransferSystem / domain systems
    void SetDemand(
        ResourceType type,
        uint32_t amount,
        FlagId targetFlag,
        int priority,              // from PriorityForReason
        DemandOwner owner,
        TransportTaskReason reason);

    void ClearDemand(FlagId targetFlag);
    void ClearDemand(ResourceType type, FlagId targetFlag);

    // Demand lifecycle
    // Called by TransportController when delivery completes
    void CompleteDemand(uint32_t observerTicketId);

    // Called by Simulation when a task is created for a demand
    void OnTaskCreated(int demandIndex, TransportTaskId taskId);

    // Internal — converts active demands to transport requests
    void PublishTransportRequests(WorldModel& world);

    // Query — no side effects
    int GetDemandCount() const;
    ResourceType GetDemandType(int index) const;
    uint32_t GetDemandRemaining(int index) const;
    DemandOwner GetDemandOwner(int index) const;
    TransportTaskReason GetDemandReason(int index) const;
};
```

### Contract

| Method | Precondition | Postcondition |
|--------|-------------|---------------|
| `SetDemand(type, amt, flag, pri, owner, reason)` | — | Demand for `(type, flag)` exists or is created, `remaining += amount`. If `activeTask == 0`, eligible for publication next tick. |
| `ClearDemand(flag)` | — | All demands targeting `flag` are removed. |
| `ClearDemand(type, flag)` | — | Demand for `(type, flag)` is removed if exists. |
| `CompleteDemand(ticket)` | `ticket` refers to an active demand | `demand.remaining -= 1`. If `remaining == 0`: demand is removed, `activeTask` cleared. If `remaining > 0`: `activeTask` cleared, demand eligible for re-publication. |
| `OnTaskCreated(index, taskId)` | `index` is a valid demand index, `GetRemaining(index) > 0` | `demands[index].activeTask = taskId`. Prevents duplicate publication for same demand. |

### Deduplication rule

`SetDemand(type, amount, flag)` with the same `(type, flag)` as an existing demand **accumulates**: `remaining += amount`. The demand is NOT re-created — only the remaining count increases. This prevents duplicate entries when multiple nodes or multiple ticks publish demand for the same resource and destination.

### Publication rule

`PublishTransportRequests` iterates active demands where `remaining > 0 && activeTask == 0`. For each, it creates ONE `TransportRequest` and sets `activeTask` to a sentinel value (preventing re-publication until the task completes or fails). `CompleteDemand` clears `activeTask`, allowing re-publication.

This ensures at most one in-flight task per demand at any time, while remaining accurately tracks total needed quantity.

## TransportController

### What it does

- **Task creation**: `CreateTask(res, origin, dest, reason)` — allocates a task from the fixed pool, builds a route via `IRoadGraph`, enqueues at the origin node.
- **Dispatch**: picks the highest-scored waiting task (priority + age), assigns a Carrier, initiates pickup.
- **Carrier lifecycle**: drives the carrier through Pickup → Travelling → Delivering states.
- **Delivery**: on arrival at destination, calls `destNode.ReceiveCargo(res, amount)`, records delivery, calls `DemandManager.CompleteDemand(ticket)`, frees the carrier and task.
- **Route management**: `AdvanceHop` moves the carrier one edge along the route. `IsLastHop` checks if the next hop reaches the destination.

### What it explicitly does NOT do

- **Does NOT create demand.** DemandManager is the sole publisher. TransportController only receives demands and executes them.
- **Does NOT decide cargo destination.** The task's `origin`/`destination` are set at creation time by DemandManager (from the demand entry). Carrier never changes destination.
- **Does NOT evaluate deficits.** Deficit evaluation is owned by LocalTransferSystem. TransportController never inspects `building.inputDelivered` or `node.pendingDemand`.
- **Does NOT prioritise by domain.** Priority is set by `PriorityForReason` at demand creation time. TransportController uses `basePriority + age` (domain-agnostic).
- **Does NOT interpret reasons.** `TransportTaskReason` is a tag used by telemetry only. TransportController never branches on reason.

### Key invariants

```
1. TransportController never calls SetDemand.
2. TransportController never reads pendingDemand.
3. TransportController never modifies outputBuffer.
4. TransportController never inspects building state.
5. TransportController never chooses which demand to fulfill — DemandManager decides what to publish, Dispatcher decides which published task to execute next.
6. TransportController never observes buildings. It sees only:
   - DemandManager (for demand queries and completion callbacks)
   - TransportNode (for buffer operations at pickup/delivery)
   - Carrier (for state machine lifecycle)
   Any code path that checks `building->Needs(...)` or `building->HasOutput()` inside TransportController is a violation of this layer boundary.
```

## Reservation Model

**Explicit decision: NO reservation.**

Neither DemandManager, TransportController, nor Dispatcher reserves resources in the source node buffer. Reservation is deferred to the last possible moment — carrier pickup.

### Why no reservation

| Approach | Problem |
|----------|---------|
| **Reserve at task creation** | Resource sits idle while carrier en route. If multiple tasks target the same source node, they deadlock on each other's reservations. |
| **Reserve at dispatch** | Same problem — resources held unnecessarily, reduced throughput. |
| **Reserve at pickup** | Carrier checks buffer atomically at pickup. If insufficient: retry. If still insufficient after `kMaxPickupRetries`: cancel, demand re-queued. |

### Consequence

A race exists: LocalTransferSystem may drain the source node buffer (local supply) between task creation and carrier pickup. This is **intentional** — local supply takes priority over remote transport:

```
Tick N:     DemandManager creates task (target: nodeA needs Wood)
Tick N..M:  LocalTransferSystem supplies local consumer from nodeB (source node)
            → nodeB.buffer.Count(Wood) drops below task amount
Tick M+1:   Carrier arrives at nodeB → Pickup → buffer.Count(Wood) < amount
            → retry (up to kMaxPickupRetries)
            → if still insufficient: CancelTask(), ClearDemand() via LTS re-evaluation
```

This ensures **local consumption is never blocked by a remote transport assignment**. The carrier retry/cancel mechanism resolves the conflict automatically.

### Expectation

**Carrier may arrive and find source empty. This is expected behavior.**

This is not a bug, not a race condition that needs fixing. It is a direct consequence of the no-reservation design. The carrier retry/cancel loop is the mechanism that resolves the conflict — not a mitigation for a flaw.

| What happens | Is this a bug? |
|-------------|----------------|
| Carrier arrives, source has full amount | Normal case |
| Carrier arrives, source has partial amount | Normal (retry or re-issue) |
| Carrier arrives, source has zero | Normal (retry → cancel → re-issue) |
| Carrier keeps retrying indefinitely | Bug — `kMaxPickupRetries` must fire |

### Guard

`CompleteDemand` is called by TransportController on successful delivery. If the carrier cancels (retries exhausted), `CancelDemand` is called instead, decrementing `remaining` by the undelivered amount. This prevents demand "resurrection" — the same demand doesn't linger after the carrier gives up.

## Carrier State Machine

### Principle — Carrier is a maximally dumb executor

Carrier knows only two things:

```
Pickup X units     → sourceNode.buffer.Remove(res, amt)
Deliver X units    → destNode.ReceiveCargo(res, amt)
```

Carrier does NOT know:

- **why** the demand exists (reason, domain origin, priority)
- **what** the source building produces (building type, production cycle)
- **who** owns the resource (warehouse, production, construction)
- **what** happens after delivery (demand lifecycle, telemetry)

It walks a route and moves cargo. Everything else is someone else's responsibility.

```
                    ┌─────────┐
                    │  Idle   │
                    └────┬────┘
                         │ Dispatcher assigns task
                         ▼
                   ┌───────────┐
                   │  Assigned │  ← has task, knows source node
                   └─────┬─────┘
                         │ Arrived at source node
                         ▼
                   ┌───────────┐
                   │  Pickup   │  ← sourceNode.buffer.Remove(res, amt)
                   └─────┬─────┘  ← cargo = new Cargo(res, amt)
                         │ Pickup complete
                         ▼
                   ┌────────────┐
                   │ Travelling │  ← route walking (AdvanceHop per flag)
                   └─────┬──────┘
                         │ IsLastHop == true
                         ▼
                   ┌────────────┐
                   │ Delivering │  ← destNode.ReceiveCargo(res, amt)
                   └─────┬──────┘  ← DemandManager.CompleteDemand(ticket)
                         │         ← cargo deleted
                         ▼
                    ┌─────────┐
                    │  Idle   │  ← returned to pool
                    └─────────┘
```

### State transitions

| Transition | Trigger | Action |
|-----------|---------|--------|
| Idle → Assigned | Dispatcher picks task, carrier is available | `carrier.task = task`, `carrier.sourceNode = task.origin`, route loaded |
| Assigned → Pickup | Carrier arrives at source node | `sourceNode.buffer.Remove(res, amt)`. If `buffer.Count(res) < amt`: carrier waits (retry next tick). If sufficient: cargo created, pickup complete. |
| Pickup → Travelling | Cargo loaded | Carrier begins walking route edges toward destination |
| Travelling → Delivering | Carrier reaches destination flag | `destNode.ReceiveCargo(res, amt)`, `DemandManager.CompleteDemand(ticket)`, cargo deleted |
| Delivering → Idle | Delivery recorded | Carrier back to pool, task freed |

### Pickup guard

If `sourceNode.buffer.Count(resource) < amount` at pickup time, the carrier waits one tick and retries. This prevents phantom assignments when the buffer was drained between task creation and carrier arrival. After `kMaxPickupRetries` (configurable, default 3) consecutive failures, the task is cancelled and re-queued.

## Resource Race Resolution

### Scenario

```
Tick T:     DemandManager creates demand: nodeDestination needs Wood
            TransportController creates task: source = nodeSource, dest = nodeDestination
Tick T+1:   Carrier dispatched, en route to nodeSource
Tick T+2:   LocalTransferSystem supplies local building from nodeSource buffer
            → nodeSource.buffer.Count(Wood) drops from 5 to 0
Tick T+3:   Carrier arrives at nodeSource, Pickup → Count(Wood) = 0
            → retry (Tick T+4, T+5)
            → after 3 retries: CancelTask, CancelDemand
```

### Who decides

| Question | Answer |
|----------|--------|
| Can local supply drain the source buffer before carrier pickup? | Yes — intentionally. Local consumption > remote transport. |
| Can the same resource be assigned to two carriers? | No — `activeTask` guard prevents duplicate task creation per demand. |
| What if local supply exactly satisfies the deficit before carrier arrives? | Carrier retries → cancels. `CompleteDemand` is NOT called (no delivery happened). `CancelDemand` decrements `remaining` by the amount the carrier was tasked to deliver. Next LTS re-evaluation will not re-publish demand (deficit satisfied locally). |
| What if local supply partially satisfies the deficit? | Carrier picks up what's available (partial pickup is NOT supported — Milestone 3 always picks up the full task amount). Carrier retries, then cancels. DemandManager re-publishes for the remaining amount. |
| Can demand "resurrect" after being satisfied locally? | No. LTS re-evaluates deficit: if `inputDelivered >= inputRequired`, no pendingDemand is published. DemandManager sees no pendingDemand, so no re-publication. If `Demand.remaining > 0` but no pendingDemand from LTS, DemandManager cancels the remaining demand. |

### Design rationale

Local consumption takes priority over remote transport because:
1. LocalTransferSystem runs BEFORE DemandManager in the tick order
2. LTS supplies local buildings before DemandManager reads pendingDemand
3. Therefore, LTS always sees the most current state
4. If a remote task is in-flight, local supply may drain its source buffer — the carrier retry mechanism resolves this without deadlock

## Failure Model

| Failure | Detection | Resolution | Status for M3 |
|---------|-----------|------------|---------------|
| Carrier destroyed mid-route | TransportController checks carrier health each tick | Task re-queued at source node. If carrier lost before pickup: cargo never created, no resource loss. If after pickup: cargo lost — resource is lost. `CompleteDemand` not called; demand remains. After `kMaxRetries`, demand cancelled, resource loss recorded in telemetry. | Undefined — resource loss is acceptable for M3 if rare. Telemetry tracks loss count. |
| Demand cancelled while carrier en route | TransportController checks `DemandManager.GetRemaining(task.demandId) > 0` before delivery | Carrier completes delivery (cargo already in transit). `CompleteDemand` called. DemandManager ignores if demand already cancelled (ticket invalid). Cancel-after-delivery: cargo arrives, demand already gone — buffer incremented, remaining unchanged. | Handled by TransportController: delivery always completes. DemandManager ignores orphan tickets. |
| Source node buffer empty at pickup time | Carrier.Pickup: `buffer.Count(res) < amount` | Retry up to `kMaxPickupRetries`. If exhausted: CancelTask, carrier returns to Idle. DemandManager doesn't cancel demand — LTS re-evaluates next tick and may re-publish. | Handled. Pickup guard covers this. |
| Destination node removed mid-route | TransportController checks `IsValidNode(task.destination)` before `IsLastHop` | CancelTask, carrier returns to Idle. DemandManager cancels demand for invalid destination. | Handled. |
| Node detached from building mid-route | TransportNode.DetachBuilding → node still exists, building gone | Carrier can still deliver to node (buffer still valid). Destination node receives cargo. Building no longer attached → LTS won't supply it. Cargo sits in node buffer until another building attaches or warehouse observes surplus. | Handled — node survives detachment. |
| Demand re-published while carrier en route | LTS writes pendingDemand; DemandManager sees remaining > 0 AND activeTask > 0 | `PublishTransportRequests` checks `activeTask != 0` → skips this demand. No duplicate task created. The in-flight carrier is still the sole active task for this demand. | Handled. `activeTask` guard prevents duplicates. |
| `kMaxPickupRetries` exceeded | Carrier retry counter reaches limit | CancelTask, carrier freed. CancelDemand adjusts remaining. LTS re-evaluates next tick. | Handled. |

## Demand Layer Invariants

```
1.  One active Task per (Node, Resource) pair — activeTask guard prevents duplicates.
2.  One active Task per Carrier — carrier serves at most one TransportTask at a time.
3.  Demand never exists without a valid Node — DemandManager clears demands for removed nodes.
4.  TransportController never creates Demand — DemandManager is the sole publisher.
5.  Carrier never creates resources — Cargo is created at Pickup from buffer.Remove(), never from nothing.
6.  Pickup decrements before Delivery increments — buffer.Remove() at source, THEN ReceiveCargo() at dest.
    Resource is in exactly one place at every Tick boundary.
7.  Cancel does not return already-delivered resources — CompleteDemand is called before cargo deletion;
    CancelDemand only affects remaining, not delivered.
8.  Local consumption > remote transport — if LTS drains a buffer before carrier pickup, carrier retries and cancels.
    Local supply never waits for a remote carrier.
9.  No reservation — resources are not reserved at task creation or dispatch time.
    Only carrier.Pickup atomically claims the resource from the source buffer.
10. Carrier ownership is linear: Node A → Carrier → Node B.
    At no point does the same resource unit exist in both Node A and Node B simultaneously,
    nor in Carrier and either Node simultaneously (outside the transient Pickup/Delivery tick).
11. Priority is domain-agnostic — Dispatcher uses basePriority + age, never switches on reason or owner.
12. CompleteDemand is idempotent — calling with an already-completed ticket has no effect.
```

These invariants are designed to be directly translatable into integration tests (T53+).

## Known Temporary Limitations

### 1. AcceptingFlagInventory (legacy stub)

`AcceptingFlagInventory` accepts all deliveries and discards all goods. It creates resources from nothing, breaking closed-form resource conservation. This is a legacy stub from before TransportNode existed.

**Status:** Will be replaced in Milestone 3. Carrier pickup will decrement `sourceNode.buffer`, delivery will increment `destNode.buffer`. After replacement, `AcceptingFlagInventory` is removed.

### 2. WarehouseSystem::buffer.Remove() (accounting correction)

```
WarehouseSystem::HandleDeliveryEvents() — currently calls:
    node.buffer.Remove(ev.resource, 1)
```

Until Carrier properly decrements source node buffer on pickup, WarehouseSystem performs an accounting removal from the source node buffer on warehouse delivery. This is a **temporary stub bypass** — the warehouse should not decrement the source node.

**Scheduled for removal:** When Carrier::Pickup properly decrements `sourceNode.buffer.Remove()`. After removal, WarehouseSystem only reads `buffer` (via `ScanTransportBuffers`), never writes it.

### 3. No inter-node routing

All transport tasks currently have `origin=0` (AcceptingFlagInventory) and deliver directly. There is no routing between TransportNodes — goods are created from nothing and delivered to arbitrary destination flags.

**Milestone 3 goal:** TransportController resolves routes between TransportNodes via `IRoadGraph`. Source node buffer is properly decremented on pickup. Destination node buffer is properly incremented on delivery. Goods are conserved.

### 4. DirectRouteRoadGraph (stub)

`DirectRouteRoadGraph::FindRoute()` always returns `[sourceFlag, destinationFlag]` — a single-hop route regardless of actual road distance. This is acceptable for testing but does not represent real routing.

**Milestone 3 replacement:** Real `IRoadGraph` implementation that finds routes through the actual road network, with `kMaxRouteLength` guard.

## Milestone 3 Assumptions

The following assumptions are **explicitly documented as design constraints** for Milestone 3.
They are not bugs — they are preconditions that implementation and tests may rely on.

### 1. Carrier travel time is effectively non-zero after DirectRouteRoadGraph replacement

```
Current (stub):       FindRoute → [start, end] (1 hop) → delivers same tick
After replacement:    FindRoute → [A, B, C, D] (N hops) → N ticks travel
```

**Risk:** Demand lifecycle logic (retry, cancel, re-issue) must not depend on same-tick delivery. If a test or handler implicitly assumes the carrier delivers before the next tick, it will break when routes become multi-hop.

**Mitigation:** All test scenarios involving inter-node transport should include at minimum one travel tick between pickup and delivery. Synchronous delivery (same tick) is a stub property, not a contract.

### 2. AcceptingFlagInventory removal reveals conserved resource accounting

```
Current:  AcceptingFlagInventory.ReceiveCargo() → returns true, discards goods
          → resources created from nothing at destination
After:    Carrier.Pickup → sourceNode.buffer.Remove()
          Carrier.Deliver → destNode.ReceiveCargo()
          → resources conserved end-to-end
```

**Risk:** Any code that depends on resources appearing at destination without being removed from a source will break. The most likely site is WarehouseSystem::HandleDeliveryEvents — currently performing `buffer.Remove()` as accounting correction.

**Mitigation:** WarehouseSystem `buffer.Remove()` bypass is explicitly scheduled for removal when Carrier implements proper pickup. T17.E/F (closed-form conservation) will change from INFO back to FAIL until conservation is restored.

### 3. One in-flight task per demand is sufficient bandwidth

The spec assumes that `activeTask` guard (at most one task per demand) does not bottleneck throughput, because:
- LTS re-evaluates deficits every tick
- DemandManager re-publishes on the same tick as CompleteDemand
- Carrier travel time is bounded by road distance (no infinite routes)
- If the same demand needs more cargo, `remaining` accumulates and a new task is created after the current one completes

**Risk:** If a building consumes resources faster than a single carrier can deliver, the building starves despite having an active demand.

**Mitigation:** This is not addressed in Milestone 3. If observed in soak tests (T17, T41), the `activeTask` model may be relaxed to allow multiple in-flight tasks per demand, with DemandManager merging completions to `remaining`.

### 4. Tick order is stable

```
LTS Tick         → writes pendingDemand[]
DemandManager    → reads pendingDemand[], creates/updates Demand
TransportController → reads Demand, creates task, dispatches carrier
```

**Assumption:** This order does not change within Milestone 3. If a new system is inserted between LTS and DemandManager, it must not read or write `pendingDemand[]`.

### 5. No partial pickups in Milestone 3

If `sourceNode.buffer.Count(res) < amount`, the carrier retries the full amount. It does not pick up what's available and deliver a partial cargo.

**Rationale:** Partial pickups would require DemandManager to split demands (remaining -= partial), which adds complexity. If soak tests show throughput degradation due to full-amount retries, partial pickups can be added in a subsequent milestone. This is a performance concern, not a correctness concern.

## Milestone 3 PR Roadmap

Each PR is independently verifiable: builds, tests pass, no regression.

| PR | What appears | What is still explicitly absent or stubbed |
|----|-------------|-------------------------------------------|
| **3.1** | `DemandManager` — full lifecycle (SetDemand, ClearDemand, CompleteDemand, OnTaskCreated, PublishTransportRequests), deduplication, `activeTask` guard, tick integration reading `pendingDemand[]` | No `TransportController` changes. No `Carrier` movement. No inter-node delivery. Demands published as `TransportRequest[]`, consumed by stub (existing `AcceptingFlagInventory` path). Integration test: T53 — DemandManager lifecycle + deduplication. |
| **3.2** | `TransportController` — `CreateTask`, dispatch, `Dispatcher` integration, carrier pool management, route resolution via `IRoadGraph` | `Carrier` may remain a simple stub (immediate delivery, no spatial movement). `AcceptingFlagInventory` still in use. Integration test: T54 — task creation → dispatch → carrier lifecycl |e. |
| **3.3** | `Carrier` — full state machine (Idle → Assigned → Pickup → Travelling → Delivering → Idle), `Pickup` decrements `sourceNode.buffer` | `AcceptingFlagInventory` still used for destination delivery (no conservation yet). `DirectRouteRoadGraph` still stubbed (single-hop). Integration test: T55 — Carrier state machine end-to-end. |
| **3.4** | `AcceptingFlagInventory` replacement — carrier delivery increments `destNode.buffer` via `ReceiveCargo`. Closed-form resource conservation restored | `DirectRouteRoadGraph` may still be stubbed. T17.E/F changes from INFO back to FAIL (now verifiable). T56 — inter-node resource conservation test. |
| **3.5** | `DirectRouteRoadGraph` replacement — real route resolution with multiple hops, travel time > 1 tick | No changes to DemandManager, TransportController, or Carrier API. T57 — multi-hop routing integration test. |

### Inter-PR contract

- PR 3.1 introduces the `DemandManager` API and its contract. PRs 3.2–3.5 implement against this API without modifying its contract.
- PR 3.2 introduces `TransportController` lifecycle. PRs 3.3–3.5 extend the carrier implementation without changing `TransportController` API.
- PR 3.4 restores closed-form conservation. T17.E/F is the regression gate for this PR.
- PR 3.5 must not break any existing test — it only changes the route resolution backend.

## Milestone 3 — Status: COMPLETED ✅ (2026-07-06)

All criteria fulfilled. 212/212 tests pass. No regression in T15, T17, or any soak test.

### Functional

- [x] DemandManager publishes demands from `LocalTransferSystem.pendingDemand[]` (read from TransportNode, not from direct system calls)
- [x] DemandManager deduplicates: `SetDemand(type, flag, amt)` accumulates remaining, never duplicates entries
- [x] DemandManager respects `activeTask` guard: at most one in-flight task per demand at any time
- [x] DemandManager clears completed demands when `remaining == 0`
- [x] Supply scenario tests: Woodcutter node has Wood surplus → Sawmill node has Wood deficit → DemandManager creates demand → task assigned → Wood moves between nodes → Sawmill receives Wood
- [x] Inter-node Wood delivery verified: Woodcutter → nodeA → Carrier → nodeB → Sawmill

### Integration

- [x] Production→Transport pipeline works without AcceptingFlagInventory for Wood movement
- [x] WarehouseSystem::ScanTransportBuffers is the only warehouse-side mechanism — no direct outputBuffer access
- [x] T15 passes with Carrier-based inter-node delivery

### Stub bypass removed

- [x] `AcceptingFlagInventory` removed from the Production→Transport pipeline (`AcceptingFlagInventory.ReceiveDelivery()` no longer called by any delivery path)
- [x] `WarehouseSystem::HandleDeliveryEvents` no longer calls `buffer.Remove()` — all buffer decrements happen at Carrier::Pickup via `sourceNode.buffer.Remove()`
- [x] `DirectRouteRoadGraph` replaced by `RoadGraph` with BFS pathfinding; multi-hop routes verified (3 hops = 3 ticks)

### Tests

- [x] 212/212 unit tests pass
- [x] T15: Warehouse integration passes (Carrier-based delivery)
- [x] T17: 50k warehouse soak passes (closed-form conservation verified: `Σ(node buffers) + Σ(carrier cargo) = constant`)
- [x] New: Carrier state machine unit test — all 5 states verified end-to-end
- [x] New: DemandManager lifecycle test — SetDemand → CompleteDemand → ClearDemand, deduplication, activeTask guard
- [x] New: Inter-node routing integration test — Wood from nodeA to nodeB via Carrier (multi-hop: 3 ticks)
- [x] New: Conservation integration test — resource invariant across Pickup/Travelling/Delivery lifecycle
- [x] New: RoadGraph unit tests — 10 tests (BFS, AddEdge, RemoveEdge, bidirectional, multi-hop, no-path)

### Documentation

- [x] This document updated with completion status
- [x] ARCHITECTURE.md created — Milestone 3 freeze documented, Milestone 4 direction noted
- [x] AGENTS.md updated — Milestone 3 freeze, legacy cleanup verified
