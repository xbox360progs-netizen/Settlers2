#pragma once
#include "ResourceNode.h"
#include "Flag.h"
#include "FlagManager.h"
#include "Cargo.h"
#include "Road.h"
#include "RoadManager.h"
#include "Entity.h"
#include "../Core/Vector2i.h"
#include "TransportTypes.h"
#include "TransportTask.h"
#include "TransportController.h"

#define MAX_TRANSIT_TILES 4096

namespace World {
    class DemandManager;
    class CargoManager;
    class TransportController;
    struct TransportTask;

    enum MovementAuthority {
        Legacy,
        ECS
    };

    enum CarrierState {
        WalkingToPost,   // walking from warehouse to assigned road
        Working,         // normal road walking (shuttle: pick up/drop cargo at endpoints)
        ReturningHome    // road removed, walking back to warehouse
    };

    inline bool IsTransitState(CarrierState s) {
        return s == WalkingToPost || s == ReturningHome;
    }

    class Carrier {
    public:
        Road* road;
        float ep;              // absolute position: 0 = road endpoint A, pathLen = road endpoint B
        float walkDir;         // 1.0f = moving A->B, -1.0f = moving B->A
        Cargo* m_cargo;        // cargo being carried (NULL when empty)

        CarrierState state;
        Vector2i transitTiles[MAX_TRANSIT_TILES];  // tile path for WalkingToPost/ReturningHome
        uint32_t transitCount;
        float transitProgress;               // current position along transitTiles
        bool readyToRemove;                  // ReturningHome completed, ready for cleanup
        Entity ecsEntity;                    // ECS entity for this carrier
        MovementAuthority m_authority;       // Legacy or ECS movement ownership
        uint32_t pathVersion;                // incremented when path/tiles change

        // Manager pointers — set by CarrierManager before each Update
        DemandManager* m_demandManager;
        CargoManager* m_cargoManager;
        RoadManager* m_roadManager;

        // Cached resolved Flag* pointers — set by CarrierManager before each Update/operation.
        Flag* m_roadEndpointA;
        Flag* m_roadEndpointB;

        float m_idleCheckTimer;
        bool m_returningToCenter;

        // Phase 7 — task assignment (Controller owns all routing)
        TransportTask* m_phase7Task;
        FlagId m_phase7TargetFlag;
        Cargo* m_phase7Cargo;
        TransportController* m_phase7Controller;

        Carrier(Road* r)
            : road(r), ep(0.0f), walkDir(1.0f), m_cargo(NULL),
              state(Working), transitCount(0), transitProgress(0.0f), readyToRemove(false), ecsEntity(INVALID_ENTITY), m_authority(Legacy), pathVersion(0),
              m_demandManager(NULL), m_cargoManager(NULL), m_roadManager(NULL),
              m_roadEndpointA(NULL), m_roadEndpointB(NULL), m_idleCheckTimer(0.0f),
              m_returningToCenter(false),
              m_phase7Task(NULL), m_phase7TargetFlag(0), m_phase7Cargo(NULL), m_phase7Controller(NULL)
        {
        }

        float GetPathLen() const {
            if (!road || road->tileCount < 2) return 0.0f;
            return (float)(road->tileCount - 1);
        }

        float GetCenterEp() const {
            return GetPathLen() * 0.5f;
        }

        float GetFlagEp(Flag* f) const {
            if (!road || !f) return 0.0f;
            return (f == m_roadEndpointB) ? GetPathLen() : 0.0f;
        }

        void SetupWalkingToPost(const std::vector<Vector2i>& tiles) {
            if (tiles.size() < 2) { state = Working; return; }
            transitCount = (tiles.size() < MAX_TRANSIT_TILES) ? (uint32_t)tiles.size() : MAX_TRANSIT_TILES;
            for (uint32_t i = 0; i < transitCount; ++i) transitTiles[i] = tiles[i];
            transitProgress = 0.0f;
            state = WalkingToPost;
            ++pathVersion;
        }

        void SetupReturningHome(const std::vector<Vector2i>& tiles) {
            if (tiles.size() < 2) { readyToRemove = true; return; }
            transitCount = (tiles.size() < MAX_TRANSIT_TILES) ? (uint32_t)tiles.size() : MAX_TRANSIT_TILES;
            for (uint32_t i = 0; i < transitCount; ++i) transitTiles[i] = tiles[i];
            transitProgress = 0.0f;
            state = ReturningHome;
            ++pathVersion;
            m_cargo = NULL;
        }

        void SyncFromECS(float ep, float walkDir, CarrierState state) {
            this->ep = ep;
            this->walkDir = walkDir;
            this->state = state;
        }

        void SyncToECS(float& outEp, float& outWalkDir, CarrierState& outState) const {
            outEp = this->ep;
            outWalkDir = this->walkDir;
            outState = this->state;
        }

        // Phase 7 — Controller sets the task and immediate next flag.
        // Carrier does NOT interpret the route. It only walks toward targetFlag.
        void AssignPhase7Task(TransportTask* task, FlagId targetFlag) {
            m_phase7Task = task;
            m_phase7TargetFlag = targetFlag;
            assert(m_phase7Task != NULL);
            assert(m_phase7TargetFlag != 0);
        }

        bool IsCarrying() const { return m_cargo != NULL; }

        void Update(float deltaTime) {
            if (state == WalkingToPost) {
                if (m_authority == Legacy)
                    UpdateWalkingToPost(deltaTime);
                return;
            }
            if (state == ReturningHome) {
                if (m_authority == Legacy)
                    UpdateReturningHome(deltaTime);
                return;
            }

            // Phase 7 movement — walk toward targetFlag without touching TransportTask
            if (m_phase7Task && m_phase7Task->state == TTS_Moving) {
                if (!road || road->tileCount < 2) return;
                float pathLen = GetPathLen();
                if (pathLen <= 0.0f) return;

                // Determine target endpoint
                bool targetIsA = (m_roadEndpointA && m_roadEndpointA->id == m_phase7TargetFlag);
                bool targetIsB = (m_roadEndpointB && m_roadEndpointB->id == m_phase7TargetFlag);
                if (!targetIsA && !targetIsB) return;

                float targetEp = targetIsB ? pathLen : 0.0f;
                walkDir = (targetEp > ep) ? 1.0f : -1.0f;

                float step = 3.0f * deltaTime;
                if (step > 1.5f) step = 1.5f;
                float newEp = ep + walkDir * step;

                if ((walkDir > 0.0f && newEp >= targetEp) || (walkDir < 0.0f && newEp <= targetEp)) {
                    newEp = targetEp;
                    ep = newEp;

                    // Pre-arrival asserts: Carrier verifies its own state
                    assert(m_phase7Task != NULL);
                    assert(m_phase7Cargo != NULL);
                    assert(m_phase7TargetFlag == (targetIsB ? m_roadEndpointB->id : m_roadEndpointA->id));

                    // Notify controller — only Controller may transition task state
                    if (m_phase7Controller) {
                        m_phase7Controller->NotifyCarrierArrived(
                            this, m_phase7TargetFlag);
                    }
                } else {
                    ep = newEp;
                }

                // Carrier does NOT touch any TransportTask fields — spatial movement only
                return;
            }

            if (!road || road->tileCount < 2) return;

            if (road->state != Active) {
                readyToRemove = true;
                return;
            }

            float pathLen = GetPathLen();
            if (pathLen <= 0.0f) return;

                // Idle — standing at flag, check for work
            if (walkDir == 0.0f) {
                m_idleCheckTimer -= deltaTime;
                if (m_idleCheckTimer > 0.0f) return;
                m_idleCheckTimer = 0.25f;

                Flag* checkFlags[2] = { m_roadEndpointA, m_roadEndpointB };
                for (int fi = 0; fi < 2; ++fi) {
                    Flag* f = checkFlags[fi];
                    if (!f) continue;
                    bool hasWork = false;
                    // Check existing Cargo objects via pool scan
                    if (m_cargoManager) {
                        for (int ci = 0; ci < m_cargoManager->GetActiveCount() && !hasWork; ++ci) {
                            Cargo* c = m_cargoManager->GetCargoByActiveIdx(ci);
                            if (c->state != Cargo_OnFlag) continue;
                            if (c->currentFlag.index != f->handle.index || c->currentFlag.generation != f->handle.generation) continue;
                            if (!c->ownerTask) continue;            // Carrier never creates assignment
                            if (c->ownerTask->state == TTS_Delivered || c->ownerTask->state == TTS_Cancelled) continue;
                            if (c->ownerTask->targetFlag == f->id) continue;
                            if (m_roadManager && c->ownerTask->route.count > 0) {
                                FlagId destId = c->ownerTask->route.flags[c->ownerTask->route.count - 1];
                                Flag* dest = m_roadManager->GetFlagManager()->GetFlagById(destId);
                                if (dest) {
                                    Flag* nextHop = m_roadManager->GetNextHop(f, dest);
                                    if (nextHop) {
                                        FlagHandle otherEnd = (road->a == f->handle) ? road->b : road->a;
                                        hasWork = (otherEnd == nextHop->handle);
                                    }
                                }
                            }
                        }
                    }
                            // Check cargo without ownerTask (producer placed without demand knowledge)
                            if (!hasWork && m_demandManager && m_cargoManager) {
                                for (int ci = 0; ci < m_cargoManager->GetActiveCount() && !hasWork; ++ci) {
                                    Cargo* c = m_cargoManager->GetCargoByActiveIdx(ci);
                                    if (c->state != Cargo_OnFlag) continue;
                                    if (c->currentFlag.index != f->handle.index || c->currentFlag.generation != f->handle.generation) continue;
                                    if (c->ownerTask) continue;
                                    hasWork = m_demandManager->HasDemand(c->type);
                                }
                            }
                    // Check ResourceSlots (warehouse/production outputs)
                    // Wake-up only: if there's any demand for this resource type, the Carrier
                    // walks to the flag. The actual destination is assigned by DemandManager::Reserve()
                    // at pickup time — Carrier never computes a route or chooses a target.
                    if (!hasWork && m_demandManager) {
                        for (int si = 0; si < 8 && !hasWork; ++si) {
                            ResourceSlot& slot = f->slots[si];
                            if (slot.type == ResourceType_None || slot.amount <= 0) continue;
                            bool hasDemand = m_demandManager->HasDemandFromOtherFlag(slot.type, f->handle);
                            { char dbg[256]; _snprintf(dbg, sizeof(dbg), "[CARRIER IDLE] flag=%u slot[%d] type=%s amt=%d hasOtherDemand=%d hasWork=%d\n", f->id, si, ResourceTypeToString(slot.type), slot.amount, (int)hasDemand, (int)(hasWork || hasDemand)); OutputDebugStringA(dbg); }
                            hasWork = hasDemand;
                        }
                    }
                    if (hasWork) {
                        walkDir = (f == m_roadEndpointB) ? 1.0f : -1.0f;
                        m_returningToCenter = false;
                        { char dbg[256]; _snprintf(dbg, sizeof(dbg), "[Carrier] Wake road=%u toward=%u\n", road->id, f->id); OutputDebugStringA(dbg); }
                        break;
                    }
                }
                if (walkDir == 0.0f) return; // still idle
            }

            float step = 3.0f * deltaTime;
            if (step > 1.5f) step = 1.5f; // clamp to avoid teleport on large delta
            float newEp = ep + walkDir * step;

            // Check if we reached an endpoint
            if ((walkDir > 0.0f && newEp >= pathLen) || (walkDir < 0.0f && newEp <= 0.0f)) {
                newEp = (walkDir > 0.0f) ? pathLen : 0.0f;

                Flag* atFlag = (walkDir > 0.0f) ? m_roadEndpointB : m_roadEndpointA;

                if (atFlag) {
                    // Drop cargo if carrying (check per-flag capacity via pool)
                    if (m_cargo) {
                        if (m_cargoManager && m_cargoManager->CountCargoOnFlag(atFlag->handle) < FLAG_MAX_CARGO) {
                            { char dbg[256]; _snprintf(dbg, sizeof(dbg), "[Carrier] Drop cargo id=%u type=%s flag=%u ownerTask=%p taskState=%u\n", m_cargo->id, ResourceTypeToString(m_cargo->type), atFlag->id, m_cargo->ownerTask, m_cargo->ownerTask ? m_cargo->ownerTask->state : 0); OutputDebugStringA(dbg); }
                            atFlag->AcceptCargo(m_cargo);
                            m_cargo = NULL;
                        }
                    }

                    // Try to pick up cargo from this flag (or convert ResourceSlot)
                    if (!m_cargo) {
                        Cargo* available = NULL;
                        // First pass: look for existing Cargo that routes through this road
                        if (m_cargoManager) {
                            for (int ci = 0; ci < m_cargoManager->GetActiveCount() && !available; ++ci) {
                                Cargo* c = m_cargoManager->GetCargoByActiveIdx(ci);
                                if (c->state != Cargo_OnFlag) continue;
                                if (c->currentFlag.index != atFlag->handle.index || c->currentFlag.generation != atFlag->handle.generation) continue;
                                if (!c->ownerTask) continue;            // Carrier never creates assignment
                                if (c->ownerTask->state == TTS_Delivered || c->ownerTask->state == TTS_Cancelled) continue;
                                if (c->ownerTask->targetFlag == atFlag->id) continue;
                                // Route check using task's route
                                bool routeOk = false;
                                if (m_roadManager && c->ownerTask->route.count > 0) {
                                    FlagId destId = c->ownerTask->route.flags[c->ownerTask->route.count - 1];
                                    Flag* dest = m_roadManager->GetFlagManager()->GetFlagById(destId);
                                    if (dest) {
                                        Flag* nextHop = m_roadManager->GetNextHop(atFlag, dest);
                                        if (nextHop) {
                                            FlagHandle otherEnd = (road->a == atFlag->handle) ? road->b : road->a;
                                            routeOk = (otherEnd == nextHop->handle);
                                        }
                                    }
                                }
                                {
                                    char dbg[256];
                                    _snprintf(dbg, sizeof(dbg), "[ROUTE %s] road=%u atFlag=%u cargo=%u\n",
                                        routeOk ? "PICKUP" : "SKIP",
                                        road->id, atFlag->id, c->id);
                                    OutputDebugStringA(dbg);
                                }
                                if (!routeOk)
                                    continue;
                                available = c;
                                break;
                            }
                        }
                        // Second pass: convert ResourceSlot only (with route check)
                        if (!available) {
                            available = atFlag->TakeCargoForRoad(road, m_demandManager, m_cargoManager, m_phase7Controller);
                            if (available) {
                                if (available->ownerTask) {
                                    // Phase 8.2 — route validity check via task route
                                    bool routeValid = false;
                                    if (m_roadManager && available->ownerTask->route.count > 0) {
                                        FlagId destId = available->ownerTask->route.flags[available->ownerTask->route.count - 1];
                                        Flag* dest = m_roadManager->GetFlagManager()->GetFlagById(destId);
                                        if (dest) {
                                            Flag* nextHop = m_roadManager->GetNextHop(atFlag, dest);
                                            if (nextHop) {
                                                FlagHandle otherEnd = (road->a == atFlag->handle) ? road->b : road->a;
                                                routeValid = (otherEnd == nextHop->handle);
                                            }
                                        }
                                    }
                                    if (!routeValid) {
                                        ResourceType rejectedType = available->type;
                                        { char dbg[256]; _snprintf(dbg, sizeof(dbg), "[Carrier] Reject slot road=%u atFlag=%u type=%s (no route) controller=%p\n", road->id, atFlag->id, ResourceTypeToString(rejectedType), m_phase7Controller); OutputDebugStringA(dbg); }
                                        if (m_phase7Controller) {
                                            m_phase7Controller->CancelTask(available->ownerTask->id);
                                        }
                                        available->ownerTask = NULL;
                                        if (!atFlag->AddResource(rejectedType, 1)) {
                                            available->state = Cargo_OnFlag;
                                            available->currentFlag = atFlag->handle;
                                        } else {
                                            m_cargoManager->Release(available->id);
                                        }
                                        available = NULL;
                                    }
                                } else {
                                    // No task — reject cargo (legacy ticket path removed in Phase 8.2)
                                    ResourceType rejectedType = available->type;
                                    { char dbg[256]; _snprintf(dbg, sizeof(dbg), "[Carrier] Reject slot road=%u atFlag=%u type=%s (no task)\n", road->id, atFlag->id, ResourceTypeToString(rejectedType)); OutputDebugStringA(dbg); }
                                    if (!atFlag->AddResource(rejectedType, 1)) {
                                        available->state = Cargo_OnFlag;
                                        available->currentFlag = atFlag->handle;
                                    } else {
                                        m_cargoManager->Release(available->id);
                                    }
                                    available = NULL;
                                }
                            }
                        }
                        if (available) {
                            available->state = Cargo_Carried;
                            available->currentFlag = FlagHandle();
                            m_cargo = available;
                        }
                    }
                }

                // If we reached a flag and have nothing to carry, walk toward center
                if (!m_cargo) {
                    float center = pathLen * 0.5f;
                    walkDir = (newEp > center) ? -1.0f : 1.0f;
                    m_returningToCenter = true;
                    { char dbg[256]; _snprintf(dbg, sizeof(dbg), "[Carrier] Walk to center road=%u ep=%.1f center=%.1f\n", road->id, newEp, center); OutputDebugStringA(dbg); }
                } else {
                    walkDir = -walkDir;
                }
            }

            // Clamp at center when walking to idle position (only if returning from endpoint)
            if (m_returningToCenter) {
                float center = pathLen * 0.5f;
                if ((walkDir > 0.0f && newEp >= center) || (walkDir < 0.0f && newEp <= center)) {
                    newEp = center;
                    walkDir = 0.0f;
                    m_returningToCenter = false;
                    { char dbg[256]; _snprintf(dbg, sizeof(dbg), "[Carrier] Idle road=%u at=%.1f\n", road->id, newEp); OutputDebugStringA(dbg); }
                }
            }

            ep = newEp;
        }

        void UpdateWalkingToPost(float deltaTime) {
            if (transitCount < 2) { state = Working; return; }
            float pathLen = (float)(transitCount - 1);
            transitProgress += deltaTime * 3.0f;
            if (transitProgress >= pathLen) {
                transitProgress = pathLen;
                state = Working;
                // Arrive at correct endpoint — check which flag the transit path ends at
                float roadPathLen = GetPathLen();
                const Vector2i& lastTile = transitTiles[transitCount - 1];
                bool atEndB = (m_roadEndpointB && m_roadEndpointB->pos.x == lastTile.x && m_roadEndpointB->pos.y == lastTile.y);
                ep = atEndB ? roadPathLen : 0.0f;
                walkDir = atEndB ? -1.0f : 1.0f;
                m_returningToCenter = true;
                { char dbg[256]; _snprintf(dbg, sizeof(dbg), "[Carrier] Transit done road=%u arrived=%s ep=%.1f\n", road->id, atEndB ? "B" : "A", ep); OutputDebugStringA(dbg); }
            }
        }

        void UpdateReturningHome(float deltaTime) {
            if (transitCount < 2) { readyToRemove = true; return; }
            float pathLen = (float)(transitCount - 1);
            transitProgress += deltaTime * 3.0f;
            if (transitProgress >= pathLen) {
                transitProgress = pathLen;
                readyToRemove = true;
            }
            walkDir = 1.0f;
        }
    };

    typedef Handle<Carrier> CarrierHandle;
}
