# Logistics Architecture — Phase 7 Specification

> **STATUS: FROZEN — Version 1.0**
> No changes without RFC and review. Implementation must follow this spec exactly.

## Core Principles

### P0 — TransportTask is the sole identifier of a shipment
> **One TransportTask = one physical item. The task's id and route never change during its lifetime.**
Only `state`, `hopIndex`, `carrier`, and physical cargo location may change.

A shipment of 5 wood = 5 independent TransportTask instances. No partial delivery, no fractional states. Any shipment can be traced from creation to delivery by a single task id.

### P1 — TransportController is the sole decision-maker
> **All logistics decisions belong to TransportController.**
All other entities (Carrier, Cargo, Flag, DemandManager) either report events or execute commands. They never decide routes, assignments, hops, or priorities.

### P2 — Carrier does not interpret the route
> **Carrier knows only its immediate target flag.**
Controller sets `targetFlag` on assignment. Carrier never reads `route[]`, never computes `nextHop`, never decides where to go after a drop.

### P3 — Events, not polling
> **Controller reacts to events. It never scans flags or carriers per frame.**
Every state change is triggered by a specific event (carrier became idle, task was created, carrier reached flag, road network changed). No per-frame O(n) scans of flags or tasks.

Events:
- `OnCarrierIdle(Carrier* c, FlagId atFlag)` — carrier is ready for work
- `OnTaskCreated(TransportTask* task)` — new task needs assignment
- `OnCarrierReachedTarget(Carrier* c, FlagId flagId)` — carrier arrived at flag
- `OnCarrierPickedUp(Carrier* c)` — carrier picked up cargo
- `OnCarrierDropped(Carrier* c, FlagId flagId)` — carrier left cargo on flag
- `OnRoadNetworkChanged()` — roads built/removed, triggers Blocked retry
- `OnFlagRemoved(FlagId flagId)` — flag destroyed, cleanup tasks

---

## 1. Entity Roles

| Entity | Responsibility | Decisions |
|--------|---------------|-----------|
| DemandManager | Declares what needs to move | `CreateTask()` only |
| TransportController | Owns all tasks, routes, queues, carriers | Assign, route, re-route, complete |
| TransportTask | Single source of truth for one shipment | None (data only) |
| TransportRoute | Immutable ordered list of FlagIds | None (data only) |
| Cargo | Physical resource in transit | None (data only) |
| Carrier | Executes PickUp / Walk / Drop | None |
| Flag | Game entity with CargoList | None |

---

## 2. Structures

### 2.1 TransportRoute

```cpp
enum { kMaxRouteLength = 32 };

struct TransportRoute
{
    uint8 count;                          // number of flags in route
    FlagId flags[kMaxRouteLength];        // ordered: [origin, hop1, hop2, ..., destination]
};
```

**Invariants:**
- `count >= 2` (origin and destination must differ)
- `flags[0] == origin`
- `flags[count-1] == destination`
- All flags in the route are connected by roads (verified at creation time)
- The route is **immutable** after creation. Never modified.

### 2.2 Priority and Reason

```cpp
enum class TaskReason : uint8
{
    Construction,
    Production,
    Food,
    WarehouseBalance,
    Military,
    Emergency
};

struct Priority
{
    uint8 classPriority;    // set once at creation based on TaskReason
    uint8 dynamicPriority;  // increases over time (anti-starvation)
};
```

**Priority_score formula:**
```
score = classPriority * kClassWeight + dynamicPriority * kDynamicWeight + waitingTicks
```

### 2.3 TransportTask

```cpp
```cpp
enum class TaskState : uint8
{
    Created,            // just created by DemandManager
    Blocked,            // no route found at creation — road missing
    WaitingAtSource,    // queued at source flag, awaiting carrier
    Moving,             // carrier transporting cargo to targetFlag
    ArrivedAtHop,       // cargo at hop flag, awaiting next carrier
    Cancelled,          // mid-transit cancellation — carrier will finish current hop
    Delivered           // cargo at destination, task complete
};
```

### 2.3 TransportTask

```cpp
struct TransportTask
{
    // Identity (never changes)
    uint32_t id;

    // Cargo
    ResourceType resource;  // one task = one physical unit

    // Route
    FlagId origin;
    FlagId destination;
    TransportRoute route;   // built at CreateTask(), never modified
    uint8_t hopIndex;       // current position: route.flags[hopIndex]

    // Priority
    Priority priority;
    TaskReason reason;

    // State
    TaskState state;

    // Execution references (set by Controller, for fast lookup)
    Cargo* cargo;
    Carrier* carrier;
    FlagId targetFlag;      // next flag the carrier must walk to

    // Bookkeeping
    uint32_t waitingSinceTick;
};
```

**Size estimate:** ~64 bytes + route (32×4=128) ≈ 192 bytes per task. Pool of 256 tasks = ~48 KB.

### 2.4 TransportController

```cpp
class TransportController
{
public:
    // ── Lifecycle ──
    TransportTask* CreateTask(ResourceType type, FlagId origin, FlagId destination, TaskReason reason);
    void CancelTask(uint32_t taskId);

    // ── Event callbacks (from Carrier / world) ──
    void OnCarrierIdle(Carrier* carrier, FlagId atFlag);
    void OnCarrierReachedTarget(Carrier* carrier, FlagId flagId);
    void OnCarrierPickedUp(Carrier* carrier);
    void OnCarrierDropped(Carrier* carrier, FlagId flagId);
    void OnRoadNetworkChanged();
    void OnFlagRemoved(FlagId flagId);

    // ── Query ──
    int GetActiveTaskCount() const;
    TransportTask* GetTaskById(uint32_t id);

private:
    // ── Core logic (called only from event handlers) ──
    void TryAssignTask(Carrier* carrier, FlagId atFlag);
    void AdvanceHop(TransportTask* task);
    void DeliverTask(TransportTask* task);
    void RetryBlockedTasks();
    void CleanupForFlag(FlagId flagId);

    // ── Data ──
    static const int kMaxTasks = 256;
    TransportTask m_pool[kMaxTasks];
    uint32_t m_nextTaskId;
    int m_activeCount;

    // Carriers known to Controller (CarrierManager still owns movement)
    static const int kMaxCarriers = 64;
    Carrier* m_carriers[kMaxCarriers];
    int m_carrierCount;

    // Idle carrier list (indices into m_carriers[])
    FixedVector<int, kMaxCarriers> m_idleCarriers;

    // Per-flag waiting queues (task indices)
    FixedVector<int, kMaxTasks> m_waitingByFlag[kMaxFlags];

    // External dependencies
    CargoManager* m_cargoManager;
    CarrierManager* m_carrierManager;
    RoadManager* m_roadManager;
    FlagManager* m_flagManager;
    DemandManager* m_demandManager;
};
```

---

## 3. Lifecycle (State Machine)

```
        DemandManager::CreateTask()
                    │
                    ▼
              ┌───────────┐
              │  Created   │
              └─────┬─────┘
                    │ Controller calls RoadManager::FindPath()
           ┌────────┴────────┐
           │                 │
     path found          no path
           │                 │
           ▼                 ▼
     EnqueueAtSource    ┌──────────┐
           │           │ Blocked  │ ←── OnRoadNetworkChanged() → RetryBlockedTasks()
           ▼           └──────────┘
    ┌─────────────────┐
    │ WaitingAtSource  │ ←── CancelTask() → Freed immediately
    └────────┬────────┘
             │ Controller::TryAssignTask()
             ▼
       ┌──────────┐
       │  Moving   │ ←── CancelTask() → state=Cancelled, drops at next flag
       └─────┬────┘
             │ Carrier reaches targetFlag
             ▼
      ┌──────────────┐
      │ ArrivedAtHop  │
      └──────┬───────┘
             │ Controller::AdvanceHop()
    ┌────────┴────────┐
    │                 │
hopIndex++        last hop
 OR Cancelled    == route.count-1
    │                 │
    ▼                 ▼
WaitingAtSource  ┌──────────┐
  (or Freed      │ Delivered │
   if Cancelled) └────┬─────┘
                      │ Controller::NotifyDemandManager()
                      ▼
                   Freed
```

### State ownership matrix

| Transition | Trigger | Owner |
|-----------|---------|-------|
| Created → WaitingAtSource | Route built, task enqueued | Controller |
| Created → Blocked | No route found | Controller |
| Blocked → WaitingAtSource | Road rebuilt, reroute succeeds | Controller (on `OnRoadNetworkChanged`) |
| WaitingAtSource → Moving | Task assigned to idle carrier | Controller::TryAssignTask() |
| WaitingAtSource → Cancelled | CancelTask() before pickup | Controller |
| Moving → ArrivedAtHop | Carrier reaches target flag, drops cargo | Carrier (reported via `OnCarrierReachedTarget`) |
| Moving → Cancelled | CancelTask() during transit | Controller |
| ArrivedAtHop → WaitingAtSource | hopIndex++, task requeued | Controller::AdvanceHop() |
| ArrivedAtHop → Delivered | Last hop reached | Controller::AdvanceHop() |
| ArrivedAtHop → (free) | Cancelled task after drop | Controller |
| Delivered → (free) | Notification sent, task released | Controller |

**Rule:** Carrier transitions `Moving→ArrivedAtHop` (via `OnCarrierDropped`). Everything else is Controller.

---

## 4. Event-Driven Algorithm

Controller has no per-frame Update loop. All logic lives in event handlers.

### 4.1 Core handlers

```
OnCarrierIdle(Carrier* c, FlagId atFlag)
{
    m_idleCarriers.push(c->index)
    TryAssignTask(c, atFlag)
}

OnTaskCreated(TransportTask* task)
{
    if task.state == Blocked: return  // will retry on road change

    assert(task.state == WaitingAtSource)
    m_waitingByFlag[task.route.flags[0]].push(task.id)

    // If there's an idle carrier at the source flag, assign immediately
    Carrier* c = FindIdleCarrierAtFlag(task.route.flags[0])
    if c:
        TryAssignTask(c, task.route.flags[0])
}

OnCarrierReachedTarget(Carrier* c, FlagId flagId)
{
    TransportTask* task = c->GetCurrentTask()
    assert(task != NULL)
    assert(task->state == Moving)
    assert(flagId == task->targetFlag)

    c->ClearTask()
    task->state = ArrivedAtHop
    task->carrier = NULL

    // Keep cargo: carrier dropped it on the flag via CargoManager
    AdvanceHop(task)
}

OnCarrierPickedUp(Carrier* c)
{
    // Carrier now has cargo. State is already Moving (set by TryAssignTask).
    // No action needed — Carrier will walk toward targetFlag.
}

OnCarrierDropped(Carrier* c, FlagId flagId)
{
    // Cargo is on flag. Carrier is now idle.
    OnCarrierIdle(c, flagId)
}
```

### 4.2 AdvanceHop

```
AdvanceHop(task)
{
    assert(task->state == ArrivedAtHop)
    assert(task->hopIndex < task->route.count - 1)

    task->hopIndex++
    task->cargo = NULL

    if task->hopIndex == task->route.count - 1:
        // Reached final destination — cargo is on the flag
        task->state = Delivered
        DeliverTask(task)
    else:
        task->state = WaitingAtSource
        m_waitingByFlag[task->route.flags[task->hopIndex]].push(task.id)
}
```

### 4.3 TryAssignTask

```
TryAssignTask(Carrier* c, FlagId atFlag)
{
    if m_waitingByFlag[atFlag].empty(): return

    int bestTaskIdx = SelectBestTask(m_waitingByFlag[atFlag])
    TransportTask* task = &m_pool[bestTaskIdx]

    assert(task->state == WaitingAtSource)
    assert(task->route.flags[task->hopIndex] == atFlag)

    task->carrier = c
    task->state = Moving
    task->targetFlag = task->route.flags[task->hopIndex + 1]
    c->AssignTask(task, task->targetFlag)

    RemoveFromWaitingQueue(atFlag, bestTaskIdx)
    RemoveFromIdleCarriers(c->index)
}
```

### 4.4 Priority selection

```
SelectBestTask(taskIndices)
{
    bestScore = -1
    bestIdx = -1

    for each taskIndex in taskIndices:
        TransportTask* t = &m_pool[taskIndex]
        score = t->priority.classPriority * 1000
              + t->priority.dynamicPriority * 100
              + (currentTick - t->waitingSinceTick)

        if score > bestScore:
            bestScore = score
            bestIdx = taskIndex

    return bestIdx
}
```

### 4.5 DynamicPriority (anti-starvation)

**Formula:**
```
dynamicPriority = (currentTick - createdTick) / kPriorityAgeStep
```
Where `kPriorityAgeStep = 1800` (30 seconds at 60 ticks/sec).

No per-frame computation. Read only at priority selection time.

### 4.6 Blocked retry

```
OnRoadNetworkChanged()
{
    RetryBlockedTasks()
}

RetryBlockedTasks()
{
    for each task in m_pool where task.state == Blocked:
        Route newRoute = RoadManager::FindPath(task->origin, task->destination)
        if newRoute.found:
            task->route = newRoute
            task->state = WaitingAtSource
            m_waitingByFlag[task->route.flags[0]].push(task.id)
}
```

No timers. No per-frame scans. Only called on road-network events.

---

## 5. Carrier Contract

```cpp
class Carrier
{
public:
    // Called by Controller. Carrier receives only its immediate target.
    void AssignTask(TransportTask* task, FlagId targetFlag);
    void ClearTask();                         // called when hop completes

    TransportTask* GetCurrentTask() const;
    FlagId GetTargetFlag() const;

    // Callbacks — Carrier notifies Controller of events
    void OnReachedTargetFlag(FlagId flagId);  // "I'm at my target"
    void OnPickupComplete();                  // "I have cargo"
    void OnDropComplete();                    // "Cargo is on the flag"

    // Query
    FlagId GetCurrentFlag() const;
    bool IsIdle() const;
    bool HasCargo() const;
private:
    TransportTask* m_task;
    FlagId m_targetFlag;
    Cargo* m_cargo;
    // ... movement state (ep, walkDir, road, etc.)
};
```

**Carrier does NOT:**
- Scan for work
- Query demand
- Compute routes
- Read `route[]` or `hopIndex`
- Decide destinations
- Change task state
- Call flag logistics APIs

**Carrier only:**
- Holds `m_task` + `m_targetFlag` (set by Controller)
- Walks toward `m_targetFlag`
- On arrival at target: calls `OnReachedTargetFlag(flagId)`
- On pickup: calls `OnPickupComplete()`
- On drop: calls `OnDropComplete()`

---

## 6. Cargo Contract

```cpp
struct Cargo
{
    TransportTask* task;
    // CargoManager pool fields (id, state, handle, etc.)
};
```

Cargo has no routing information. It is a physical object that exists on a flag or in transit with a carrier. Its sole purpose is to track the physical resource's position in the world.

**Cargo does NOT:**
- Know destination
- Know route
- Know carrier
- Have transportJobId

---

## 7. Warehouse is Not Special

Controller treats all nodes identically. `origin` and `destination` are `FlagId` values. Controller never checks whether a flag has a warehouse, a mine, or a lumberjack.

The storehouse is just a Cargo source: when a carrier picks up from a warehouse flag, `CargoManager` creates a Cargo and decrements the storehouse inventory. The same code path works for any production building: a woodcutter flags has Cargo placed on it by the building, and the carrier picks it up identically.

No special-case code for warehouses in Controller.

---

## 8. Cancellation

Three phases, each with different behavior:

| Phase | State | Action |
|-------|-------|--------|
| Before PickUp | WaitingAtSource | Immediate. Remove from queue, free task. Cargo never existed. |
| During transit | Moving | Mark task `Cancelled`. Carrier continues to current targetFlag, drops cargo. Controller then frees task (cargo is discarded). |
| After delivery | ArrivedAtHop / Delivered | Ignored. Too late — cargo already at destination or intermediate flag. |

```cpp
void CancelTask(uint32_t taskId)
{
    TransportTask* task = &m_pool[taskId];
    switch (task->state)
    {
    case WaitingAtSource:
        RemoveFromWaitingQueue(task->route.flags[task->hopIndex], taskId);
        FreeTask(task);
        break;

    case Moving:
        task->state = Cancelled;     // Carrier will drop at next flag, then clean up
        break;

    case ArrivedAtHop:
    case Delivered:
        // Too late — cargo is already on a flag
        break;

    default:
        break;
    }
}
```

## 9. DemandManager Changes

DemandManager is simplified to a pure demand declaration system:

```cpp
class DemandManager
{
public:
    // Called by construction, production, etc.
    void RequestTransport(ResourceType type, uint8 amount, FlagId origin, FlagId destination, TaskReason reason);
};
```

Internally it calls `TransportController::CreateTask()`. It no longer manages tickets, allocations, or supply entries.

---

## 9. Key Invariants

### I1 — Single owner per transition
Each state transition has exactly one owner (see ownership matrix §3).

### I2 — Route immutability
`TransportRoute.flags[]` is never modified after task creation. `hopIndex` is the only mutable routing field.

### I3 — Task uniqueness
No two tasks share the same Cargo or Carrier reference at the same time.

### I4 — Position consistency
```
task->state == WaitingAtSource  →  cargo is on flag route.flags[hopIndex]
task->state == Moving           →  cargo is with carrier
task->state == ArrivedAtHop     →  cargo is on flag route.flags[hopIndex]
task->state == Delivered        →  cargo is at destination (route.flags[last])
```

### I5 — No orphan tasks
```
task->state != Delivered  ⇒  task is in exactly one Controller queue
task->state == Delivered  ⇒  task is in no queue
```

### I6 — Carrier ownership
```
carrier->m_task == NULL  ⇔  task is in idle carrier list
carrier->m_task != NULL  ⇒  task->carrier == carrier  AND  task->state == Moving
```

### I7 — No dangling references
All `FlagId` and `Carrier*` references are validated before use. When a flag or carrier is destroyed, Controller cleans up all associated tasks.

### I8 — Anti-starvation
Every task in `WaitingAtSource` state accumulates age-based priority. The `dynamicPriority` value grows by 1 every 30 seconds, ensuring no task waits forever.

### I9 — No resource loss on cancellation
A `Cancelled` task in `Moving` state always completes its current hop. Cargo is dropped on the target flag. Only then is the task freed. Resources never disappear mid-route.

---

## 10. Migration Plan

### Phase 6b ARCHIVE — Freeze current state
- [ ] Revert all Phase 6b TransportJob changes (keep as `archive/phase-6b` branch)
- [ ] Set `kUseTransportJobs = false` in DemandManager.h (restore legacy behavior)
- [ ] Verify game boots and runs on legacy path
- [ ] Branch `phase-7` from this stable point

### Phase 7.0 — Specification (this document)
- [ ] Review and approve architecture
- [ ] Define all data structures in header files (no implementation)
- [ ] Define integration points with existing systems

### Phase 7.1 — Skeleton
- [ ] Implement TransportRoute, TransportTask, Priority types
- [ ] Implement TransportController with empty Update()
- [ ] Add CreateTask() and queue management (no carrier assignment)
- [ ] Compile-time assert: sizeof(TransportTask) within budget
- [ ] Keep kUseTransportJobs = false; no behavior change

### Phase 7.2 — Single-hop transport
- [ ] Controller::AssignTasks() for one route, one carrier
- [ ] Carrier::AssignTask() + PickUp/Walk/Drop integration
- [ ] OnCarrierPickedUp / OnCarrierArrivedAtFlag callbacks
- [ ] Test: warehouse→single flag, verify task lifecycle

### Phase 7.3 — Multi-hop transport
- [ ] AdvanceHop() implementation
- [ ] Per-flag waiting queues in Controller
- [ ] Test: warehouse→flag A→flag B, verify hop handoff

### Phase 7.4 — Priority dispatching ✅
- [x] `TransportBasePriority` enum: Low(0), Normal(100), High(200), Critical(300)
- [x] `PriorityForReason()` — maps `TransportTaskReason` to base priority
- [x] `PickNextTask(flagId)` — linear scan, selects by (priority DESC, enqueueOrder ASC)
- [x] Age bonus on selection (no per-frame mutation)
- [x] Anti-starvation: old Low task rises to Normal after ~100 ticks
- [x] Instrumentation: Queue + Dispatch logs

**Architectural boundary added:**
> **Route planner decides WHERE cargo moves.**
> **Priority dispatcher decides WHEN cargo moves.**
> These responsibilities must never be mixed. The dispatcher (`PickNextTask`)
> selects among waiting tasks but never modifies route, hopIndex, or targetFlag.

### Phase 7.5 — Cleanup & removal
- [ ] Remove TransportJobManager, DemandTicket, old Demand pipeline
- [ ] Remove Reserve(), FindBestDemand(), Allocate()
- [ ] Remove kUseTransportJobs flag
- [ ] Run full T1–T5 soak

### Phase 8 — Economy integration (migration, not rewrite)

**8.1 — Demand → TransportTask adapter (bridge)**
- [ ] DemandManager creates TransportTasks via bridge; old pipeline still alive
- [ ] Invariant: one Demand = at most one active TransportTask

**8.2 — Resource ownership migration**
- [ ] Define ownership chain: Ground → Flag → TransportTask → Carrier → Building
- [ ] Runtime audit `[Resource] id=712 owner=TransportTask(17)` (debug)
- [ ] Remove old ownership paths

**8.3 — Parallel validation mode**
- [ ] old TransportJobManager = observe only, new TransportController = execute
- [ ] Log: `[MIGRATION] demand=81 old=flag12 new=flag12 OK`

**8.4 — Remove legacy transport**
- [ ] All scenarios pass before deleting old code
- [ ] Remove TransportJobManager, DemandTicket, `kUseTransportJobs`, old routing

---

## 11. Resolved Decisions

| # | Question | Decision |
|---|----------|----------|
| Q1 | When to build route? | **At CreateTask()**. `RoadManager::FindPath()` is called immediately. If no path: state = Blocked. |
| Q2 | DynamicPriority formula? | **Age-based**: `(currentTick - createdTick) / kPriorityAgeStep`. Read at selection time, no per-frame compute. |
| Q3 | How does Carrier know where to go? | **Controller sets targetFlag at assignment time.** Carrier never reads route[]. |
| Q4 | amount > 1? | **One task = one physical unit.** amount field removed. 5 wood = 5 independent TransportTask. |
| Q5 | Task cancellation? | **Three phases**: before pickup → immediate; during transit → finish current hop, then discard; after arrival → ignored. |
| Q6 | Blocked retry? | **Event-driven only**. `OnRoadNetworkChanged()` → `RetryBlockedTasks()`. No timers, no polling. |
| Q7 | Warehouse as origin? | **Not special.** All nodes are `FlagId`. Storehouse is just a Cargo source. Controller never checks building type. |
| Q8 | Priority affect route? | **No.** Route planner decides WHERE cargo moves. Priority dispatcher decides WHEN cargo moves. `PickNextTask` never modifies route/hopIndex/targetFlag. |
| Q9 | Who owns a resource? | **Ownership chain**: Ground → Flag inventory (stationary) → TransportTask (in transit) → Carrier (on carrier) → Building inventory (consumed). Never Carrier+Building or Demand+Task simultaneously. |
| Q10 | Is `Update()` a decision loop? | **No.** `Update()` exists only for telemetry (`LogTelemetry` every 600 ticks) and monotonic tick increment (`m_currentTick++` for age bonus). Assignment, route mutation, retries, and state transitions are event-driven from `Notify*` callbacks only. |

## 12. Transport Contract (Phase 8)

> **Economy requests movement. Transport performs movement.**
> Economy never moves resources directly. Transport is a domain service;
> it receives requests, executes routing, and reports completion.
> The economy system never touches a Carrier, never walks a path,
> and never decides which physical flag a resource sits at.

### Ownership chain
```
Ground
  → Flag inventory (stationary)
    → TransportTask (in transit)
      → Carrier (on carrier)
        → Building inventory (consumed)
```

**Violations**:
- A `Carrier` and a `Building` claiming ownership of the same resource simultaneously.
- A `Demand`/`DemandTicket` holding a resource reference while `TransportTask` also references it.
- Economy code calling `Carrier::AssignPhase7Task()` or directly modifying `Flag` inventory.
