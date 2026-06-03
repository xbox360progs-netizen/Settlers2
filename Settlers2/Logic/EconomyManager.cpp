#include "stdafx.h"
#include "EconomyManager.h"
#include "../World/Components/Building.h"
#include "../World/Flag.h"
#include "../World/Warehouse.h"

namespace Logic {

    EconomyManager::EconomyManager()
        : m_warehouse(NULL), m_validateCounter(0)
    {
        for (int i = 0; i < MAX_REQUESTS; ++i)
            m_requests[i].active = false;

        for (int i = 0; i < World::ResourceType_Count; ++i)
            m_deliveryReserved[i] = 0;
    }

    void EconomyManager::RequestResource(World::Building* requester, World::ResourceType type, int amount, int priority) {
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

    void EconomyManager::ComputeDeliveryReserved() {
        for (int t = 0; t < World::ResourceType_Count; ++t)
            m_deliveryReserved[t] = 0;

        for (size_t i = 0; i < m_buildings.size(); ++i) {
            World::Building* b = m_buildings[i];
            if (!b->connectedFlag) continue;

            for (int s = 0; s < 8; ++s) {
                World::ResourceType type = b->connectedFlag->slots[s].type;
                int reserved = b->connectedFlag->slots[s].reserved;
                if (type != World::ResourceType_None && reserved > 0) {
                    m_deliveryReserved[type] += reserved;
                }
            }
        }

        if (m_warehouse && m_warehouse->connectedFlag) {
            for (int s = 0; s < 8; ++s) {
                World::ResourceType type = m_warehouse->connectedFlag->slots[s].type;
                int reserved = m_warehouse->connectedFlag->slots[s].reserved;
                if (type != World::ResourceType_None && reserved > 0) {
                    m_deliveryReserved[type] += reserved;
                }
            }
        }
    }

    World::Building* EconomyManager::FindBestSupplier(
        World::ResourceType type,
        int& outAmount,
        World::Building* exclude,
        const Vector2i& requesterPos)
    {
        // 1. Nearest producer with stock
        World::Building* b = m_registry.FindBestSupplier(type, outAmount, exclude, requesterPos, m_deliveryReserved);
        if (b) return b;

        // 2. Warehouse as fallback (include distance)
        if (m_warehouse && m_warehouse->resources[type] > 0) {
            outAmount = m_warehouse->resources[type];
            return static_cast<World::Building*>(m_warehouse);
        }

        return NULL;
    }

    void EconomyManager::Update(World::CarrierManager* carrierManager) {
        ComputeDeliveryReserved();
        m_registry.ClearPlanningReservations();

        // ─── Phase 1: Process pending ResourceRequests ───────────────────
        for (int r = 0; r < MAX_REQUESTS; ++r) {
            if (!m_requests[r].active) continue;

            int available = 0;
            World::Building* producer = FindBestSupplier(
                m_requests[r].type, available, m_requests[r].requester,
                m_requests[r].requester->pos);
            if (!producer) continue;

            if (!producer->connectedFlag || !m_requests[r].requester->connectedFlag) continue;

            int toDeliver = m_requests[r].amount;
            if (toDeliver > available) toDeliver = available;

            if (!producer->connectedFlag->AddResource(m_requests[r].type, toDeliver))
                continue;

            if (!producer->connectedFlag->Reserve(m_requests[r].type, toDeliver))
                continue;

            m_registry.ReservePlanning(m_requests[r].type, toDeliver);

            World::TransportJob job;
            job.cargo = World::Cargo(m_requests[r].type, (uint8_t)toDeliver);
            job.source = producer->connectedFlag;
            job.destination = m_requests[r].requester->connectedFlag;
            job.priority = m_settings.routeConfig[m_requests[r].type].transferPriority;
            carrierManager->AssignJob(job);

            if (producer == static_cast<World::Building*>(m_warehouse)) {
                m_warehouse->RemoveResource(m_requests[r].type, toDeliver);
            } else {
                std::map<World::ResourceType, int>::iterator it = producer->inventory.find(m_requests[r].type);
                if (it != producer->inventory.end()) {
                    it->second -= toDeliver;
                    if (it->second < 0) it->second = 0;
                }
            }

            m_requests[r].active = false;
        }

        // ─── Phase 2: Generate requests for buildings that need inputs ───
        for (size_t i = 0; i < m_buildings.size(); ++i) {
            World::Building* b = m_buildings[i];
            if (b->state != World::State_Finished) continue;

            for (size_t j = 0; j < b->inputResources.size(); ++j) {
                World::ResourceType needed = b->inputResources[j];
                std::map<World::ResourceType, int>::iterator it = b->inventory.find(needed);
                int have = (it != b->inventory.end()) ? it->second : 0;
                if (have < 1) {
                    RequestResource(b, needed, 1, m_settings.routeConfig[needed].transferPriority);
                }
            }
        }

        // ─── Phase 3: Transfer flag → building inventory ──────────────────
        for (size_t i = 0; i < m_buildings.size(); ++i) {
            World::Building* b = m_buildings[i];
            if (b->state != World::State_Finished) continue;
            if (!b->connectedFlag) continue;

            for (size_t j = 0; j < b->inputResources.size(); ++j) {
                World::ResourceType needed = b->inputResources[j];
                if (b->connectedFlag->GetAvailable(needed) > 0) {
                    b->connectedFlag->RemoveResource(needed, 1);
                    b->inventory[needed]++;
                }
            }
        }

        // ─── Phase 4: Building production ────────────────────────────────
        for (size_t i = 0; i < m_buildings.size(); ++i) {
            World::Building* b = m_buildings[i];
            if (b->state != World::State_Finished) continue;
            b->Update();
        }

        // ─── Phase 5: Warehouse collects from its flag ───────────────────
        if (m_warehouse && m_warehouse->connectedFlag) {
            for (int t = 0; t < World::ResourceType_Count; ++t) {
                World::ResourceType type = (World::ResourceType)t;
                if (m_warehouse->connectedFlag->GetAvailable(type) > 0) {
                    m_warehouse->connectedFlag->RemoveResource(type, 1);
                    m_warehouse->AddResource(type, 1);
                }
            }
        }

        // ─── Phase 6: Outbound — routing-aware surplus distribution ──────
        for (size_t i = 0; i < m_buildings.size(); ++i) {
            World::Building* b = m_buildings[i];
            if (b->state != World::State_Finished) continue;
            if (!b->connectedFlag) continue;

            for (size_t j = 0; j < b->outputResources.size(); ++j) {
                World::ResourceType outType = b->outputResources[j];
                ResourceRouteConfig& cfg = m_settings.routeConfig[outType];

                std::map<World::ResourceType, int>::iterator it = b->inventory.find(outType);
                if (it == b->inventory.end() || it->second <= 0) continue;

                if (cfg.routing == ROUTE_DIRECT) {
                    continue;
                }

                if (!m_warehouse || !m_warehouse->connectedFlag) continue;
                if (!b->connectedFlag->AddResource(outType, 1))
                    continue;
                it->second--;

                m_registry.ReservePlanning(outType, 1);

                World::TransportJob job;
                job.cargo = World::Cargo(outType, 1);
                job.source = b->connectedFlag;
                job.destination = m_warehouse->connectedFlag;
                job.priority = cfg.transferPriority;
                carrierManager->AssignJob(job);
            }
        }

        // ─── Periodic validation ─────────────────────────────────────────
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
                std::map<World::ResourceType, int>::iterator it = b->inventory.find(type);
                if (it != b->inventory.end())
                    total += it->second;
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

    void EconomyManager::AddBuilding(World::Building* building) {
        m_buildings.push_back(building);
        if (building->state == World::State_Finished)
            m_registry.Register(building);
    }

    bool EconomyManager::HasBuilding(World::BuildingType type) const {
        for (size_t i = 0; i < m_buildings.size(); ++i) {
            if (m_buildings[i]->type == type) {
                return true;
            }
        }
        return false;
    }
}
