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

## Phase 7.2 — Task management (no carriers)

**Controller can:**
- `CreateTask(type, origin, destination, reason)`
- `FindPath()` via RoadManager → route or Blocked
- Enqueue in `m_waitingByFlag[source]`
- `CancelTask()` — WaitingAtSource only (immediate)

**Checklist:**
- [ ] `CreateTask()` — allocate from pool, build route, set state
- [ ] `RetryBlockedTasks()` — called from `OnRoadNetworkChanged()`
- [ ] `CancelTask()` — remove from queue, free

---

## Phase 7.3 — Carrier integration (single hop)

**Controller can:**
- Register carriers (`m_carriers[]`, `m_idleCarriers`)
- `TryAssignTask(carrier, flagId)` — match task → carrier
- Carrier receives `AssignTask(task, targetFlag)`
- Carrier walks, picks up cargo, drops at target
- `OnCarrierReachedTarget()` → `AdvanceHop()` → delivered

**Checklist:**
- [ ] Carrier registration in Controller
- [ ] `OnCarrierIdle()` → `TryAssignTask()`
- [ ] `OnCarrierReachedTarget()` → `AdvanceHop()` → Delivered
- [ ] Cargo integration: create Cargo on pickup, release on drop
- [ ] Test: warehouse → single flag, task lifecycle complete

---

## Phase 7.4 — Multi-hop

**Controller can:**
- `AdvanceHop()` → `hopIndex++`, re-enqueue or deliver
- `OnCarrierReachedTarget()` at intermediate flag → ArrivedAtHop → requeue

**Checklist:**
- [ ] `AdvanceHop()` with hopIndex check
- [ ] Re-enqueue at next flag's waiting queue
- [ ] Test: warehouse → flag A → flag B, verify handoff
- [ ] Test: 3+ hop chain

---

## Phase 7.5 — Priority dispatching

- [ ] `SelectBestTask()` with priority score formula
- [ ] DynamicPriority read at selection time (age-based)
- [ ] Test: multiple tasks at same flag, verify priority order
- [ ] Test: anti-starvation (old task overtakes newer higher-priority task)

---

## Phase 7.6 — Cancellation & Blocked retry

- [ ] `CancelTask()` — WaitingAtSource (immediate), Moving (finish hop), arrived (ignore)
- [ ] `OnRoadNetworkChanged()` → `RetryBlockedTasks()`
- [ ] `OnFlagRemoved()` → `CleanupForFlag()`
- [ ] Test: cancel before pickup
- [ ] Test: cancel during transit
- [ ] Test: remove road with Blocked task → retry succeeds

---

## Phase 7.7 — Legacy removal

- [ ] Remove `TransportJobManager`, `DemandTicket`, old Demand pipeline
- [ ] Remove `kUseTransportJobs` flag
- [ ] Remove `Reserve()`, `FindBestDemand()`, `Allocate()`
- [ ] Remove old Carrier routing code
- [ ] Run full soak: T1 (single hop), T2 (multi-hop), T3 (cancellation), T4 (road change), T5 (30-min)
