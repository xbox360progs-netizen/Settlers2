// Phase 7 ��� Controller. CreateTask + waiting queue + Assignment + Priority.
//
// Self-test scenarios (Phase 7.2.5):
//
//   1. Pool exhaustion:
//      Create 256 tasks with valid routes.
//      257th CreateTask() ��� returns NULL.
//
//   2. Independent per-flag queues:
//      CreateTask(A���B), CreateTask(A���C), CreateTask(D���E).
//      GetWaitingCount(A) == 2, GetWaitingCount(D) == 1.
//      PeekWaitingTask(A) == first task, PeekWaitingTask(D) == third.
//
//   3. Route truncation at kMaxRouteLength (64):
//      A route longer than 64 flags ��� first 64 flags stored, rest discarded.
//      Task state == WaitingAtSource (not Blocked).
//
//   4. No-path scenarios:
//      CreateTask between unconnected flags ��� state == TTS_Blocked.
//      GetBlockedCount() == 1.
//
//   5. Debug API purity:
//      Repeated GetWaitingCount() and PeekWaitingTask() calls with no
//      intervening CreateTask()/CancelTask() return identical results.
//
// Phase 7.3.1 ��� Assignment scenarios:
//
//   6. One carrier, one task:
//      CreateTask(A���B). NotifyCarrierIdle(c1, A).
//      Task1 state == TTS_Assigned.
//      Carrier1.m_phase7Task == Task1.
//      Waiting queue at A is empty.
//
//   7. No carrier (no NotifyCarrierIdle):
//      CreateTask(A���B). No idle notification.
//      Task1 state == TTS_WaitingAtSource.
//      Waiting queue at A has 1 entry.
//
//   8. No task, idle carrier:
//      NotifyCarrierIdle(c1, A) with empty queue.
//      Nothing changes. Carrier stays idle.
//
//   9. Two carriers, one task:
//      CreateTask(A���B). NotifyCarrierIdle(c1, A). NotifyCarrierIdle(c2, A).
//      Only c1 gets assigned. c2 finds empty queue.
//
//  10. Two tasks, one carrier:
//      CreateTask(A���B), CreateTask(A���C). NotifyCarrierIdle(c1, A).
//      Only Task1 assigned. Task2 remains in queue.
//
// Phase 7.3.2 ��� PickUp:
//
//  11. Assigned ��� Moving (no Cargo):
//      NotifyCarrierIdle(c1, A) ��� Assign Task1.
//      NotifyCarrierPickedUp(c1) ��� Task1 state == TTS_Moving.
//      Carrier unchanged (no Cargo created).
//
// Phase 7.3.3a ��� PickUp with Cargo ownership:
//
//  12. Full ownership triangle:
//      NotifyCarrierIdle(c1, A) ��� Assign Task1.
//      NotifyCarrierPickedUp(c1, cargo1) ���
//        Task1.cargo == cargo1
//        cargo1.ownerTask == Task1
//        c1.m_phase7Cargo == cargo1
//        Task1.state == TTS_Moving
//      ValidateOwnership(Task1) passes.
//
// Phase 7.3.3b ��� Walk:
//
//  13. Walk toward targetFlag:
//      Carrier c1: state == TTS_Moving, targetFlag == route.flags[1].
//      c1.Update(dt) ��� ep moves toward targetFlag.
//      On arrival: asserts fire, NotifyCarrierArrived(c1, targetFlag).
//      Controller validates: ValidateAssignment, ValidateOwnership,
//        ValidateMovement (state==Moving, targetFlag matches).
//
//  14. Carrier never touches TransportTask:
//      During c1.Update(), no task->state, hopIndex, or route changes.
//      Carrier moves ep/walkDir only (spatial movement).
//
// Phase 7.3.4 ��� AdvanceHop / CompleteDelivery:
//
//  15. Single hop delivery:
//      Carrier c1 arrives at targetFlag B (destination, last hop).
//      NotifyCarrierArrived ��� IsLastHop==true ��� CompleteDelivery.
//      task->state == TTS_Delivered.
//      task->cargo == NULL, task->carrier == NULL.
//      carrier->m_phase7Task == NULL (freed).
//
//  16. Intermediate hop (not last):
//      Route A���B���C, carrier arrives at B (not destination).
//      AdvanceHop: hopIndex 0���1, targetFlag B���C.
//      state == TTS_WaitingAtSource.
//      Task re-enqueued at B.
//      carrier released, NotifyCarrierIdle(c, B).
//      TryAssignTask re-assigns same task (next hop to C).
//
//  17. Ownership preserved across hop:
//      After AdvanceHop + re-assign + NotifyCarrierPickedUp:
//      task->cargo unchanged (same cargo object).
//      ValidateOwnership passes.
//      Carrier walks to C.
//
// Phase 7.5 ��� Cancellation & Blocked retry:
//
//  18. Cancel WaitingAtSource:
//      CreateTask(A���B). CancelTask(taskId).
//      Task removed from queue. state == TTS_Cancelled.
//
//  19. Cancel Assigned:
//      CreateTask(A���B). Assign to c1. CancelTask(taskId).
//      carrier released (m_phase7Task==NULL).
//      state == TTS_Cancelled.
//
//  20. Cancel Moving:
//      CreateTask(A���B). Assign + PickUp. CancelTask(taskId).
//      carrier released, task re-enqueued at A.
//      state == TTS_WaitingAtSource.
//
//  21. Cancel Blocked:
//      Route blocked at creation. CancelTask(taskId).
//      state == TTS_Cancelled.
//
//  22. RetryBlockedTasks:
//      Two blocked tasks. Road network changed.
//      NotifyRoadNetworkChanged ��� RetryBlockedTasks.
//      If path found: state ��� WaitingAtSource, enqueued.
//      If still blocked: state stays TTS_Blocked.
//
//  23. transitionCount safety:
//      Every state change increments transitionCount.
//      Infinite loop (���64 transitions) triggers assert.
//      Normal lifecycle: Created���Blocked/WaitingAtSource���Assigned���Moving
//        ���Arrived���AdvanceHop���...���Delivered produces ~6���12 transitions.
//
// Phase 7.6 ��� Multi-hop:
//
//  24. Three-hop chain A���B���C���D:
//      CreateTask(A, D). Full lifecycle with 4 flags:
//        hop=1/3 A���B, arrive, AdvanceHop
//        hop=2/3 B���C, arrive, AdvanceHop
//        hop=3/3 C���D, arrive, CompleteDelivery
//      All 3 hops complete with same cargo object.
//      route unchanged through entire lifecycle.
//      Diagnostic output:
//        [Transport] Hop task=17 1/3 src=8 dst=11
//        [Transport] Hop task=17 2/3 src=11 dst=14
//        [Transport] Hop task=17 3/3 src=14 dst=17
//        [Transport] Complete task=17 hops=3 transitions=12 dest=17
//
//  25. Same-carrier handoff:
//      carrier1 assigned at A, walks to B, AdvanceHop releases.
//      NotifyCarrierIdle(c1, B) ��� same carrier assigned to next hop.
//      carrier1 walks B���C, same pattern to D.
//      ValidateOwnership passes at every intermediate flag.
//
//  26. Different-carrier handoff:
//      carrier1 A���B, drops at B.
//      carrier2 idle at B picks up next hop.
//      carrier2 B���C, drops at C.
//      carrier3 idle at C picks up last hop.
//      carrier3 C���D, delivers.
//      task->carrier changes each hop; route unchanged.
//
//  27. Mid-route cancellation:
//      route A���B���C���D. Road B���C removed during Moving A���B.
//      Arrived at B. IsRouteValid(B���D) == false.
//      state ��� TTS_Blocked, carrier released.
//      Route rebuilt ��� continue from B (not restart from A).
//
//  28. Flag removal:
//      Flag B removed. All tasks whose route includes B ��� Blocked.
//      WaitingAtSource at B: RemoveFromQueue ��� Blocked.
//      Assigned to B: release carrier ��� Blocked.
//      Moving toward B: release carrier, re-enqueue at A ��� Blocked.
//      Delivered/Cancelled: no-op.
//
// Phase 7.4 ��� Priority dispatching:
//
//  29. Priority order:
//      Queue at flag A has tasks: T1(pri=300), T2(pri=100), T3(pri=200).
//      PickNextTask(A) ��� T1 (highest priority).
//
//  30. FIFO within same priority:
//      Queue at flag A: T1(pri=100, order=1), T2(pri=100, order=2).
//      PickNextTask(A) ��� T1 (oldest enqueue order).
//
//  31. Age bonus prevents starvation:
//      T1(pri=0, created=0), T2(pri=100, created=1).
//      At tick 100: effective scores: T1=0+100=100, T2=100+99=199 ��� T2.
//      T1 score rises with age but capped at +200.
//      Against same priority: older task always wins.
//
//  32. Priority does NOT affect route:
//      AssignTask sets targetFlag from route[hopIndex+1].
//      PickNextTask only changes which task gets assigned next.
//      Route, hopIndex, targetFlag are unchanged by selection.
//
//  33. Instrumentation:
//      [Transport] Dispatch task=17 pri=300 age=12
//      [Transport] Queue f=8 cnt=5 best=17
//
// Phase 7.7 ��� Load balancing / telemetry:
//
//  34. Carrier utilization:
//      Every 600 ticks, LogTelemetry scans all tasks:
//        active = TTS_Assigned + TTS_Moving
//        totalCarriers = CarrierManager::GetCarrierCount()
//        avgWait = totalAgeOfWaiting / waitingCount
//      Log: [Transport] Utilization 18/24 active avgWait=43
//
//  35. Queue pressure:
//      Per-flag scan finds max queue depth, oldest task age.
//      Log: [Transport] Flag=8 q=14 oldest=311 blocked=2
//
//  36. Fairness validation:
//      assert(oldestWaitingAge < 10000).
//      10 000 ticks ~ 167 seconds at 60 fps.
//      Any task waiting longer hits the assert.
//      Warehouse tasks (pri=0) must be dispatched within this window.

#include <vector>
#include <cassert>
#include <cstdio>
#include "TransportController.h"
#include "TransportTask.h"
#include "TransportNode.h"
#include "Carrier.h"
#include "Cargo.h"
#include "../Core/ResourceDebug.h"
#include "../Interfaces/IRoadGraph.h"
#include "../Interfaces/IFlagInventory.h"
#include "../Interfaces/ICargoRepository.h"
#include "../Interfaces/IDemandService.h"
#include "../World/WorldModel.h"

namespace World {

    TransportController::TransportController(
        IRoadGraph& roadGraph,
        IFlagInventory& inventory,
        ICargoRepository& cargo,
        IDemandService& demand)
        : m_nextTaskId(1)
        , m_activeCount(0)
        , m_currentTick(0)
        , m_enqueueCounter(1)
        , m_recentDeliveryCount(0)
        , m_roads(roadGraph)
        , m_inventory(inventory)
        , m_cargo(cargo)
        , m_demand(demand)
    {
        for (int i = 0; i < kMaxTasks; ++i) {
            m_pool[i].id = 0;
            m_pool[i].resource = ResourceType_None;
            m_pool[i].state = TTS_Delivered;
            m_pool[i].cargo = NULL;
            m_pool[i].carrier = NULL;
            m_pool[i].nextWaiting = NULL;
            m_pool[i].transitionCount = 0;
        }
        for (uint32_t i = 0; i < kMaxFlags; ++i) {
            m_waitingHead[i] = NULL;
            m_waitingTail[i] = NULL;
        }
        for (int i = 0; i < kMaxCarriers; ++i) {
            m_carriers[i].state = TCS_Idle;
            m_carriers[i].taskId = 0;
            m_carriers[i].cargoType = ResourceType_None;
            m_carriers[i].cargoAmount = 0;
        }
    }

    TransportController::~TransportController() {}

    // ������ State transitions ������������������������������������������������������������������������������������������������������������������������������������������������

    void TransportController::SetTaskState(TransportTask* task, TransportTaskState newState)
    {
        assert(task != NULL);
        task->transitionCount++;
        assert(task->transitionCount < 64);
        task->state = newState;
    }

    // ������ Pool allocation ������������������������������������������������������������������������������������������������������������������������������������������������������

    TransportTask* TransportController::AllocateTask()
    {
        for (int i = 0; i < kMaxTasks; ++i) {
            if (m_pool[i].state == TTS_Delivered && m_pool[i].id == 0) {
                m_activeCount++;
                return &m_pool[i];
            }
        }
        return NULL;
    }

    void TransportController::FreeTask(TransportTask* task)
    {
        ReleaseCarrierForTask(task->id);
        task->id = 0;
        task->state = TTS_Delivered;
        task->carrier = NULL;
        task->cargo = NULL;
        task->resource = ResourceType_None;
        task->reason = TTR_Construction;
        task->hopIndex = 0;
        task->targetFlag = 0;
        task->enqueueOrder = 0;
        task->basePriority = 0;
        task->observerTicketId = 0;
        task->createdTick = 0;
        task->transitionCount = 0;
        for (int i = 0; i < task->route.count; ++i) {
            task->route.flags[i] = 0;
        }
        task->route.count = 0;
        m_activeCount--;
    }

    // ������ Waiting queue ������������������������������������������������������������������������������������������������������������������������������������������������������������

    void TransportController::EnqueueWaiting(TransportTask* task, FlagId atFlag)
    {
        assert(atFlag < kMaxFlags);
        assert(task != NULL);
        task->nextWaiting = NULL;
        task->enqueueOrder = m_enqueueCounter++;

        if (m_waitingTail[atFlag] != NULL) {
            m_waitingTail[atFlag]->nextWaiting = task;
            m_waitingTail[atFlag] = task;
        } else {
            m_waitingHead[atFlag] = task;
            m_waitingTail[atFlag] = task;
        }
    }

    // ������ Waiting queue ��� internal helpers ���������������������������������������������������������������������������������������������������

    // Phase 7.4 ��� PickNextTask selects the best task from a per-flag queue.
    // Selection rule: (priority DESC, enqueueOrder ASC).
    // Age bonus = min(currentTick - createdTick, 200) added to basePriority.
    // This prevents starvation of old low-priority tasks.
    TransportTask* TransportController::PickNextTask(FlagId flagId)
    {
        if (flagId >= kMaxFlags) return NULL;

        TransportTask* best = NULL;
        uint16_t bestScore = 0;
        uint16_t bestOrder = 0;

        TransportTask* cur = m_waitingHead[flagId];
        while (cur) {
            uint16_t age = (uint16_t)(m_currentTick - cur->createdTick);
            if (age > 200) age = 200;
            uint16_t score = cur->basePriority + age;

            if (!best || score > bestScore || (score == bestScore && cur->enqueueOrder < bestOrder)) {
                best = cur;
                bestScore = score;
                bestOrder = cur->enqueueOrder;
            }
            cur = cur->nextWaiting;
        }

#ifdef _DEBUG
        if (best) {
            uint16_t cnt = GetWaitingCount(flagId);
            char dbg[256];
            _snprintf(dbg, sizeof(dbg),
                "[Transport] Queue f=%u cnt=%u best=%u\n",
                flagId, cnt, best->id);
            std::printf("%s", dbg);
        }
#endif

        return best;
    }

    // ������ Queue management ���������������������������������������������������������������������������������������������������������������������������������������������������

    void TransportController::RemoveFromQueue(TransportTask* task)
    {
        assert(task != NULL);
        // Find which flag this task is queued at by scanning all queues
        for (uint32_t f = 0; f < kMaxFlags; ++f) {
            TransportTask* prev = NULL;
            TransportTask* cur = m_waitingHead[f];
            while (cur) {
                if (cur == task) {
                    // Remove from linked list
                    if (prev) {
                        prev->nextWaiting = cur->nextWaiting;
                    } else {
                        m_waitingHead[f] = cur->nextWaiting;
                    }
                    if (m_waitingTail[f] == cur) {
                        m_waitingTail[f] = prev;
                    }
                    cur->nextWaiting = NULL;
                    return;
                }
                prev = cur;
                cur = cur->nextWaiting;
            }
        }
    }

    // ������ Ownership validation ���������������������������������������������������������������������������������������������������������������������������������������

    void TransportController::ValidateAssignment(const TransportTask* task, const Carrier* c) const
    {
        assert(task != NULL);
        assert(c != NULL);
        assert(task->carrier == c);
        assert(c->m_phase7Task == task);
    }

    void TransportController::ValidateOwnership(const TransportTask* task) const
    {
        assert(task != NULL);
        assert(task->carrier != NULL);
        assert(task->cargo != NULL);
        Carrier* c = static_cast<Carrier*>(task->carrier);
        assert(c->m_phase7Cargo == task->cargo);
        assert(task->cargo->ownerTask == task);
    }

    void TransportController::ValidateMovement(const TransportTask* task) const
    {
        assert(task != NULL);
        assert(task->state == TTS_Moving);
        Carrier* c = static_cast<Carrier*>(task->carrier);
        assert(c != NULL);
        assert(task->targetFlag == c->m_phase7TargetFlag);
    }

    // ������ Retry / recovery ������������������������������������������������������������������������������������������������������������������������������������������������������

    bool TransportController::IsRouteValid(const TransportTask* task) const
    {
        assert(task != NULL);
        if (task->hopIndex + 2 >= task->route.count) return true;

        TransportRoute route;
        return m_roads.FindRoute(
            task->route.flags[task->hopIndex + 1],
            task->route.flags[task->route.count - 1],
            route);
    }

    void TransportController::RetryBlockedTasks()
    {
        for (int i = 0; i < kMaxTasks; ++i) {
            if (m_pool[i].state != TTS_Blocked) continue;

            TransportTask* task = &m_pool[i];
            TransportRoute route;

            if (m_roads.FindRoute(
                    task->route.flags[task->hopIndex],
                    task->route.flags[task->route.count - 1],
                    route))
            {
                task->route = route;
                task->hopIndex = 0;
                SetTaskState(task, TTS_WaitingAtSource);
                EnqueueWaiting(task, route.flags[0]);
            }
        }
    }

    // ������ Hop management (Phase 7.3.4) ������������������������������������������������������������������������������������������������������������������

    bool TransportController::IsLastHop(const TransportTask* task) const
    {
        assert(task != NULL);
        // The destination is at route.count - 1. We just arrived at
        // route[hopIndex + 1]. If that equals route.count - 1, it's the
        // final destination.
        return (task->hopIndex + 1 >= task->route.count - 1);
    }

    void TransportController::AdvanceHop(Carrier* c, TransportTask* task)
    {
        assert(task->hopIndex + 1 < task->route.count);
        assert(task->cargo != NULL);

        FlagId oldTargetFlag = task->targetFlag;
        FlagId arrivedFlagId = task->route.flags[task->hopIndex + 1];

        // Advance hop index ��� we are now at the arrived flag
        task->hopIndex++;
        task->targetFlag = task->route.flags[task->hopIndex + 1];
        SetTaskState(task, TTS_WaitingAtSource);

        // Release carrier ��� cargo stays at the flag with the task
        task->carrier = NULL;
        c->m_phase7Task = NULL;
        c->m_phase7TargetFlag = 0;
        c->m_phase7Cargo = NULL;
        c->m_cargo = NULL;

        // Re-enqueue at current flag for pickup to next hop
        EnqueueWaiting(task, arrivedFlagId);

        // Post-conditions
        assert(task->state == TTS_WaitingAtSource);
        assert(task->targetFlag != oldTargetFlag);
        assert(task->carrier == NULL);
        assert(task->cargo != NULL);
        assert(c->m_phase7Task == NULL);

#ifdef _DEBUG
        // Runtime trace for debugging
        char dbg[256];
        _snprintf(dbg, sizeof(dbg),
            "[Transport] Hop task=%u %u/%u src=%u dst=%u\n",
            task->id, task->hopIndex, task->route.count - 1,
            arrivedFlagId, task->targetFlag);
        std::printf("%s", dbg);
#endif

        // Carrier becomes idle ��� check for next hop
        NotifyCarrierIdle(c, arrivedFlagId);
    }

    void TransportController::CompleteDelivery(Carrier* c, TransportTask* task)
    {
        assert(task->cargo != NULL);

        FlagId destFlagId = task->route.flags[task->route.count - 1];
        Cargo* cargo = task->cargo;
        ResourceType resType = cargo->type;
        uint8_t amount = cargo->amount;
        uint32_t cargoId = cargo->id;

        // Unlink cargo from carrier first
        c->m_phase7Cargo = NULL;
        c->m_cargo = NULL;

        // Release cargo — resource goes out of tracked system (legacy path)
        task->cargo = NULL;
        m_cargo.Release(cargoId);

        if (task->observerTicketId > 0) {
            m_demand.CompleteDemand(task->observerTicketId);
        }

#ifdef _DEBUG
        {
            char dbg[256];
            _snprintf(dbg, sizeof(dbg),
                "[Transport] Delivered task=%u type=%s dest=flag%u\n",
                task->id, ResourceTypeToString(resType), destFlagId);
            std::printf("%s", dbg);
        }
#endif

        SetTaskState(task, TTS_Delivered);

        if (m_recentDeliveryCount < kMaxRecentDeliveries) {
            m_recentDeliveries[m_recentDeliveryCount].resource = task->resource;
            m_recentDeliveries[m_recentDeliveryCount].destinationFlag = destFlagId;
            m_recentDeliveries[m_recentDeliveryCount].reason = task->reason;
            m_recentDeliveryCount++;
        }

        task->carrier = NULL;
        c->m_phase7Task = NULL;
        c->m_phase7TargetFlag = 0;

        assert(task->state == TTS_Delivered);
        assert(task->cargo == NULL);
        assert(task->carrier == NULL);
        assert(c->m_phase7Task == NULL);

#ifdef _DEBUG
        {
            char dbg2[256];
            _snprintf(dbg2, sizeof(dbg2),
                "[Transport] Complete task=%u hops=%u transitions=%u dest=%u\n",
                task->id, task->route.count - 1,
                task->transitionCount, destFlagId);
            std::printf("%s", dbg2);
        }
#endif

        FreeTask(task);
        NotifyCarrierIdle(c, destFlagId);
    }

    // ������ Assignment (Phase 7.3.1) ���������������������������������������������������������������������������������������������������������������������������

    // TryAssignTask is the assignment policy:
    //   1. Peek at the waiting queue
    //   2. No task ��� return NULL (carrier stays idle)
    //   3. Acquire the task (remove from queue)
    //   4. Assign it (set state, link carrier)
    TransportTask* TransportController::TryAssignTask(void* carrier, FlagId atFlag)
    {
        if (!carrier) return NULL;

        TransportTask* task = PickNextTask(atFlag);
        if (!task) return NULL;

        RemoveFromQueue(task);
        AssignTask(carrier, task);

#ifdef _DEBUG
        uint16_t age = (uint16_t)(m_currentTick - task->createdTick);
        if (age > 200) age = 200;
        char dbg[256];
        _snprintf(dbg, sizeof(dbg),
            "[Transport] Dispatch task=%u pri=%u age=%u\n",
            task->id, task->basePriority + age, age);
        std::printf("%s", dbg);
#endif

        return task;
    }

    // AssignTask is the single point where Carrier ��� Task linkage is created.
    // All invariants are checked here.
    void TransportController::AssignTask(void* carrier, TransportTask* task)
    {
        Carrier* c = static_cast<Carrier*>(carrier);
        assert(task->state == TTS_WaitingAtSource);
        assert(task->carrier == NULL);
        assert(c->m_phase7Task == NULL);     // Carrier must be idle

        task->carrier = c;
        SetTaskState(task, TTS_Assigned);
        // First hop from source ��� route.flags[0] is the source, flags[1] is the first hop target
        task->targetFlag = task->route.flags[task->hopIndex + 1];

        c->AssignPhase7Task(task, task->targetFlag);
        c->m_phase7Controller = this;

        // Post-conditions
        assert(task->state == TTS_Assigned);
        assert(task->hopIndex + 1 < task->route.count);
        assert(c->m_phase7TargetFlag == task->targetFlag);
        ValidateAssignment(task, c);
    }

    // ������ Lifecycle ������������������������������������������������������������������������������������������������������������������������������������������������������������������������

    TransportTask* TransportController::CreateTask(
        ResourceType resource,
        FlagId origin,
        FlagId destination,
        TransportTaskReason reason)
    {
        if (resource == ResourceType_None) return NULL;
        if (origin == destination) return NULL;

        TransportTask* task = AllocateTask();
        if (!task) return NULL;

        task->id = m_nextTaskId++;
        task->resource = resource;
        task->reason = reason;
        SetTaskState(task, TTS_Created);
        task->hopIndex = 0;
        task->targetFlag = destination;
        task->cargo = NULL;
        task->carrier = NULL;
        task->createdTick = m_currentTick;
        task->transitionCount = 0;
        task->nextWaiting = NULL;
        task->basePriority = PriorityForReason(reason);
        task->enqueueOrder = 0;
        task->observerTicketId = 0;

        TransportRoute route;
        if (m_roads.FindRoute(origin, destination, route)) {
            task->route = route;
            SetTaskState(task, TTS_WaitingAtSource);
            EnqueueWaiting(task, origin);
        } else {
            SetTaskState(task, TTS_Blocked);
        }

        return task;
    }

    void TransportController::CancelTask(TransportTaskId taskId)
    {
        TransportTask* task = GetTaskById(taskId);
        if (!task) return;

        switch (task->state) {

        case TTS_WaitingAtSource:
            RemoveFromQueue(task);
            SetTaskState(task, TTS_Cancelled);
            break;

        case TTS_Assigned:
            ReleaseCarrierForTask(task->id);
            SetTaskState(task, TTS_Cancelled);
            break;

        case TTS_Moving:
            ReleaseCarrierForTask(task->id);
            EnqueueWaiting(task, task->route.flags[task->hopIndex]);
            SetTaskState(task, TTS_WaitingAtSource);
            break;

        case TTS_Blocked:
            SetTaskState(task, TTS_Cancelled);
            break;

        default:
            break;
        }
    }

    // ������ Event callbacks (stubs until Phase 7.3+) ���������������������������������������������������������������������������

    void TransportController::NotifyCarrierIdle(void* carrier, FlagId atFlag)
    {
        // Carrier became idle at a flag. Try to assign the next waiting task.
        TryAssignTask(carrier, atFlag);
    }
    void TransportController::NotifyCarrierArrived(void* carrier, FlagId flagId)
    {
        if (!carrier) return;
        Carrier* c = static_cast<Carrier*>(carrier);
        TransportTask* task = c->m_phase7Task;
        if (!task) return;
        ValidateAssignment(task, c);
        ValidateOwnership(task);
        ValidateMovement(task);
        assert(flagId == c->m_phase7TargetFlag);
        assert(task->state == TTS_Moving);

        if (IsLastHop(task)) {
            CompleteDelivery(c, task);
        } else if (IsRouteValid(task)) {
            AdvanceHop(c, task);
        } else {
            // Next hop is blocked ��� stay at current flag, release carrier
            SetTaskState(task, TTS_Blocked);
            task->carrier = NULL;
            c->m_phase7Task = NULL;
            c->m_phase7TargetFlag = 0;
            c->m_phase7Cargo = NULL;
            c->m_cargo = NULL;
            assert(task->carrier == NULL);
            assert(c->m_phase7Task == NULL);
            NotifyCarrierIdle(c, flagId);
        }
    }
    void TransportController::NotifyCarrierPickedUp(void* carrier, void* cargo)
    {
        if (!carrier || !cargo) return;
        Carrier* c = static_cast<Carrier*>(carrier);
        Cargo* cargoObj = static_cast<Cargo*>(cargo);
        TransportTask* task = c->m_phase7Task;
        if (!task) return;
        ValidateAssignment(task, c);
        assert(task->state == TTS_Assigned);

        // Ownership triangle: link Task ��� Cargo ��� Carrier
        task->cargo = cargoObj;
        cargoObj->ownerTask = task;
        c->m_phase7Cargo = cargoObj;

        SetTaskState(task, TTS_Moving);

        assert(task->state == TTS_Moving);
        ValidateOwnership(task);
    }
    void TransportController::NotifyCarrierDropped(void* /*carrier*/, FlagId /*flagId*/) {}
    void TransportController::NotifyRoadNetworkChanged()
    {
        RetryBlockedTasks();
    }
    void TransportController::NotifyFlagRemoved(FlagId flagId)
    {
        // Scan all active tasks ��� if route includes the removed flag, block them
        for (int i = 0; i < kMaxTasks; ++i) {
            TransportTask* task = &m_pool[i];
            if (task->id == 0) continue; // unused slot

            // Check if this flag appears anywhere in the route
            bool usesFlag = false;
            for (uint8_t r = 0; r < task->route.count; ++r) {
                if (task->route.flags[r] == flagId) {
                    usesFlag = true;
                    break;
                }
            }
            if (!usesFlag) continue;

            switch (task->state) {

            case TTS_WaitingAtSource:
                RemoveFromQueue(task);
                SetTaskState(task, TTS_Blocked);
                break;

            case TTS_Assigned:
                ReleaseCarrierForTask(task->id);
                SetTaskState(task, TTS_Blocked);
                break;

            case TTS_Moving:
                ReleaseCarrierForTask(task->id);
                EnqueueWaiting(task, task->route.flags[task->hopIndex]);
                SetTaskState(task, TTS_Blocked);
                break;

            default:
                // Delivered, Cancelled, already Blocked ��� no-op
                break;
            }
        }
    }

    // ������ Query ������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������

    int TransportController::GetActiveTaskCount() const { return m_activeCount; }

    TransportTask* TransportController::GetTaskById(TransportTaskId taskId)
    {
        for (int i = 0; i < kMaxTasks; ++i) {
            if (m_pool[i].id == taskId) return &m_pool[i];
        }
        return NULL;
    }

    // ������ Debug / test API ���������������������������������������������������������������������������������������������������������������������������������������������������

    uint16_t TransportController::GetWaitingCount(FlagId flagId) const
    {
        if (flagId >= kMaxFlags) return 0;
        uint16_t count = 0;
        TransportTask* t = m_waitingHead[flagId];
        while (t) {
            count++;
            t = t->nextWaiting;
        }
        return count;
    }

    TransportTask* TransportController::PeekWaitingTask(FlagId flagId) const
    {
        if (flagId >= kMaxFlags) return NULL;
        return m_waitingHead[flagId];
    }

    uint16_t TransportController::GetBlockedCount() const
    {
        uint16_t count = 0;
        for (int i = 0; i < kMaxTasks; ++i) {
            if (m_pool[i].state == TTS_Blocked) count++;
        }
        return count;
    }
    void TransportController::Update(float /*deltaTime*/)
    {
        m_currentTick++;
    }

    // ������ PR 3.2 ��� Carrier pool dispatching ������������������������������������������������������������

    TransportNode* TransportController::FindNodeForFlag(WorldModel& world, FlagId flag) const
    {
        if (flag < kNodeDemandFlagBase) return NULL;
        int idx = (int)(flag - kNodeDemandFlagBase);
        if (idx >= world.transportNodeCount) return NULL;
        return &world.transportNodes[idx];
    }

    void TransportController::Tick(WorldModel& world)
    {
        // 1. Process pending requests into tasks (same as Simulation::ProcessTransportRequests)
        int writeIdx = 0;
        for (int i = 0; i < world.pendingRequestCount; ++i) {
            TransportRequest& req = world.pendingRequests[i];
            if (req.fulfilled) continue;

            TransportTask* task = CreateTask(req.resource, req.origin, req.destination, req.reason);
            if (task != NULL) {
                req.fulfilled = true;
                if (req.demandIndex != kNoDemand) {
                    task->observerTicketId = static_cast<uint32_t>(req.demandIndex) + 1;
                    m_demand.OnTaskCreated(req.demandIndex, task->id);
                }
            }

            if (!req.fulfilled) {
                if (writeIdx != i) {
                    world.pendingRequests[writeIdx] = req;
                }
                writeIdx++;
            }
        }
        world.pendingRequestCount = writeIdx;

        // 2. Assign idle carriers to waiting tasks
        while (TryAssignWaitingTask() > 0) {}

        // 3. Execute carrier state machine (PR 3.3)
        for (int i = 0; i < kMaxCarriers; ++i) {
            TransportCarrier& c = m_carriers[i];

            // TCS_Assigned → try pickup from source node buffer
            // PR 3.4: pickup only if both source AND destination nodes exist
            if (c.state == TCS_Assigned) {
                TransportTask* task = GetTaskById(c.taskId);
                if (task && task->state == TTS_Assigned) {
                    FlagId srcFlag = task->route.flags[0];
                    FlagId dstFlag = task->route.flags[task->route.count - 1];
                    TransportNode* src = FindNodeForFlag(world, srcFlag);
                    TransportNode* dst = FindNodeForFlag(world, dstFlag);
                    if (src && dst && src->GetBufferAmount(task->resource) >= 1) {
                        src->buffer.Remove(task->resource, 1);
                        c.cargoType = task->resource;
                        c.cargoAmount = 1;
                        c.state = TCS_Pickup;
                        SetTaskState(task, TTS_Moving);
                    }
                }
            }

            // TCS_Pickup → TCS_Travelling (immediate transition)
            if (c.state == TCS_Pickup) {
                c.state = TCS_Travelling;
            }

            // TCS_Travelling → advance ONE hop per tick (PR 3.5)
            if (c.state == TCS_Travelling) {
                TransportTask* task = GetTaskById(c.taskId);
                if (!task) { c.state = TCS_Idle; continue; }

                if (IsLastHop(task)) {
                    // At the last hop → deliver
                    c.state = TCS_Delivering;
                } else {
                    // Advance one hop toward destination
                    task->hopIndex++;
                    task->targetFlag = task->route.flags[task->hopIndex + 1];
                }
            }

            // TCS_Delivering → deliver cargo to destination node
            // PR 3.4: dest guaranteed non-NULL (validated at pickup) — assert safety
            // PR 3.6: demand completion before FreeTask clears observerTicketId
            if (c.state == TCS_Delivering) {
                TransportTask* task = GetTaskById(c.taskId);
                if (!task) { c.state = TCS_Idle; continue; }

                FlagId destFlag = task->route.flags[task->route.count - 1];
                TransportNode* dest = FindNodeForFlag(world, destFlag);
                assert(dest != NULL);
                dest->ReceiveCargo(c.cargoType, c.cargoAmount);

                if (task->observerTicketId > 0) {
                    m_demand.CompleteDemand(task->observerTicketId);
                }

                if (m_recentDeliveryCount < kMaxRecentDeliveries) {
                    m_recentDeliveries[m_recentDeliveryCount].resource = task->resource;
                    m_recentDeliveries[m_recentDeliveryCount].destinationFlag = destFlag;
                    m_recentDeliveries[m_recentDeliveryCount].reason = task->reason;
                    m_recentDeliveryCount++;
                }

                SetTaskState(task, TTS_Delivered);
                FreeTask(task);
            }
        }

        // 4. Assign freed carriers to remaining waiting tasks
        while (TryAssignWaitingTask() > 0) {}

        m_currentTick++;
    }

    void TransportController::ReleaseCarrierForTask(TransportTaskId taskId)
    {
        // 1. Release TransportCarrier pool slot (new Tick() path)
        for (int i = 0; i < kMaxCarriers; ++i) {
            if (m_carriers[i].taskId == taskId) {
                m_carriers[i].state = TCS_Idle;
                m_carriers[i].taskId = 0;
                m_carriers[i].cargoType = ResourceType_None;
                m_carriers[i].cargoAmount = 0;
                break;
            }
        }

        // 2. Release legacy Phase 7 Carrier if present (PR 3.7 — single cleanup point)
        TransportTask* task = GetTaskById(taskId);
        if (task && task->carrier) {
            Carrier* c = static_cast<Carrier*>(task->carrier);
            c->m_phase7Task = NULL;
            c->m_phase7TargetFlag = 0;
            c->m_phase7Cargo = NULL;
            c->m_cargo = NULL;
            task->carrier = NULL;
        }
    }

    int TransportController::TryAssignWaitingTask()
    {
        int carrierIdx = -1;
        for (int i = 0; i < kMaxCarriers; ++i) {
            if (m_carriers[i].state == TCS_Idle) {
                carrierIdx = i;
                break;
            }
        }
        if (carrierIdx < 0) return 0;

        TransportTask* best = NULL;
        FlagId bestFlag = 0;
        for (uint32_t f = 0; f < kMaxFlags; ++f) {
            if (m_waitingHead[f] != NULL) {
                best = PickNextTask(f);
                if (best != NULL) {
                    bestFlag = f;
                    break;
                }
            }
        }
        if (!best) return 0;

        RemoveFromQueue(best);
        SetTaskState(best, TTS_Assigned);
        best->targetFlag = best->route.flags[best->hopIndex + 1];

        m_carriers[carrierIdx].state = TCS_Assigned;
        m_carriers[carrierIdx].taskId = best->id;

        return 1;
    }

    int TransportController::GetIdleCarrierCount() const
    {
        int count = 0;
        for (int i = 0; i < kMaxCarriers; ++i) {
            if (m_carriers[i].state == TCS_Idle) count++;
        }
        return count;
    }

    int TransportController::GetBusyCarrierCount() const
    {
        int count = 0;
        for (int i = 0; i < kMaxCarriers; ++i) {
            if (m_carriers[i].state != TCS_Idle) count++;
        }
        return count;
    }

} // namespace World
