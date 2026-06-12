#include "stdafx.h"
#include "CarrierManager.h"
#include "FlagManager.h"
#include "TransportJobManager.h"
#include "Flag.h"
#include "Road.h"
#include "RoadManager.h"

namespace World {

    // Helper: restore in-flight cargo to source flag before destroying carrier
    static void RestoreCarrierCargo(Carrier* c) {
        if (!c || !c->hasPickedUp || c->cargoDelivered || !c->job) return;
        // sourceFlag resolved by carrier manager before calling this
        Flag* srcFlag = NULL;
        if (c->m_resolvedSourceFlag) {
            srcFlag = c->m_resolvedSourceFlag;
        }
        if (srcFlag && c->cargo.type != ResourceType_None && c->cargo.amount > 0) {
            srcFlag->AddResource(c->cargo.type, c->cargo.amount, c->cargo.destFlagId);
            char buf[256];
            _snprintf(buf, sizeof(buf),
                "[Carrier] RESTORE cargo: %s x%d -> flag %u(%d,%d)\n",
                ResourceTypeToString(c->cargo.type), c->cargo.amount,
                srcFlag->id, srcFlag->pos.x, srcFlag->pos.y);
            OutputDebugStringA(buf);
        }
    }

    // Helper: disconnect job from carrier and set to Waiting for reassignment
    static void ReleaseCarrierJob(Carrier* c) {
        if (c->job) {
            c->job->assignedCarrier = Handle<Carrier>();
            c->job->state = TransportJob::Waiting;
        }
    }

    // Handle API
    CarrierHandle CarrierManager::RegisterCarrier(Carrier* c) {
        return m_carrierRegistry.Register<Carrier>(c);
    }

    Carrier* CarrierManager::ResolveCarrier(CarrierHandle h) const {
        return m_carrierRegistry.Resolve<Carrier>(h);
    }

    CarrierHandle CarrierManager::GetCarrierHandle(Carrier* c) const {
        return m_carrierRegistry.FindHandle<Carrier>(c);
    }

    void CarrierManager::UnregisterCarrier(CarrierHandle h) {
        m_carrierRegistry.Unregister<Carrier>(h);
    }

    CarrierManager::CarrierManager()
        : m_flagManager(NULL), m_jobManager(NULL), m_roadManager(NULL), m_warehouseFlag(NULL)
    {
    }

    std::vector<Vector2i> CarrierManager::BuildTransitPath(Flag* fromFlag, Flag* toFlag) {
        std::vector<Vector2i> result;
        if (!fromFlag || !toFlag || !m_roadManager) return result;

        std::vector<Flag*> flagPath = m_roadManager->FindFlagPath(fromFlag, toFlag);
        if (flagPath.size() < 2) return result;

        for (size_t i = 0; i + 1 < flagPath.size(); ++i) {
            Road* road = m_roadManager->GetRoadBetween(flagPath[i], flagPath[i + 1]);
            if (!road || road->tiles.size() < 2) continue;

            if (i == 0) {
                for (size_t t = 0; t < road->tiles.size(); ++t)
                    result.push_back(road->tiles[t]);
            } else {
                for (size_t t = 1; t < road->tiles.size(); ++t)
                    result.push_back(road->tiles[t]);
            }
        }
        return result;
    }

    void CarrierManager::CreateCarrier(Road* road) {
        if (!road) return;
        for (size_t i = 0; i < m_carriers.size(); ++i) {
            if (m_carriers[i]->road == road)
                return;
        }

        Flag* ra = m_flagManager ? m_flagManager->ResolveFlag(road->a) : NULL;
        Flag* rb = m_flagManager ? m_flagManager->ResolveFlag(road->b) : NULL;

        // Don't create carriers for roads not connected to the warehouse
        if (m_warehouseFlag && m_roadManager) {
            std::vector<Flag*> pathA = m_roadManager->FindFlagPath(m_warehouseFlag, ra);
            std::vector<Flag*> pathB = m_roadManager->FindFlagPath(m_warehouseFlag, rb);
            if (pathA.size() < 2 && pathB.size() < 2) {
                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[Carrier] CreateCarrier road %u: wh=%u a=%u(pathA=%u) b=%u(pathB=%u) isolated — no carrier\n",
                    road->id, m_warehouseFlag->id,
                    ra ? ra->id : 0, (unsigned)pathA.size(),
                    rb ? rb->id : 0, (unsigned)pathB.size());
                OutputDebugStringA(buf);
                return;
            }
        }

        Carrier* c = new Carrier(road);
        c->m_roadEndpointA = ra;
        c->m_roadEndpointB = rb;
        road->carrier = RegisterCarrier(c);
        m_carriers.push_back(c);

        // Walk from warehouse to nearest road endpoint
        if (m_warehouseFlag && m_roadManager) {
            Flag* targetFlag = NULL;
            if (!ra || !rb) {
                UnregisterCarrier(road->carrier);
                road->carrier = Handle<Carrier>();
                delete c;
                m_carriers.pop_back();
                return;
            }
            std::vector<Flag*> pathA = m_roadManager->FindFlagPath(m_warehouseFlag, ra);
            std::vector<Flag*> pathB = m_roadManager->FindFlagPath(m_warehouseFlag, rb);
            char buf[256];
            _snprintf(buf, sizeof(buf), "[Carrier] CreateCarrier road %u: wh=%u a=%u(pathA=%u) b=%u(pathB=%u)\n",
                road->id, m_warehouseFlag->id,
                ra->id, (unsigned)pathA.size(),
                rb->id, (unsigned)pathB.size());
            OutputDebugStringA(buf);
            if (pathA.size() >= 2 && (pathB.size() < 2 || pathA.size() <= pathB.size())) {
                targetFlag = ra;
            } else if (pathB.size() >= 2) {
                targetFlag = rb;
            }
            if (targetFlag) {
                std::vector<Vector2i> transitPath = BuildTransitPath(m_warehouseFlag, targetFlag);
                _snprintf(buf, sizeof(buf), "[Carrier] CreateCarrier transit: wh=%u->flag%u tiles=%u\n",
                    m_warehouseFlag->id, targetFlag->id, (unsigned)transitPath.size());
                OutputDebugStringA(buf);
                c->SetupWalkingToPost(transitPath);
            }
        }
    }

    void CarrierManager::RemoveCarrier(Flag* a, Flag* b) {
        for (size_t i = 0; i < m_carriers.size(); ++i) {
            Carrier* c = m_carriers[i];
            if (!c->road) continue;
            Flag* ra = m_flagManager ? m_flagManager->ResolveFlag(c->road->a) : NULL;
            Flag* rb = m_flagManager ? m_flagManager->ResolveFlag(c->road->b) : NULL;
            if ((ra == a && rb == b) || (ra == b && rb == a)) {
                if (c->job && m_flagManager) c->m_resolvedSourceFlag = m_flagManager->ResolveFlag(c->job->sourceFlag);
                RestoreCarrierCargo(c);
                ReleaseCarrierJob(c);
                if (c->road) {
                    UnregisterCarrier(c->road->carrier);
                    c->road->carrier = Handle<Carrier>();
                }
                delete c;
                m_carriers.erase(m_carriers.begin() + i);
                return;
            }
        }
    }

    void CarrierManager::RemoveCarriersForFlag(Flag* f) {
        for (int i = (int)m_carriers.size() - 1; i >= 0; --i) {
            Carrier* c = m_carriers[i];
            if (!c->road) continue;
            Flag* ra = m_flagManager ? m_flagManager->ResolveFlag(c->road->a) : NULL;
            Flag* rb = m_flagManager ? m_flagManager->ResolveFlag(c->road->b) : NULL;
            if (ra == f || rb == f) {
                if (c->state == WalkingToPost) {
                    if (c->road) {
                        UnregisterCarrier(c->road->carrier);
                        c->road->carrier = Handle<Carrier>();
                    }
                    delete c;
                    m_carriers.erase(m_carriers.begin() + i);
                    continue;
                }
                if (c->job && m_flagManager) c->m_resolvedSourceFlag = m_flagManager->ResolveFlag(c->job->sourceFlag);
                RestoreCarrierCargo(c);
                ReleaseCarrierJob(c);
                if (c->road) c->road->carrier = Handle<Carrier>();
                Road* oldRoad = c->road;
                c->road = NULL;

                // Walk back to warehouse
                if (m_warehouseFlag && m_roadManager) {
                    float pathLen = (float)(oldRoad->tiles.size() - 1);
                    Flag* nearestFlag = (c->ep <= pathLen * 0.5f) ? ra : rb;
                    std::vector<Vector2i> returnPath = BuildTransitPath(nearestFlag, m_warehouseFlag);

                    std::vector<Vector2i> fullPath;
                    if (nearestFlag == ra) {
                        for (int t = (int)c->ep; t >= 0; --t)
                            fullPath.push_back(oldRoad->tiles[t]);
                    } else {
                        for (size_t t = (size_t)c->ep; t < oldRoad->tiles.size(); ++t)
                            fullPath.push_back(oldRoad->tiles[t]);
                    }
                    if (!fullPath.empty() && !returnPath.empty())
                        fullPath.pop_back();
                    fullPath.insert(fullPath.end(), returnPath.begin(), returnPath.end());
                    c->SetupReturningHome(fullPath);
                } else {
                    c->SetupReturningHome(std::vector<Vector2i>());
                }
            }
        }
    }

    void CarrierManager::RemoveCarriersForRoad(Road* road) {
        if (!road) return;
        Flag* ra = m_flagManager ? m_flagManager->ResolveFlag(road->a) : NULL;
        Flag* rb = m_flagManager ? m_flagManager->ResolveFlag(road->b) : NULL;
        for (int i = (int)m_carriers.size() - 1; i >= 0; --i) {
            Carrier* c = m_carriers[i];
            if (c->road == road) {
                if (c->state == WalkingToPost) {
                    if (c->road) {
                        UnregisterCarrier(c->road->carrier);
                        c->road->carrier = Handle<Carrier>();
                    }
                    delete c;
                    m_carriers.erase(m_carriers.begin() + i);
                    return;
                }
                if (c->job && m_flagManager) c->m_resolvedSourceFlag = m_flagManager->ResolveFlag(c->job->sourceFlag);
                RestoreCarrierCargo(c);
                ReleaseCarrierJob(c);
                if (c->road) c->road->carrier = Handle<Carrier>();
                c->road = NULL;

                // Walk back to warehouse
                if (m_warehouseFlag && m_roadManager) {
                    float pathLen = (float)(road->tiles.size() - 1);
                    if (pathLen < 1.0f) pathLen = 1.0f;
                    Flag* nearestFlag = (c->ep <= pathLen * 0.5f) ? ra : rb;
                    std::vector<Vector2i> returnPath = BuildTransitPath(nearestFlag, m_warehouseFlag);

                    std::vector<Vector2i> fullPath;
                    if (nearestFlag == ra) {
                        for (int t = (int)c->ep; t >= 0; --t)
                            fullPath.push_back(road->tiles[t]);
                    } else {
                        for (size_t t = (size_t)c->ep; t < road->tiles.size(); ++t)
                            fullPath.push_back(road->tiles[t]);
                    }
                    if (!fullPath.empty() && !returnPath.empty())
                        fullPath.pop_back();
                    fullPath.insert(fullPath.end(), returnPath.begin(), returnPath.end());
                    c->SetupReturningHome(fullPath);
                } else {
                    c->SetupReturningHome(std::vector<Vector2i>());
                }
                return;
            }
        }
    }

    void CarrierManager::SyncCarriersForRoad(Road* road) {
        if (!road) return;
        if (GetCarrierForRoad(road)) return;
        CreateCarrier(road);
    }

    Carrier* CarrierManager::GetCarrierForRoad(Road* road) const {
        for (size_t i = 0; i < m_carriers.size(); ++i) {
            if (m_carriers[i]->road == road)
                return m_carriers[i];
        }
        return NULL;
    }

    bool CarrierManager::IsRoadInUse(Road* road) const {
        for (size_t i = 0; i < m_carriers.size(); ++i) {
            if (m_carriers[i]->road == road)
                return true;
        }
        return false;
    }

    bool CarrierManager::IsFlagInUse(Flag* flag) const {
        if (!flag || !m_flagManager) return false;
        for (size_t i = 0; i < m_carriers.size(); ++i) {
            Carrier* c = m_carriers[i];
            if (c->job) {
                for (size_t j = 0; j < c->job->route.size(); ++j) {
                    if (m_flagManager->ResolveFlag(c->job->route[j]) == flag)
                        return true;
                }
            }
        }
        return false;
    }

    void CarrierManager::Update(float deltaTime) {
        for (int i = (int)m_carriers.size() - 1; i >= 0; --i) {
            Carrier* c = m_carriers[i];

            if (c->readyToRemove) {
                UnregisterCarrier(GetCarrierHandle(c));
                delete c;
                m_carriers.erase(m_carriers.begin() + i);
                continue;
            }

            // Resolve handles to cached Flag* pointers before Update
            c->m_resolvedLegFrom = NULL;
            c->m_resolvedLegTo = NULL;
            if (c->road) {
                c->m_roadEndpointA = m_flagManager ? m_flagManager->ResolveFlag(c->road->a) : NULL;
                c->m_roadEndpointB = m_flagManager ? m_flagManager->ResolveFlag(c->road->b) : NULL;
            } else {
                c->m_roadEndpointA = NULL;
                c->m_roadEndpointB = NULL;
            }
            if (c->job && m_flagManager) {
                c->m_resolvedSourceFlag = m_flagManager->ResolveFlag(c->job->sourceFlag);
                c->m_resolvedDestFlag = m_flagManager->ResolveFlag(c->job->destinationFlag);
                uint32_t leg = c->job->currentLeg;
                if (leg + 1 < c->job->route.size()) {
                    c->m_resolvedLegFrom = m_flagManager->ResolveFlag(c->job->route[leg]);
                    c->m_resolvedLegTo = m_flagManager->ResolveFlag(c->job->route[leg + 1]);
                }
            } else {
                c->m_resolvedSourceFlag = NULL;
                c->m_resolvedDestFlag = NULL;
            }

            c->Update(deltaTime);

            if (c->HasArrived()) {
                TransportJob* doneJob = c->job;
                c->ClearJob();
                if (m_jobManager && doneJob) {
                    m_jobManager->OnLegDelivered(doneJob);
                }
            }
        }
    }
}
