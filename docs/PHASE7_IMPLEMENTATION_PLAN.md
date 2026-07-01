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

### 7.3.1 — Assignment (no movement)
- Controller: `NotifyCarrierIdle(carrier, atFlag)` → `TryAssignTask()`
- Pop task from `m_waitingHead[atFlag]`, set `carrier`, `targetFlag`, `state = TTS_Assigned`
- Carrier receives `AssignTask(task, targetFlag)`
- No movement, no cargo

### 7.3.2 — PickUp
- Carrier at source flag → `NotifyCarrierPickedUp(carrier)`
- Controller: `state = TTS_Moving`, allocate Cargo (stub OK)
- Cargo linked to task (`task->cargo = c`, `c->task = task`)

### 7.3.3 — Walk
- Carrier walks toward `targetFlag`
- Controller not involved (pure Carrier movement)

### 7.3.4 — Drop
- Carrier arrives at `targetFlag` → `NotifyCarrierArrived(carrier, flagId)`
- Controller: `NotifyCarrierDropped(carrier, flagId)` → `AdvanceHop(task)`
- `AdvanceHop`: hopIndex++ → if last: `TTS_Delivered`; else: re-enqueue at next flag's queue, `state = TTS_WaitingAtSource`
- Carrier becomes idle → `NotifyCarrierIdle(carrier, atFlag)`

**Checklist:**
- [ ] Controller: `TryAssignTask()` — pop from waitingHead, assign to carrier
- [ ] `NotifyCarrierIdle` → `TryAssignTask()`
- [ ] `NotifyCarrierPickedUp` → state→Moving, Cargo stub
- [ ] `NotifyCarrierArrived` → `AdvanceHop()`
- [ ] `AdvanceHop()` — hopIndex++, requeue or deliver
- [ ] Test: single hop warehouse→flag, full lifecycle
- [ ] Task invariants maintained (carrier/cargo NULL checks)

## Phase 7.4 — Priority dispatching

- [ ] `SelectBestTask()` with priority score formula
- [ ] DynamicPriority read at selection time (age-based)
- [ ] Test: multiple tasks at same flag, verify priority order
- [ ] Test: anti-starvation (old task overtakes newer higher-priority task)

## Phase 7.5 — Cancellation & Blocked retry

- [ ] `CancelTask()` — WaitingAtSource (immediate), Moving (finish hop), arrived (ignore)
- [ ] `OnRoadNetworkChanged()` → `RetryBlockedTasks()`
- [ ] `OnFlagRemoved()` → `CleanupForFlag()`
- [ ] Test: cancel before pickup
- [ ] Test: cancel during transit
- [ ] Test: remove road with Blocked task → retry succeeds

## Phase 7.6 — Multi-hop

- [ ] `AdvanceHop()` with hopIndex check (moved from 7.3.4)
- [ ] Re-enqueue at next flag's waiting queue
- [ ] Test: warehouse → flag A → flag B, verify handoff
- [ ] Test: 3+ hop chain

## Phase 7.7 — Legacy removal

- [ ] Remove `TransportJobManager`, `DemandTicket`, old Demand pipeline
- [ ] Remove `kUseTransportJobs` flag
- [ ] Remove `Reserve()`, `FindBestDemand()`, `Allocate()`
- [ ] Remove old Carrier routing code
- [ ] Run full soak: T1 (single hop), T2 (multi-hop), T3 (cancellation), T4 (road change), T5 (30-min)
