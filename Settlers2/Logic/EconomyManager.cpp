#include "stdafx.h"
#include "EconomyManager.h"
#include "../World/Components/Building.h"
#include "../World/Flag.h"
#include "../World/Warehouse.h"
#include "../World/Worker.h"
#include "../World/Map.h"

namespace Logic {

    EconomyManager::EconomyManager()
        : m_warehouse(NULL), m_flagManager(NULL), m_roadManager(NULL), m_cargoManager(NULL), m_storehouseManager(NULL), m_demandManager(NULL), m_validateCounter(0), m_phase6Initialized(false)
    {
        for (int i = 0; i < MAX_REQUESTS; ++i)
            m_requests[i].active = false;

        for (int i = 0; i < MAX_CONSTRUCTION_REQUESTS; ++i)
            m_constructionRequests[i].active = false;
    }

    void EconomyManager::RequestResource(World::Building* requester, World::ResourceType type, int amount, int priority) {
        for (int i = 0; i < MAX_REQUESTS; ++i) {
            if (m_requests[i].active && m_requests[i].requester == requester && m_requests[i].type == type) {
                return;
            }
        }
        for (int i = 0; i < MAX_REQUESTS; ++i) {
            if (!m_requests[i].active) {
                m_requests[i].requester = requester;
                m_requests[i].type = type;
                m_requests[i].amount = amount;
                m_requests[i].priority = priority;
                m_requests[i].active = true;
                return;
            }
        }
    }

    void EconomyManager::RequestConstructionResource(World::Flag* destFlag, World::ResourceType type, int amount, int priority) {
        uint32_t fid = destFlag ? destFlag->id : World::INVALID_FLAG_ID;
        for (int i = 0; i < MAX_CONSTRUCTION_REQUESTS; ++i) {
            if (m_constructionRequests[i].active && m_constructionRequests[i].destFlagId == fid && m_constructionRequests[i].type == type) {
                // Update amount to match current need, but never exceed what's not yet been pushed.
                // amount tracks "more to push from warehouse"; if deliveries occurred (woodMissing
                // decreased), the delta is how many are newly available to push.
                int delivered = m_constructionRequests[i].totalRequested - amount;
                if (delivered < 0) delivered = 0;
                // Amount already pushed from warehouse = totalRequested - amount_current
                int alreadyPushed = m_constructionRequests[i].totalRequested - m_constructionRequests[i].amount;
                if (alreadyPushed < 0) alreadyPushed = 0;
                int inTransit = alreadyPushed - delivered;
                if (inTransit < 0) inTransit = 0;
                int newAmount = amount - inTransit;
                if (newAmount < 0) newAmount = 0;
                m_constructionRequests[i].amount = newAmount;
                return;
            }
        }
        for (int i = 0; i < MAX_CONSTRUCTION_REQUESTS; ++i) {
            if (!m_constructionRequests[i].active) {
                m_constructionRequests[i].destFlagId = fid;
                m_constructionRequests[i].destFlag = destFlag;
                m_constructionRequests[i].type = type;
                m_constructionRequests[i].amount = amount;
                m_constructionRequests[i].totalRequested = amount;
                m_constructionRequests[i].priority = priority;
                m_constructionRequests[i].active = true;

                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[Economy] Request created: %s -> flag %u amount=%d priority=%d\n",
                    World::ResourceTypeToString(type), fid, amount, priority);
                OutputDebugStringA(buf);

                return;
            }
        }
    }

    World::Building* EconomyManager::FindBestSupplier(
        World::ResourceType type,
        int& outAmount,
        World::Building* exclude,
        const Vector2i& requesterPos)
    {
        World::Building* b = m_registry.FindBestSupplier(type, outAmount, exclude, requesterPos, NULL);
        if (b) return b;

        if (m_storehouseManager && m_storehouseManager->GetStoredCount(type) > 0) {
            outAmount = (int)m_storehouseManager->GetStoredCount(type);
            return static_cast<World::Building*>(m_warehouse);
        }

        return NULL;
    }

    void EconomyManager::Update(float dt) {
        m_registry.ClearPlanningReservations();

        // ─── Phase 1a: Process pending construction resource requests ─────
        for (int r = 0; r < MAX_CONSTRUCTION_REQUESTS; ++r) {
            if (!m_constructionRequests[r].active) continue;

            if (m_constructionRequests[r].destFlagId == World::INVALID_FLAG_ID) { m_constructionRequests[r].active = false; continue; }
            if (!m_constructionRequests[r].destFlag) {
                m_constructionRequests[r].destFlag = m_flagManager ? m_flagManager->GetFlagById(m_constructionRequests[r].destFlagId) : NULL;
            }
            if (!m_constructionRequests[r].destFlag) { m_constructionRequests[r].active = false; continue; }

            if (!m_warehouse || !m_warehouse->connectedFlag) {
                m_constructionRequests[r].active = false;
                continue;
            }
            if (!m_storehouseManager || m_storehouseManager->GetStoredCount(m_constructionRequests[r].type) <= 0) {
                continue;
            }

            // Check if destination flag is reachable via road network
            World::Flag* whFlag = m_warehouse->connectedFlag;
            bool reachable = false;
            if (m_flagManager) {
                std::vector<World::Flag*> path = m_roadManager->FindFlagPath(
                    whFlag, m_constructionRequests[r].destFlag);
                reachable = !path.empty();
            }
            if (!reachable) continue;

            // Stop releasing once budget is exhausted — keep request active for
            // Phase 8 cleanup (stops GenerateRequests from recreating it).
            if (m_constructionRequests[r].amount <= 0) continue;

            if (!whFlag->AddResource(m_constructionRequests[r].type, 1, m_constructionRequests[r].destFlagId)) {
                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[CONSTR PHASE1a] skip: wh->flag AddResource failed type=%s dest=%u\n",
                    World::ResourceTypeToString(m_constructionRequests[r].type),
                    m_constructionRequests[r].destFlagId);
                OutputDebugStringA(buf);
                continue;
            }

            m_storehouseManager->RemoveResourceFromStorehouse(m_warehouse->m_storehouseIndex, m_constructionRequests[r].type, 1);
            m_constructionRequests[r].amount--;
            {
                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[CONSTR PHASE1a] pushed %s whFlag=%u dest=%u remaining=%d\n",
                    World::ResourceTypeToString(m_constructionRequests[r].type),
                    whFlag->id, m_constructionRequests[r].destFlagId,
                    m_constructionRequests[r].amount);
                OutputDebugStringA(buf);
            }
        }

        // ─── Phase 1b: Process pending building ResourceRequests ──────────
        for (int r = 0; r < MAX_REQUESTS; ++r) {
            if (!m_requests[r].active) continue;

            int available = 0;
            World::Building* producer = FindBestSupplier(
                m_requests[r].type, available, m_requests[r].requester,
                m_requests[r].requester->pos);
            if (!producer) continue;

            if (!producer->connectedFlag || !m_requests[r].requester->connectedFlag) continue;

            if (!producer->connectedFlag->AddResource(m_requests[r].type, 1, m_requests[r].requester->connectedFlag->id))
                continue;

            if (producer == static_cast<World::Building*>(m_warehouse)) {
                m_storehouseManager->RemoveResourceFromStorehouse(m_warehouse->m_storehouseIndex, m_requests[r].type, 1);
            } else {
                producer->m_storage[m_requests[r].type]--;
                if (producer->m_storage[m_requests[r].type] < 0)
                    producer->m_storage[m_requests[r].type] = 0;
            }

            m_requests[r].amount--;
            if (m_requests[r].amount <= 0) {
                m_requests[r].active = false;
            }
        }

        // ─── Phase 2: Generate requests for buildings that need inputs ───
        for (size_t i = 0; i < m_buildings.size(); ++i) {
            World::Building* b = m_buildings[i];

            if (b->m_numRules > 0) {
                for (int r = 0; r < b->m_numRules; ++r) {
                    for (int j = 0; j < b->m_rules[r].numInputs; ++j) {
                        World::ResourceType needed = b->m_rules[r].input[j];
                        int have = b->m_storage[needed];
                        if (have < b->m_rules[r].inputAmount[j]) {
                            RequestResource(b, needed, b->m_rules[r].inputAmount[j],
                                            m_settings.routeConfig[needed].transferPriority);
                            if (m_demandManager && b->connectedFlag) {
                                m_demandManager->SetDemand(needed,
                                    b->m_rules[r].inputAmount[j],
                                    b->connectedFlag->handle,
                                    m_settings.routeConfig[needed].transferPriority);
                            }
                        }
                    }
                }
            } else {
                for (size_t j = 0; j < b->inputResources.size(); ++j) {
                    World::ResourceType needed = b->inputResources[j];
                    int have = b->m_storage[needed];
                    if (have < 1) {
                        RequestResource(b, needed, 1, m_settings.routeConfig[needed].transferPriority);
                        if (m_demandManager && b->connectedFlag) {
                            m_demandManager->SetDemand(needed, 1,
                                b->connectedFlag->handle,
                                m_settings.routeConfig[needed].transferPriority);
                        }
                    }
                }
            }
        }

        // ─── Phase 3: Transfer flag → building inventory ──────────────────
        for (size_t i = 0; i < m_buildings.size(); ++i) {
            World::Building* b = m_buildings[i];
            if (!b->connectedFlag) continue;

            if (b->m_numRules > 0) {
                for (int r = 0; r < b->m_numRules; ++r) {
                    for (int j = 0; j < b->m_rules[r].numInputs; ++j) {
                        World::ResourceType needed = b->m_rules[r].input[j];
                        int needCount = b->m_rules[r].inputAmount[j];
                        while (needCount > 0 && b->connectedFlag->GetAvailable(needed) > 0) {
                            b->connectedFlag->RemoveResource(needed, 1);
                            b->m_storage[needed]++;
                            needCount--;
                        }
                    }
                }
            } else {
                for (size_t j = 0; j < b->inputResources.size(); ++j) {
                    World::ResourceType needed = b->inputResources[j];
                    if (b->connectedFlag->GetAvailable(needed) > 0) {
                        b->connectedFlag->RemoveResource(needed, 1);
                        b->m_storage[needed]++;
                    }
                }
            }
        }

        // ─── Phase 4: Building production ────────────────────────────────
        for (size_t i = 0; i < m_buildings.size(); ++i) {
            World::Building* b = m_buildings[i];
            b->Update(dt);
        }

        // ─── Phase 5: Depleted building sprite update ────────────────────
        for (size_t i = 0; i < m_buildings.size(); ++i) {
            World::Building* b = m_buildings[i];
            if (!b->IsDepleted() || b->m_depletedSpriteIdx < 0) continue;

            World::TileLayer* buildingsLayer = b->map ? b->map->GetLayer(World::Buildings) : NULL;
            if (buildingsLayer) {
                int tx = b->pos.x;
                int ty = b->pos.y;
                if (tx >= 0 && tx < buildingsLayer->GetWidth() && ty >= 0 && ty < buildingsLayer->GetHeight()) {
                    World::Tile& tile = buildingsLayer->GetTile(tx, ty);
                    tile.regionIndex = b->m_depletedSpriteIdx;
                }
            }
            // Clear the idx so we only update once
            b->m_depletedSpriteIdx = -1;
        }

        // ─── Phase 6: Outbound — routing-aware surplus distribution ──────
        uint32_t whFlagId = (m_warehouse && m_warehouse->connectedFlag) ? m_warehouse->connectedFlag->id : World::INVALID_FLAG_ID;
        if (whFlagId == 0) whFlagId = World::INVALID_FLAG_ID;  // Safety: id=0 is invalid
        if (!m_phase6Initialized && whFlagId != World::INVALID_FLAG_ID) {
            m_phase6Initialized = true;
            char dbg[256];
            _snprintf(dbg, sizeof(dbg),
                "[Phase6 Init] m_warehouse=%p connectedFlag=%p whFlagId=%u INVALID=%u\n",
                m_warehouse,
                m_warehouse ? m_warehouse->connectedFlag : NULL,
                whFlagId,
                World::INVALID_FLAG_ID);
            OutputDebugStringA(dbg);
        }
        if (whFlagId == World::INVALID_FLAG_ID) {
            static int warnCounter = 0;
            if (++warnCounter % 60 == 0) {
                OutputDebugStringA("[Phase6] WARNING: no valid warehouse flag\n");
            }
            whFlagId = 0;  // fallback: let CollectWarehouse pick up (destFlagId=0)
        }
        for (size_t i = 0; i < m_buildings.size(); ++i) {
            World::Building* b = m_buildings[i];
            if (!b->connectedFlag) continue;

            if (b->m_numRules > 0) {
                for (int r = 0; r < b->m_numRules; ++r) {
                    for (int o = 0; o < b->m_rules[r].numOutputs; ++o) {
                        World::ResourceType outType = b->m_rules[r].output[o];
                        ResourceRouteConfig& cfg = m_settings.routeConfig[outType];

                        if (b->m_storage[outType] <= 0) continue;

                        if (cfg.routing == ROUTE_DIRECT) continue;

                        if (!b->connectedFlag->AddResource(outType, 1, whFlagId)) continue;
                        b->m_storage[outType]--;
                    }
                }
            } else {
                for (size_t j = 0; j < b->outputResources.size(); ++j) {
                    World::ResourceType outType = b->outputResources[j];
                    ResourceRouteConfig& cfg = m_settings.routeConfig[outType];
                    if (b->m_storage[outType] <= 0) continue;
                    if (cfg.routing == ROUTE_DIRECT) continue;
                    if (!b->connectedFlag->AddResource(outType, 1, whFlagId)) continue;
                    b->m_storage[outType]--;
                }
            }
        }
        // ─── Phase 7: Reroute orphaned resources ─────────────────────────
        // Resources whose destination is no longer reachable get redirected
        // to the warehouse flag (or cleared if warehouse is unreachable).
        // Uses flag IDs to safely handle deleted destination flags.
        if (m_flagManager && m_warehouse && m_warehouse->connectedFlag) {
            uint32_t whFlagId = m_warehouse->connectedFlag->id;
            for (size_t fi = 0; fi < m_flagManager->GetCount(); ++fi) {
                World::Flag* f = m_flagManager->GetFlag(fi);
                if (!f) continue;
                for (int si = 0; si < 8; ++si) {
                    World::ResourceSlot& slot = f->slots[si];
                    if (slot.type == World::ResourceType_None || slot.amount <= 0) continue;
                    if (!slot.destFlagId) continue;

                    // Look up destination flag by ID (NULL if flag was deleted)
                    World::Flag* destFlag = m_flagManager->GetFlagById(slot.destFlagId);
                    if (destFlag) {
                        if (f == destFlag) continue; // already at destination
                        std::vector<World::Flag*> path = m_roadManager->FindFlagPath(f, destFlag);
                        if (!path.empty()) continue; // still reachable
                    }
                    // Destination is deleted or unreachable — reroute
                    if (f->id != whFlagId) {
                        World::Flag* whFlag = m_warehouse->connectedFlag;
                        if (whFlag && whFlag->id == whFlagId) {
                            std::vector<World::Flag*> whPath = m_roadManager->FindFlagPath(f, whFlag);
                            if (!whPath.empty()) {
                                slot.destFlagId = whFlagId;
                                continue;
                            }
                        }
                    }
                    // Clear destination — resource becomes free (warehouse or any flag can collect it)
                    slot.destFlagId = World::INVALID_FLAG_ID;
                }
            }
        }

        // ─── Phase 8: Clean up stale construction requests ───────────────
        // Remove requests whose destFlag is gone or whose resources are no
        // longer needed (building already finished). Uses flag ID for safety.
        for (int r = 0; r < MAX_CONSTRUCTION_REQUESTS; ++r) {
            if (!m_constructionRequests[r].active) continue;
            if (m_constructionRequests[r].destFlagId == World::INVALID_FLAG_ID) { m_constructionRequests[r].active = false; continue; }
            // If the flag was deleted (ID lookup returns NULL), the request is stale
            if (!m_flagManager) { m_constructionRequests[r].active = false; continue; }
            World::Flag* df = m_flagManager->GetFlagById(m_constructionRequests[r].destFlagId);
            if (!df) { m_constructionRequests[r].active = false; continue; }
            // If the flag now has a finished building, the construction request is stale
            if (df->hasBuilding && df->building != NULL) {
                m_constructionRequests[r].active = false;
            }
        }

        m_validateCounter++;
        if (m_validateCounter >= 300) {
            m_validateCounter = 0;
            ValidateEconomy();
        }
    }

    int EconomyManager::GetTotalStock(World::ResourceType type) const {
        int total = 0;

        for (size_t i = 0; i < m_buildings.size(); ++i) {
            total += m_buildings[i]->m_storage[type];
        }

        if (m_storehouseManager) {
            total += (int)m_storehouseManager->GetStoredCount(type);
        }

        if (m_flagManager) {
            for (size_t i = 0; i < m_flagManager->GetCount(); ++i) {
                World::Flag* f = m_flagManager->GetFlag(i);
                if (f) {
                    total += f->GetAvailable(type);
                }
            }
        }

        if (m_storehouseManager) {
            total += (int)m_storehouseManager->GetInTransitCount(type);
        }

        return total;
    }

    int EconomyManager::GetCargoInTransit(World::ResourceType type) const {
        if (!m_storehouseManager) return 0;
        return (int)m_storehouseManager->GetInTransitCount(type);
    }

    int EconomyManager::GetCargoOnFlags(World::ResourceType type) const {
        if (!m_cargoManager) return 0;
        int count = 0;
        for (int ci = 0; ci < m_cargoManager->GetActiveCount(); ++ci) {
            World::Cargo* c = m_cargoManager->GetCargoByActiveIdx(ci);
            if (c->state == World::Cargo_OnFlag && c->type == type) {
                count += c->amount;
            }
        }
        return count;
    }

    void EconomyManager::ValidateEconomy() {
        if (!m_warehouse) return;

        static const World::ResourceType tracked[] = {
            World::ResourceType_Wood,
            World::ResourceType_Planks,
            World::ResourceType_Stone
        };
        static const int numTracked = sizeof(tracked) / sizeof(tracked[0]);

        for (int t = 0; t < numTracked; ++t) {
            World::ResourceType type = tracked[t];
            int total = 0;

            for (size_t i = 0; i < m_buildings.size(); ++i) {
                World::Building* b = m_buildings[i];
                total += b->m_storage[type];
            }

            total += (int)m_storehouseManager->GetStoredCount(type);

            for (size_t i = 0; i < m_buildings.size(); ++i) {
                World::Building* b = m_buildings[i];
                if (b->connectedFlag) {
                    total += b->connectedFlag->GetAvailable(type);
                }
            }
            if (m_warehouse->connectedFlag) {
                total += m_warehouse->connectedFlag->GetAvailable(type);
            }

            if (total < 0) {
                DebugBreak();
            }
        }
    }

    bool EconomyManager::HasActiveConstructionRequest(World::Flag* destFlag) const {
        if (!destFlag) return false;
        for (int i = 0; i < MAX_CONSTRUCTION_REQUESTS; ++i) {
            if (m_constructionRequests[i].active && m_constructionRequests[i].destFlagId == destFlag->id) {
                return true;
            }
        }
        return false;
    }

    bool EconomyManager::HasWorkers(World::Building* building) const {
        if (!building) return false;
        return building->m_population > 0;
    }

    void EconomyManager::AddBuilding(World::Building* building) {
        m_buildings.push_back(building);
        m_registry.Register(building);
    }

    void EconomyManager::RemoveBuilding(World::Building* building) {
        for (size_t i = 0; i < m_buildings.size(); ++i) {
            if (m_buildings[i] == building) {
                m_buildings.erase(m_buildings.begin() + i);
                break;
            }
        }
        m_registry.Unregister(building);
    }

    bool EconomyManager::HasBuilding(World::BuildingType type) const {
        for (size_t i = 0; i < m_buildings.size(); ++i) {
            if (m_buildings[i]->type == type) {
                return true;
            }
        }
        return false;
    }

    void EconomyManager::CollectWarehouse() {
        if (m_warehouse && m_warehouse->connectedFlag && m_storehouseManager && m_warehouse->m_storehouseIndex >= 0) {
            World::Flag* whFlag = m_warehouse->connectedFlag;
            for (int si = 0; si < 8; ++si) {
                World::ResourceSlot& slot = whFlag->slots[si];
                if (slot.type == World::ResourceType_None || slot.amount <= 0) continue;
                // Collect resources destined for NOTHING (free surplus), INVALID, or this warehouse flag
                // Resources with a *different* destFlagId are in-transit and must stay on the flag.
                if (slot.destFlagId != 0 && slot.destFlagId != World::INVALID_FLAG_ID && slot.destFlagId != whFlag->id) continue;
                if (slot.amount > 0) {
                    whFlag->RemoveResource(slot.type, 1);
                    m_storehouseManager->AddResourceToStorehouse(m_warehouse->m_storehouseIndex, slot.type, 1);
                }
            }
        }
    }
}
