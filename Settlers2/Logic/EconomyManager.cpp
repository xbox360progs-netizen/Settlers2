#include "stdafx.h"
#include "EconomyManager.h"
#include "../World/Components/Building.h"
#include "../World/Flag.h"
#include "../World/Warehouse.h"
#include "../World/Worker.h"

namespace Logic {

    EconomyManager::EconomyManager()
        : m_warehouse(NULL), m_flagManager(NULL), m_roadManager(NULL), m_validateCounter(0)
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
        uint32_t fid = destFlag ? destFlag->id : 0;
        for (int i = 0; i < MAX_CONSTRUCTION_REQUESTS; ++i) {
            if (m_constructionRequests[i].active && m_constructionRequests[i].destFlagId == fid && m_constructionRequests[i].type == type) {
                return;
            }
        }
        for (int i = 0; i < MAX_CONSTRUCTION_REQUESTS; ++i) {
            if (!m_constructionRequests[i].active) {
                m_constructionRequests[i].destFlagId = fid;
                m_constructionRequests[i].destFlag = destFlag;
                m_constructionRequests[i].type = type;
                m_constructionRequests[i].amount = amount;
                m_constructionRequests[i].priority = priority;
                m_constructionRequests[i].active = true;

                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[Economy] Request created: %s -> flag %u\n",
                    World::ResourceTypeToString(type), fid);
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

        if (m_warehouse && m_warehouse->resources[type] > 0) {
            outAmount = m_warehouse->resources[type];
            return static_cast<World::Building*>(m_warehouse);
        }

        return NULL;
    }

    void EconomyManager::Update(float dt) {
        m_registry.ClearPlanningReservations();

        // ─── Phase 1a: Process pending construction resource requests ─────
        for (int r = 0; r < MAX_CONSTRUCTION_REQUESTS; ++r) {
            if (!m_constructionRequests[r].active) continue;

            if (!m_constructionRequests[r].destFlagId) { m_constructionRequests[r].active = false; continue; }
            if (!m_constructionRequests[r].destFlag) {
                m_constructionRequests[r].destFlag = m_flagManager ? m_flagManager->GetFlagById(m_constructionRequests[r].destFlagId) : NULL;
            }
            if (!m_constructionRequests[r].destFlag) { m_constructionRequests[r].active = false; continue; }

            if (!m_warehouse || !m_warehouse->connectedFlag) {
                m_constructionRequests[r].active = false;
                continue;
            }
            if (m_warehouse->resources[m_constructionRequests[r].type] <= 0) {
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
                continue;
            }

            m_warehouse->RemoveResource(m_constructionRequests[r].type, 1);
            m_constructionRequests[r].amount--;
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
                m_warehouse->RemoveResource(m_requests[r].type, 1);
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
            if (b->state != World::State_Finished) continue;

            if (b->m_numRules > 0) {
                for (int r = 0; r < b->m_numRules; ++r) {
                    for (int j = 0; j < b->m_rules[r].numInputs; ++j) {
                        World::ResourceType needed = b->m_rules[r].input[j];
                        int have = b->m_storage[needed];
                        if (have < b->m_rules[r].inputAmount[j]) {
                            RequestResource(b, needed, b->m_rules[r].inputAmount[j],
                                            m_settings.routeConfig[needed].transferPriority);
                        }
                    }
                }
            } else {
                for (size_t j = 0; j < b->inputResources.size(); ++j) {
                    World::ResourceType needed = b->inputResources[j];
                    int have = b->m_storage[needed];
                    if (have < 1) {
                        RequestResource(b, needed, 1, m_settings.routeConfig[needed].transferPriority);
                    }
                }
            }
        }

        // ─── Phase 3: Transfer flag → building inventory ──────────────────
        for (size_t i = 0; i < m_buildings.size(); ++i) {
            World::Building* b = m_buildings[i];
            if (b->state != World::State_Finished) continue;
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
            if (b->state != World::State_Finished) continue;
            b->Update(dt);
        }

        // ─── Phase 6: Outbound — routing-aware surplus distribution ──────
        uint32_t whFlagId = (m_warehouse && m_warehouse->connectedFlag) ? m_warehouse->connectedFlag->id : 0;
        for (size_t i = 0; i < m_buildings.size(); ++i) {
            World::Building* b = m_buildings[i];
            if (b->state != World::State_Finished) continue;
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
                    slot.destFlagId = 0;
                }
            }
        }

        // ─── Phase 8: Clean up stale construction requests ───────────────
        // Remove requests whose destFlag is gone or whose resources are no
        // longer needed (building already finished). Uses flag ID for safety.
        for (int r = 0; r < MAX_CONSTRUCTION_REQUESTS; ++r) {
            if (!m_constructionRequests[r].active) continue;
            if (!m_constructionRequests[r].destFlagId) { m_constructionRequests[r].active = false; continue; }
            // If the flag was deleted (ID lookup returns NULL), the request is stale
            if (!m_flagManager) { m_constructionRequests[r].active = false; continue; }
            World::Flag* df = m_flagManager->GetFlagById(m_constructionRequests[r].destFlagId);
            if (!df) { m_constructionRequests[r].active = false; continue; }
            // If the flag now has a finished building, the construction request is stale
            if (df->hasBuilding && df->building && df->building->state == World::State_Finished) {
                m_constructionRequests[r].active = false;
            }
        }

        m_validateCounter++;
        if (m_validateCounter >= 300) {
            m_validateCounter = 0;
            ValidateEconomy();
        }
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

            total += m_warehouse->resources[type];

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
        if (!building || !m_warehouse) return false;
        for (size_t i = 0; i < m_warehouse->specialists.size(); ++i) {
            if (m_warehouse->specialists[i]->home == building) return true;
        }
        return false;
    }

    void EconomyManager::AddBuilding(World::Building* building) {
        m_buildings.push_back(building);
        if (building->state == World::State_Finished)
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
        if (m_warehouse && m_warehouse->connectedFlag) {
            World::Flag* whFlag = m_warehouse->connectedFlag;
            for (int si = 0; si < 8; ++si) {
                World::ResourceSlot& slot = whFlag->slots[si];
                if (slot.type == World::ResourceType_None || slot.amount <= 0) continue;
                // Collect resources destined for NOTHING (free surplus) OR destined for this very flag
                // (incoming from Phase-6 routing / carrier delivery).  Resources with a *different*
                // destFlagId are in-transit and must stay on the flag.
                if (slot.destFlagId != 0 && slot.destFlagId != whFlag->id) continue;
                if (slot.amount > 0) {
                    whFlag->RemoveResource(slot.type, 1);
                    m_warehouse->AddResource(slot.type, 1);
                }
            }
        }
    }
}
