#pragma once
#include "ResourceNode.h"
#include "Flag.h"
#include "TransportJob.h"
#include "Road.h"
#include "../Core/Vector2i.h"

namespace World {

    enum CarrierState {
        WalkingToPost,   // walking from warehouse to assigned road
        Working,         // normal road walking (has job or idle)
        ReturningHome    // road removed, walking back to warehouse
    };

    class Carrier {
    public:
        Road* road;
        float ep;              // absolute position: 0 = road->a, pathLen = road->b
        float walkDir;         // 1.0f = moving a->b, -1.0f = moving b->a
        bool cargoDelivered;   // true after dropping cargo, waiting for CarrierManager
        bool hasPickedUp;      // true after CommitPickup, prevents double-pickup
        Cargo cargo;
        TransportJob* job;

        CarrierState state;
        std::vector<Vector2i> transitTiles;  // tile path for WalkingToPost/ReturningHome
        float transitProgress;               // current position along transitTiles
        bool readyToRemove;                  // ReturningHome completed, ready for cleanup

        // Cached resolved Flag* pointers — set by CarrierManager before each Update/operation.
        // Transient: refreshed every frame, not used for persistent storage.
        Flag* m_resolvedSourceFlag;
        Flag* m_resolvedDestFlag;
        Flag* m_resolvedLegFrom;
        Flag* m_resolvedLegTo;
        Flag* m_roadEndpointA;
        Flag* m_roadEndpointB;

        Carrier(Road* r)
            : road(r), ep(0.0f), walkDir(1.0f), cargoDelivered(false), hasPickedUp(false), job(NULL),
              state(Working), transitProgress(0.0f), readyToRemove(false),
              m_resolvedSourceFlag(NULL), m_resolvedDestFlag(NULL),
              m_resolvedLegFrom(NULL), m_resolvedLegTo(NULL),
              m_roadEndpointA(NULL), m_roadEndpointB(NULL)
        {
            cargo.type = ResourceType_None;
            cargo.amount = 0;
            cargo.destFlagId = 0;
        }

        float GetPathLen() const {
            if (!road || road->tiles.size() < 2) return 0.0f;
            return (float)(road->tiles.size() - 1);
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
            transitTiles = tiles;
            transitProgress = 0.0f;
            state = WalkingToPost;
        }

        void SetupReturningHome(const std::vector<Vector2i>& tiles) {
            if (tiles.size() < 2) { readyToRemove = true; return; }
            transitTiles = tiles;
            transitProgress = 0.0f;
            state = ReturningHome;
            if (job) {
                job->assignedCarrier = Handle<Carrier>();
                job->state = TransportJob::Waiting;
                job = NULL;
            }
            cargoDelivered = false;
            hasPickedUp = false;
            cargo.type = ResourceType_None;
            cargo.amount = 0;
            cargo.destFlagId = 0;
        }

        bool AssignJob(TransportJob* j, Flag* fromFlag) {
            if (!j || !fromFlag || !road) return false;
            if (!m_roadEndpointA || !m_roadEndpointB) return false;
            if (!m_resolvedDestFlag) return false;

            int slotIdx = -1;
            for (int si = 0; si < 8; ++si) {
                ResourceSlot& slot = fromFlag->slots[si];
                if (slot.type == j->resource && slot.amount > 0 && slot.destFlagId == m_resolvedDestFlag->id) {
                    slotIdx = si;
                    break;
                }
            }
            if (slotIdx < 0) return false;

            ResourceSlot& slot = fromFlag->slots[slotIdx];
            cargo.type = slot.type;
            cargo.amount = 1;
            cargo.destFlagId = m_resolvedDestFlag->id;

            job = j;
            cargoDelivered = false;
            hasPickedUp = false;

            char buf[256];
            _snprintf(buf, sizeof(buf),
                "[Cargo] Job#%u ASSIGNED: from=Flag%u(%d,%d) dst=Flag%u(%d,%d) ep=%.1f road %u<->%u\n",
                j->id,
                fromFlag->id, fromFlag->pos.x, fromFlag->pos.y,
                m_resolvedDestFlag->id, m_resolvedDestFlag->pos.x, m_resolvedDestFlag->pos.y,
                ep,
                m_roadEndpointA ? m_roadEndpointA->id : 0,
                m_roadEndpointB ? m_roadEndpointB->id : 0);
            OutputDebugStringA(buf);

            return true;
        }

        void Update(float deltaTime) {
            if (state == WalkingToPost) {
                UpdateWalkingToPost(deltaTime);
                return;
            }
            if (state == ReturningHome) {
                UpdateReturningHome(deltaTime);
                return;
            }

            if (!road || road->tiles.size() < 2) return;
            
            if (road->state != Active) {
                readyToRemove = true;
                return;
            }

            float pathLen = GetPathLen();
            if (pathLen <= 0.0f) return;

            if (HasJob() && !cargoDelivered) {
                if (job->currentLeg + 1 >= job->route.size()) {
                    cargoDelivered = true;
                    return;
                }
                Flag* legFrom = m_resolvedLegFrom;
                Flag* legTo = m_resolvedLegTo;
                if (!legFrom || !legTo) {
                    readyToRemove = true;
                    return;
                }
                float pickupEp = GetFlagEp(legFrom);
                float destEp = GetFlagEp(legTo);

                if (!hasPickedUp) {
                    if (ep < pickupEp - 0.01f || ep > pickupEp + 0.01f) {
                        walkDir = (pickupEp >= ep) ? 1.0f : -1.0f;
                        ep += walkDir * deltaTime * 3.0f;
                        if ((walkDir > 0 && ep >= pickupEp) || (walkDir < 0 && ep <= pickupEp)) {
                            ep = pickupEp;
                        }
                    } else {
                        legFrom->CommitPickup(cargo.type, 1, cargo.destFlagId);
                        hasPickedUp = true;

                        char buf[256];
                        _snprintf(buf, sizeof(buf),
                            "[Cargo] Job#%u PICKED_UP: %s at flag %u(%d,%d) road %u<->%u\n",
                            job->id, ResourceTypeToString(cargo.type),
                            legFrom->id, legFrom->pos.x, legFrom->pos.y,
                            m_roadEndpointA ? m_roadEndpointA->id : 0,
                            m_roadEndpointB ? m_roadEndpointB->id : 0);
                        OutputDebugStringA(buf);
                    }
                } else {
                    walkDir = (destEp >= ep) ? 1.0f : -1.0f;
                    ep += walkDir * deltaTime * 3.0f;
                    if ((walkDir > 0 && ep >= destEp) || (walkDir < 0 && ep <= destEp)) {
                        ep = destEp;
                        legTo->AddResource(cargo.type, cargo.amount, cargo.destFlagId);

                        char buf[256];
                        _snprintf(buf, sizeof(buf),
                            "[Cargo] Job#%u DROPPED: %s at Flag%u(%d,%d) destFlagId=%u\n",
                            job->id, ResourceTypeToString(cargo.type),
                            legTo->id, legTo->pos.x, legTo->pos.y,
                            cargo.destFlagId);

                        cargo.type = ResourceType_None;
                        cargo.amount = 0;
                        cargo.destFlagId = 0;
                        cargoDelivered = true;
                    }
                }
            } else if (!HasJob()) {
                float centerEp = GetCenterEp();
                if (ep < centerEp - 0.1f || ep > centerEp + 0.1f) {
                    walkDir = (centerEp >= ep) ? 1.0f : -1.0f;
                    ep += walkDir * deltaTime * 3.0f;
                    if ((walkDir > 0 && ep >= centerEp) || (walkDir < 0 && ep <= centerEp)) {
                        ep = centerEp;
                    }
                }
            }
        }

        void UpdateWalkingToPost(float deltaTime) {
            if (transitTiles.size() < 2) { state = Working; return; }
            float pathLen = (float)(transitTiles.size() - 1);
            transitProgress += deltaTime * 3.0f;
            if (transitProgress >= pathLen) {
                transitProgress = pathLen;
                state = Working;
                if (m_roadEndpointA && m_roadEndpointB) {
                    const Vector2i& lastTile = transitTiles.back();
                    if (m_roadEndpointA->pos.x == lastTile.x && m_roadEndpointA->pos.y == lastTile.y) {
                        ep = 0.0f; walkDir = 1.0f;
                    } else {
                        ep = GetPathLen(); walkDir = -1.0f;
                    }
                }
            }
            walkDir = 1.0f;
        }

        void UpdateReturningHome(float deltaTime) {
            if (transitTiles.size() < 2) { readyToRemove = true; return; }
            float pathLen = (float)(transitTiles.size() - 1);
            transitProgress += deltaTime * 3.0f;
            if (transitProgress >= pathLen) {
                transitProgress = pathLen;
                readyToRemove = true;
            }
            walkDir = 1.0f;
        }

        bool HasArrived() const {
            return cargoDelivered;
        }

        bool HasJob() const {
            return job != NULL;
        }

        void ClearJob() {
            if (job) {
                job->assignedCarrier = Handle<Carrier>();
            }
            job = NULL;
            cargoDelivered = false;
            hasPickedUp = false;
            cargo.type = ResourceType_None;
            cargo.amount = 0;
            cargo.destFlagId = 0;
        }
    };

    typedef Handle<Carrier> CarrierHandle;
}
