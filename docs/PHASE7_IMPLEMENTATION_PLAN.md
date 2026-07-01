# Phase 7 — Implementation Plan

> **Rule: No feature outside the spec.** Only what is in `LOGISTICS_ARCHITECTURE.md`. Extensions after stable baseline.

---

## Phase 7.0 — Specification (DONE) ✅

- [x] `docs/LOGISTICS_ARCHITECTURE.md` — STATUS: FROZEN v1.0
- [x] All Q1–Q7 resolved
- [x] Phase 6b archived as research prototype

---

## Phase 6b ARCHIVE — Return to baseline

- [ ] Verify `kUseTransportJobs = false` in DemandManager.h
- [ ] Verify game boots and runs (construction, delivery, multi-flag)
- [ ] Branch `archive/phase-6b` from current commit
- [ ] Create clean `phase-7` branch from this baseline

---

## Phase 7.1 — Data model skeleton

**Files:**
- `World/TransportTask.h` — TransportTask, TransportRoute, Priority, TaskState, TaskReason
- `World/TransportController.h` — class TransportController (empty shell, no logic)

**Checklist:**
- [ ] `TransportRoute` — `FlagId flags[kMaxRouteLength]` + `count`
- [ ] `TransportTask` — all fields per spec (no `amount`)
- [ ] `Priority` — `classPriority` + `dynamicPriority`
- [ ] `TaskState` — Created, Blocked, WaitingAtSource, Moving, ArrivedAtHop, Cancelled, Delivered
- [ ] `TaskReason` — Construction, Production, Food, WarehouseBalance, Military, Emergency
- [ ] `TransportController` — class declaration, no implementation
- [ ] Compile-time assert: `sizeof(TransportTask) <= 192`

---

## Phase 7.2 — Task management (no carriers) ✅

**Controller can:**
- `CreateTask(type, origin, destination, reason)`
- `FindPath()` via RoadManager → route or Blocked
- Enqueue in `m_waitingByFlag[source]` (linked list via `nextWaiting`)
- Debug API: `GetWaitingCount()`, `PeekWaitingTask()`, `GetBlockedCount()`

**Checklist:**
- [x] `CreateTask()` — allocate from pool, build route, set state
- [x] Pool exhaustion: 257th CreateTask → NULL
- [x] Independent per-flag queues
- [x] Route truncation at kMaxRouteLength (64)
- [x] No-path → TTS_Blocked
- [x] Debug API purity (no side effects)

## 7.2.5 — Controller self-test

- [x] Self-test scenarios documented in `TransportController.cpp`
- [x] All five scenarios pass code review
- [x] Controller stable and testable in isolation

## Phase 7.3 — Carrier integration (staged)

### 7.3.1 — Assignment (no movement) ✅
- Controller: `NotifyCarrierIdle(carrier, atFlag)` → `TryAssignTask()`
- `AcquireWaitingTask()` extracts head from queue
- `AssignTask()` — single IPC point: set `carrier`, `targetFlag`, `state = TTS_Assigned`
- Carrier receives `AssignPhase7Task(task, targetFlag)` — stores only, no routing
- `ValidateAssignment(task, carrier)` — bidirectional assert
- 5 scenarios documented (6–10 in file header)
- No movement, no cargo

### 7.3.2 — PickUp (state only, no Cargo) ✅
- Carrier at source flag → `NotifyCarrierPickedUp(carrier)`
- Controller: `ValidateAssignment(task, c)`, `state = TTS_Moving`
- No Cargo object created yet (logical PickUp only)
- Post-condition: `task->state == Moving && task->carrier == c`

### 7.3.3a — PickUp with Cargo ownership (no movement) ✅
- `NotifyCarrierPickedUp(carrier, cargo)` — Controller links:
  - `task->cargo = cargo; cargo->ownerTask = task; carrier->m_phase7Cargo = cargo`
  - `task->state = TTS_Moving`
- Carrier stores only a pointer to Cargo — no routing, no state changes
- `ValidateOwnership(task)` checks: `task→carrier`, `task→cargo`, `carrier→cargo`, `cargo→ownerTask`
- No movement, no Drop, no AdvanceHop

### 7.3.3b — Walk (pure movement) ✅
- Carrier walks toward `targetFlag` — knows only targetFlag, no route/hopIndex/destination
- Movement is spatial only: `ep` + `walkDir`, no TransportTask fields touched
- On arrival: 3 internal asserts, then `NotifyCarrierArrived(this, targetFlag)`
- Controller `NotifyCarrierArrived` validates: `ValidateAssignment`, `ValidateOwnership`, `ValidateMovement`
- `ValidateMovement` checks: `state == TTS_Moving`, `task->targetFlag == carrier->targetFlag`
- Carrier does NOT call any Controller method during movement
- Controller pointer set once on Assignment (`c->m_phase7Controller = this;`)

### 7.3.4 — Drop & AdvanceHop ✅
- `NotifyCarrierArrived` dispatches: `IsLastHop` → `CompleteDelivery` or `AdvanceHop`
- `AdvanceHop`: hopIndex++, new targetFlag, `state = TTS_WaitingAtSource`, release carrier, re-enqueue, `NotifyCarrierIdle`
- `CompleteDelivery`: unlink cargo, `state = TTS_Delivered`, release carrier, `NotifyCarrierIdle`
- Post-condition asserts on both paths
- Debug trace: `[Transport] AdvanceHop` / `[Transport] Delivered`
- Carrier changes only spatial state; Controller changes only task state

**Checklist:**
- [x] NotifyCarrierArrived → IsLastHop → dispatch
- [x] AdvanceHop — hopIndex++, new targetFlag, WaitingAtSource, release, re-enqueue
- [x] CompleteDelivery — unlink cargo, Delivered, release carrier
- [x] Ownership triangle preserved across hops (task↔cargo persists)
- [x] Debug runtime trace on both paths
- [ ] Phase 7.6: full multi-hop lifecycle test (A→B→C→D)

## Phase 7.4 — Priority dispatching ✅

- [x] `TransportBasePriority` enum: Low(0), Normal(100), High(200), Critical(300)
- [x] `PriorityForReason()` — maps `TransportTaskReason` to base priority
- [x] `TransportTask::basePriority` — immutable, set at creation
- [x] `TransportTask::enqueueOrder` — monotonic FIFO counter, set on `EnqueueWaiting`
- [x] `PickNextTask(flagId)` — linear scan, selects by (priority DESC, enqueueOrder ASC)
- [x] Age bonus — `min(currentTick - createdTick, 200)` computed on selection; no per-frame mutation
- [x] Anti-starvation: old Low(0) task rises to Normal(100) after 100 ticks, caps at 200
- [x] Invariant: PickNextTask never changes route/hopIndex/targetFlag
- [x] Instrumentation: `[Transport] Queue f=N cnt=N best=N` and `[Transport] Dispatch task=N pri=N age=N`
- [x] Removed `PeekWaiting()` / `AcquireWaitingTask()` — replaced by `PickNextTask()` + `RemoveFromQueue()`
- [x] Removed `TransportPriority` struct — replaced by `basePriority` + `enqueueOrder`
- [x] `Update(float)` increments `m_currentTick` for age computation
- [x] 5 test scenarios documented (29–33)

## Phase 7.5 — Cancellation & Blocked retry ✅

- [x] `CancelTask()` — WaitingAtSource (remove from queue, cancel), Assigned (release carrier, cancel), Moving (release, re-enqueue at source), Blocked (cancel)
- [x] `RemoveFromQueue()` — linked-list traversal, O(n) with n ≤ 256
- [x] `RetryBlockedTasks()` — re-run `FindPath()` for each Blocked task; if path found, rebuild route and go to `WaitingAtSource`
- [x] `NotifyRoadNetworkChanged()` → `RetryBlockedTasks()`
- [x] `SetTaskState()` — single point for state transitions, increments `transitionCount`, asserts < 64
- [x] `transitionCount` field on `TransportTask`, initialized to 0
- [x] All state assignments replaced with `SetTaskState()`
- [x] Test: cancel each state (WaitingAtSource, Assigned, Moving, Blocked)
- [x] Test: retry Blocked on road change

## Phase 7.6 — Multi-hop ✅

- [x] `AdvanceHop()` implemented in 7.3.4 — re-assignment cycle verified
- [x] `IsRouteValid()` — checks next hop reachability before AdvanceHop
- [x] `NotifyFlagRemoved(flagId)` — blocks all tasks using that flag
- [x] Mid-route cancellation: road removal → Blocked → Retry → Continue (not restart)
- [x] Diagnostic logging: `[Transport] Hop task=N a/b src=X dst=Y` on each hop
- [x] Diagnostic logging: `[Transport] Complete task=N hops=M transitions=T` on delivery
- [x] 5 multi-hop scenarios documented (24–28)
- [x] Same-carrier handoff (carrier continues through all hops)
- [x] Different-carrier handoff (task->carrier may change, route immutable)

## Phase 7.7 — Load balancing / telemetry ✅

- [x] Carrier utilization: active = Assigned + Moving, total = CarrierManager::GetCarrierCount()
- [x] avgWait = totalAgeOfWaiting / waitingCount (snapshot every 600 ticks)
- [x] Log: `[Transport] Utilization 18/24 active avgWait=43`
- [x] Queue pressure: per-flag scan finds max depth + oldest age
- [x] Log: `[Transport] Flag=8 q=14 oldest=311 blocked=2`
- [x] Fairness validation: `assert(oldestWaitingAge < 10000)` (~167s at 60fps)
- [x] SetCarrierManager() dependency injection
- [x] LogTelemetry() called periodically from Update()
- [x] 3 test scenarios documented (34–36)

## Phase 8 — Economy integration

### Phase 8.1 — Demand → TransportTask adapter (bridge)

- [ ] DemandManager creates TransportTasks via bridge (old pipeline still alive)
- [ ] Invariant: one Demand = at most one active TransportTask
- [ ] Add `CreateTransportTask()` that maps Demand → TransportTask
- [ ] CargoManager reports delivery completion to DemandManager

### Phase 8.2 — Resource ownership migration

- [ ] Define ownership chain: Ground → Flag → TransportTask → Carrier → Building
- [ ] Never Carrier+Building or Demand+Task simultaneously
- [ ] Runtime audit: `[Resource] id=712 owner=TransportTask(17)` (debug)
- [ ] Remove old ownership paths (Reserve/Allocate)

### Phase 8.3 — Parallel validation mode

- [ ] old TransportJobManager = observe only
- [ ] new TransportController = execute
- [ ] Log: `[MIGRATION] demand=81 old=flag12 new=flag12 OK`
- [ ] Run all scenarios with both systems active

### Phase 8.4 — Remove legacy transport

- [ ] All scenarios pass: wood→warehouse, warehouse→construction, mine→smelter, food→worker, blocked road recovery, flag deletion
- [ ] Remove `TransportJobManager`, `DemandTicket`, old Demand pipeline
- [ ] Remove `kUseTransportJobs` flag
- [ ] Remove `Reserve()`, `FindBestDemand()`, `Allocate()`
- [ ] Remove old Carrier routing code
- [ ] Remove old ownership code
- [ ] Transport tests green
- [ ] Run full soak: T1 (single hop), T2 (multi-hop), T3 (cancellation), T4 (road change), T5 (30-min)
