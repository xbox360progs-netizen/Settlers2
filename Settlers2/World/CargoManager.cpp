#include "stdafx.h"
#include "CargoManager.h"
#include "StorehouseManager.h"
#include "FlagManager.h"
#include "DemandManager.h"
#include "TransportTask.h"

namespace World {

    CargoManager::CargoManager()
        : m_activeCount(0), m_poolCount(0), m_nextId(1), m_storehouseManager(NULL)
    {
        for (int i = 0; i < MAX_WORLD_CARGO; ++i) {
            m_pool[i].id = 0;
        }
    }

    Cargo* CargoManager::Allocate(ResourceType type, uint32_t amount, Handle<Flag> onFlag)
    {
        for (int i = 0; i < MAX_WORLD_CARGO; ++i) {
            if (m_pool[i].id == 0) {
                Cargo* c = &m_pool[i];
                c->id = m_nextId++;
                c->type = type;
                c->amount = amount;
                c->state = Cargo_OnFlag;
                c->currentFlag = onFlag;
                c->ownerTask = NULL;                // Phase 8.2 — clear before wiring
                ++m_poolCount;
                m_activeIndices[m_activeCount++] = i;

                if (m_storehouseManager) {
                    m_storehouseManager->ModifyTransitResource(type, (int)amount);
                }

                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[Cargo] Allocate id=%u %s amount=%u onFlag(handleIdx=%u)\n",
                    c->id, ResourceTypeToString(type), amount, onFlag.index);
                OutputDebugStringA(buf);

                return c;
            }
        }
        return NULL;
    }

    void CargoManager::Release(uint32_t id)
    {
        for (int i = 0; i < MAX_WORLD_CARGO; ++i) {
            if (m_pool[i].id == id) {
                if (m_storehouseManager) {
                    m_storehouseManager->ModifyTransitResource(m_pool[i].type, -(int)m_pool[i].amount);
                }

                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[Cargo] Release id=%u %s\n",
                    id, ResourceTypeToString(m_pool[i].type));
                OutputDebugStringA(buf);

                m_pool[i].id = 0;
                m_pool[i].type = ResourceType_None;
                m_pool[i].state = Cargo_Delivered;

                // Swap-and-pop from active indices
                for (int j = 0; j < m_activeCount; ++j) {
                    if (m_activeIndices[j] == i) {
                        m_activeIndices[j] = m_activeIndices[--m_activeCount];
                        break;
                    }
                }
                --m_poolCount;
                return;
            }
        }
    }

    void CargoManager::ReleaseAllForFlag(Handle<Flag> flag)
    {
        for (int i = 0; i < m_activeCount; ) {
            int poolIdx = m_activeIndices[i];
            if (m_pool[poolIdx].id != 0 && m_pool[poolIdx].currentFlag.index == flag.index) {
                if (m_storehouseManager) {
                    m_storehouseManager->ModifyTransitResource(m_pool[poolIdx].type, -(int)m_pool[poolIdx].amount);
                }
                m_pool[poolIdx].id = 0;
                m_pool[poolIdx].type = ResourceType_None;
                m_pool[poolIdx].state = Cargo_Delivered;
                m_pool[poolIdx].ownerTask = NULL;
                m_activeIndices[i] = m_activeIndices[--m_activeCount];
                --m_poolCount;
            } else {
                ++i;
            }
        }
    }

    Cargo* CargoManager::GetById(uint32_t id) const
    {
        for (int i = 0; i < MAX_WORLD_CARGO; ++i) {
            if (m_pool[i].id == id)
                return const_cast<Cargo*>(&m_pool[i]);
        }
        return NULL;
    }

    Cargo* CargoManager::GetByIndex(int i) const
    {
        int count = 0;
        for (int j = 0; j < MAX_WORLD_CARGO; ++j) {
            if (m_pool[j].id != 0) {
                if (count == i) return const_cast<Cargo*>(&m_pool[j]);
                ++count;
            }
        }
        return NULL;
    }

    int CargoManager::CountCargoOnFlag(Handle<Flag> flag) const
    {
        int count = 0;
        for (int i = 0; i < m_activeCount; ++i) {
            int poolIdx = m_activeIndices[i];
            if (m_pool[poolIdx].state == Cargo_OnFlag &&
                m_pool[poolIdx].currentFlag.index == flag.index &&
                m_pool[poolIdx].currentFlag.generation == flag.generation)
            {
                ++count;
            }
        }
        return count;
    }

    void CargoManager::CheckDeliveries(DemandManager* dm, FlagManager* fm)
    {
        if (!dm || !fm) return;

        static int s_unassignedCount = 0;   // Phase 8.3A — throttle unassigned cargo log
        s_unassignedCount = 0;

        for (int i = 0; i < MAX_WORLD_CARGO; ++i) {
            Cargo* c = &m_pool[i];
            if (c->id == 0) continue;
            if (c->state != Cargo_OnFlag) continue;

            // Phase 8.3A — log unassigned cargo (without ownerTask)
            if (!c->ownerTask && s_unassignedCount < 3) {
                char un[256];
                _snprintf(un, sizeof(un),
                    "[Transport] Unassigned cargo id=%u type=%s flag=%u\n",
                    c->id, ResourceTypeToString(c->type), c->currentFlag.index);
                OutputDebugStringA(un);
                s_unassignedCount++;
            }

            if (!c->ownerTask) continue;                        // Phase 8.2 — task, not ticket

            // Phase 8.2 — check task state; discard cancelled/delivered tasks
            if (c->ownerTask->state == TTS_Cancelled || c->ownerTask->state == TTS_Delivered) {
                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[Cargo] Detach cancelled/delivered task id=%u type=%s\n",
                    c->id, ResourceTypeToString(c->type));
                OutputDebugStringA(buf);
                c->ownerTask = NULL;
                continue;
            }

            // Deliver if cargo has reached its destination flag
            Flag* flag = fm->ResolveFlag(c->currentFlag);
            bool atDest = (flag && (c->ownerTask->state == TTS_Moving || c->ownerTask->state == TTS_WaitingAtSource) &&
                           flag->id == c->ownerTask->targetFlag);
            {
                char dbg[256]; _snprintf(dbg, sizeof(dbg), "[CheckDeliveries] cargo=%u type=%s flag=%u task=%u taskState=%u targetFlag=%u atDest=%d\n", c->id, ResourceTypeToString(c->type), flag ? flag->id : 0, c->ownerTask->id, c->ownerTask->state, c->ownerTask->targetFlag, atDest ? 1 : 0); OutputDebugStringA(dbg);
            }
            if (atDest)
            {
                if (!flag->AddResource(c->type, c->amount)) {
                    continue; // all 8 slots full — retry next frame
                }

                // Phase 8.3A — shadow completion: compare expected vs actual destination
                FlagId expectedDest = c->ownerTask->route.flags[c->ownerTask->route.count - 1];
                FlagId actualDest = c->ownerTask->targetFlag;
                char mig[256];
                _snprintf(mig, sizeof(mig),
                    "[MIGRATION] task=%u ticket=%u type=%s expected=flag%u actual=flag%u %s\n",
                    c->ownerTask->id, c->ownerTask->observerTicketId,
                    ResourceTypeToString(c->type), expectedDest, actualDest,
                    (expectedDest == actualDest) ? "OK" : "MISMATCH");
                OutputDebugStringA(mig);

                // Phase 8.2 — delivery accounting via task→ticket observer link
                if (c->ownerTask->observerTicketId > 0) {
                    DemandTicket* ticket = dm->GetTicket(c->ownerTask->observerTicketId);
                    if (ticket) {
                        dm->Deliver(ticket);
                    }
                }

                uint32_t id = c->id;
                c->ownerTask = NULL;
                Release(id);
            }
        }
    }

    void CargoManager::Clear()
    {
        for (int i = 0; i < MAX_WORLD_CARGO; ++i) {
            m_pool[i].id = 0;
            m_pool[i].type = ResourceType_None;
            m_pool[i].state = Cargo_Delivered;
        }
        m_activeCount = 0;
        m_poolCount = 0;
    }

}
