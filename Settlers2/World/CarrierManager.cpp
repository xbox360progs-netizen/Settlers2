#include "stdafx.h"
#include "CarrierManager.h"
#include "FlagManager.h"
#include "TransportJobManager.h"
#include "Flag.h"
#include "Road.h"
#include "RoadManager.h"
#include "Systems/CarrierSystem.h"

namespace World {

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
        : m_flagManager(NULL), m_jobManager(NULL), m_roadManager(NULL), m_warehouseFlag(NULL), m_carrierSystem(NULL),
          m_demandManager(NULL), m_cargoManager(NULL)
    {
    }

    std::vector<Vector2i> CarrierManager::BuildTransitPath(Flag* fromFlag, Flag* toFlag) {
        std::vector<Vector2i> result;
        if (!fromFlag || !toFlag || !m_roadManager) return result;

        std::vector<Flag*> flagPath = m_roadManager->FindFlagPath(fromFlag, toFlag);
        if (flagPath.size() < 2) return result;

        // Resolve direction for each road segment: if we're traversing from the road's B end
        // to the A end, reverse the tile order so the path flows continuously.
        for (size_t i = 0; i + 1 < flagPath.size(); ++i) {
            Road* road = m_roadManager->GetRoadBetween(flagPath[i], flagPath[i + 1]);
            if (!road || road->tiles.size() < 2) continue;

            Flag* roadA = m_flagManager ? m_flagManager->ResolveFlag(road->a) : NULL;
            // flagPath[i] is the source, flagPath[i+1] is the destination
            bool reverse = (roadA == flagPath[i + 1]); // if road A is the DESTINATION, we're going B→A

            if (i == 0) {
                // First road: include the source flag tile
                if (reverse) {
                    for (int t = (int)road->tiles.size() - 1; t >= 0; --t)
                        result.push_back(road->tiles[t]);
                } else {
                    for (size_t t = 0; t < road->tiles.size(); ++t)
                        result.push_back(road->tiles[t]);
                }
            } else {
                // Subsequent roads: skip first tile (overlaps previous road's last tile)
                if (reverse) {
                    for (int t = (int)road->tiles.size() - 2; t >= 0; --t)
                        result.push_back(road->tiles[t]);
                } else {
                    for (size_t t = 1; t < road->tiles.size(); ++t)
                        result.push_back(road->tiles[t]);
                }
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

        // Create ECS entity for movement (starts at progress=0, matches new Carrier)
        if (m_carrierSystem && c->ecsEntity == INVALID_ENTITY) {
            std::vector<Vector2i> tiles;
            if (IsTransitState(c->state)) {
                tiles = c->transitTiles;
            } else if (c->road && c->road->tiles.size() >= 2) {
                tiles = c->road->tiles;
            }
            if (!tiles.empty()) {
                c->ecsEntity = m_carrierSystem->CreateCarrier(CarrierInit(tiles, c->state, c->pathVersion));
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
                // Release any cargo the carrier was holding back to its last flag
                if (c->m_cargo) {
                    Flag* dropFlag = (c->walkDir > 0.0f) ? rb : ra;
                    if (dropFlag) {
                        dropFlag->AcceptCargo(c->m_cargo);
                    }
                    c->m_cargo = NULL;
                }
                if (c->ecsEntity != INVALID_ENTITY && m_carrierSystem)
                    m_carrierSystem->RemoveCarrier(c->ecsEntity);
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
                    if (c->ecsEntity != INVALID_ENTITY && m_carrierSystem)
                        m_carrierSystem->RemoveCarrier(c->ecsEntity);
                    if (c->road) {
                        UnregisterCarrier(c->road->carrier);
                        c->road->carrier = Handle<Carrier>();
                    }
                    delete c;
                    m_carriers.erase(m_carriers.begin() + i);
                    continue;
                }
                // Release any cargo being carried
                if (c->m_cargo) {
                    Flag* dropFlag = (c->walkDir > 0.0f) ? rb : ra;
                    if (dropFlag) {
                        dropFlag->AcceptCargo(c->m_cargo);
                    }
                    c->m_cargo = NULL;
                }
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
                // Update ECS path for return journey
                if (c->ecsEntity != INVALID_ENTITY && m_carrierSystem)
                    m_carrierSystem->UpdatePath(c->ecsEntity, c->transitTiles);
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
                    if (c->ecsEntity != INVALID_ENTITY && m_carrierSystem)
                        m_carrierSystem->RemoveCarrier(c->ecsEntity);
                    if (c->road) {
                        UnregisterCarrier(c->road->carrier);
                        c->road->carrier = Handle<Carrier>();
                    }
                    delete c;
                    m_carriers.erase(m_carriers.begin() + i);
                    return;
                }
                // Release any cargo being carried
                if (c->m_cargo) {
                    Flag* dropFlag = (c->walkDir > 0.0f) ? rb : ra;
                    if (dropFlag) {
                        dropFlag->AcceptCargo(c->m_cargo);
                    }
                    c->m_cargo = NULL;
                }
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
                // Update ECS path for return journey
                if (c->ecsEntity != INVALID_ENTITY && m_carrierSystem)
                    m_carrierSystem->UpdatePath(c->ecsEntity, c->transitTiles);
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
            if (c->road) {
                Flag* ra = m_flagManager->ResolveFlag(c->road->a);
                Flag* rb = m_flagManager->ResolveFlag(c->road->b);
                if (ra == flag || rb == flag)
                    return true;
            }
        }
        return false;
    }

    void CarrierManager::Update(float deltaTime) {
        // Phase 1: Check readyToRemove, resolve endpoints, update carriers
        for (int i = (int)m_carriers.size() - 1; i >= 0; --i) {
            Carrier* c = m_carriers[i];

            if (c->readyToRemove) {
                if (c->ecsEntity != INVALID_ENTITY && m_carrierSystem)
                    m_carrierSystem->RemoveCarrier(c->ecsEntity);
                UnregisterCarrier(GetCarrierHandle(c));
                delete c;
                m_carriers.erase(m_carriers.begin() + i);
                continue;
            }

            // Set manager pointers
            c->m_demandManager = m_demandManager;
            c->m_cargoManager = m_cargoManager;
            c->m_roadManager = m_roadManager;

            // Resolve handles to cached Flag* pointers
            if (c->road) {
                c->m_roadEndpointA = m_flagManager ? m_flagManager->ResolveFlag(c->road->a) : NULL;
                c->m_roadEndpointB = m_flagManager ? m_flagManager->ResolveFlag(c->road->b) : NULL;
            } else {
                c->m_roadEndpointA = NULL;
                c->m_roadEndpointB = NULL;
            }

            c->Update(deltaTime);
        }

        // Phase 2: Push Carrier -> ECS (SyncLegTargets)
        for (size_t i = 0; i < m_carriers.size(); ++i) {
            Carrier* c = m_carriers[i];
            if (c->ecsEntity != INVALID_ENTITY && m_carrierSystem) {
                m_carrierSystem->SyncLegTargets(c->ecsEntity, c);
            }
        }

        // Phase 3: Sync ECS -> Carrier
        for (size_t i = 0; i < m_carriers.size(); ++i) {
            Carrier* c = m_carriers[i];
            if (c->ecsEntity != INVALID_ENTITY && m_carrierSystem) {
                m_carrierSystem->SyncToCarrier(c->ecsEntity, c);
                m_carrierSystem->DebugECSInvariants(c->ecsEntity, c);
            }
        }

        // Phase 4: Deliver cargo that reached its destination flag
        if (m_flagManager && m_demandManager && m_cargoManager) {
            for (size_t fi = 0; fi < m_flagManager->GetCount(); ++fi) {
                Flag* flag = m_flagManager->GetFlag(fi);
                if (flag && !flag->cargo.empty()) {
                    flag->CheckDeliveries(m_demandManager, m_cargoManager);
                }
            }
        }
    }
}
