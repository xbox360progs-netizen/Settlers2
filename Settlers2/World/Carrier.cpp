#include "stdafx.h"
#include "Carrier.h"
#include "CarrierManager.h"
#include "CargoManager.h"
#include "DemandManager.h"
#include "RoadManager.h"
#include "Flag.h"
#include "FlagManager.h"
#include "ResourceNode.h"
#include "../SimulationCore/Transport/TransportController.h"

#include <cstdio>

namespace World {

    float Carrier::GetPathLen() const {
        if (!road || road->tileCount < 2) return 0.0f;
        return (float)(road->tileCount - 1);
    }

    float Carrier::GetCenterEp() const {
        return GetPathLen() * 0.5f;
    }

    float Carrier::GetFlagEp(Flag* f) const {
        if (!road || !f) return 0.0f;
        return (f == m_roadEndpointB) ? GetPathLen() : 0.0f;
    }

    void Carrier::Update(float deltaTime) {
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

                if (m_phase7Controller) {
                    m_phase7Controller->NotifyCarrierArrived(this, m_phase7TargetFlag);
                }
            } else {
                ep = newEp;
            }
            return;
        }

        if (!road || road->tileCount < 2) return;

        if (road->state != Active) {
            readyToRemove = true;
            return;
        }

        float pathLen = GetPathLen();
        if (pathLen <= 0.0f) return;

        if (walkDir == 0.0f) {
            m_idleCheckTimer -= deltaTime;
            if (m_idleCheckTimer > 0.0f) return;
            m_idleCheckTimer = 0.25f;

            Flag* checkFlags[2] = { m_roadEndpointA, m_roadEndpointB };
            for (int fi = 0; fi < 2; ++fi) {
                Flag* f = checkFlags[fi];
                if (!f) continue;
                bool hasWork = false;
                if (m_cargoManager) {
                    for (int ci = 0; ci < m_cargoManager->GetActiveCount() && !hasWork; ++ci) {
                        Cargo* c = m_cargoManager->GetCargoByActiveIdx(ci);
                        if (c->state != Cargo_OnFlag) continue;
                        if (c->currentFlag.index != f->handle.index || c->currentFlag.generation != f->handle.generation) continue;
                        if (!c->ownerTask) continue;
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
                if (!hasWork && m_demandManager && m_cargoManager) {
                    for (int ci = 0; ci < m_cargoManager->GetActiveCount() && !hasWork; ++ci) {
                        Cargo* c = m_cargoManager->GetCargoByActiveIdx(ci);
                        if (c->state != Cargo_OnFlag) continue;
                        if (c->currentFlag.index != f->handle.index || c->currentFlag.generation != f->handle.generation) continue;
                        if (c->ownerTask) continue;
                        hasWork = m_demandManager->HasDemand(c->type);
                    }
                }
                if (!hasWork && m_demandManager) {
                    for (int si = 0; si < 8 && !hasWork; ++si) {
                        ResourceSlot& slot = f->slots[si];
                        if (slot.type == ResourceType_None || slot.amount <= 0) continue;
                        hasWork = m_demandManager->HasDemandFromOtherFlag(slot.type, f->handle);
                    }
                }
                if (hasWork) {
                    walkDir = (f == m_roadEndpointB) ? 1.0f : -1.0f;
                    m_returningToCenter = false;
                    break;
                }
            }
            if (walkDir == 0.0f) return;
        }

        float step = 3.0f * deltaTime;
        if (step > 1.5f) step = 1.5f;
        float newEp = ep + walkDir * step;

        if ((walkDir > 0.0f && newEp >= pathLen) || (walkDir < 0.0f && newEp <= 0.0f)) {
            newEp = (walkDir > 0.0f) ? pathLen : 0.0f;

            Flag* atFlag = (walkDir > 0.0f) ? m_roadEndpointB : m_roadEndpointA;

            if (atFlag) {
                if (m_cargo) {
                    if (m_cargoManager && m_cargoManager->CountCargoOnFlag(atFlag->handle) < FLAG_MAX_CARGO) {
                        atFlag->AcceptCargo(m_cargo);
                        m_cargo = NULL;
                    }
                }

                if (!m_cargo) {
                    Cargo* available = NULL;
                    if (m_cargoManager) {
                        for (int ci = 0; ci < m_cargoManager->GetActiveCount() && !available; ++ci) {
                            Cargo* c = m_cargoManager->GetCargoByActiveIdx(ci);
                            if (c->state != Cargo_OnFlag) continue;
                            if (c->currentFlag.index != atFlag->handle.index || c->currentFlag.generation != atFlag->handle.generation) continue;
                            if (!c->ownerTask) continue;
                            if (c->ownerTask->state == TTS_Delivered || c->ownerTask->state == TTS_Cancelled) continue;
                            if (c->ownerTask->targetFlag == atFlag->id) continue;
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
                            if (!routeOk) continue;
                            available = c;
                            break;
                        }
                    }
                    if (!available) {
                        available = atFlag->TakeCargoForRoad(road, m_demandManager, m_cargoManager, m_phase7Controller);
                        if (available) {
                            if (available->ownerTask) {
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
                                ResourceType rejectedType = available->type;
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

            if (!m_cargo) {
                float center = pathLen * 0.5f;
                walkDir = (newEp > center) ? -1.0f : 1.0f;
                m_returningToCenter = true;
            } else {
                walkDir = -walkDir;
            }
        }

        if (m_returningToCenter) {
            float center = pathLen * 0.5f;
            if ((walkDir > 0.0f && newEp >= center) || (walkDir < 0.0f && newEp <= center)) {
                newEp = center;
                walkDir = 0.0f;
                m_returningToCenter = false;
            }
        }

        ep = newEp;
    }

    void Carrier::UpdateWalkingToPost(float deltaTime) {
        if (transitCount < 2) { state = Working; return; }
        float pathLen = (float)(transitCount - 1);
        transitProgress += deltaTime * 3.0f;
        if (transitProgress >= pathLen) {
            transitProgress = pathLen;
            state = Working;
            float roadPathLen = GetPathLen();
            const Vector2i& lastTile = transitTiles[transitCount - 1];
            bool atEndB = (m_roadEndpointB && m_roadEndpointB->pos.x == lastTile.x && m_roadEndpointB->pos.y == lastTile.y);
            ep = atEndB ? roadPathLen : 0.0f;
            walkDir = atEndB ? -1.0f : 1.0f;
            m_returningToCenter = true;
        }
    }

} // namespace World
