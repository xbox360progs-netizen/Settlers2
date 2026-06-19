#include "stdafx.h"
#include "CargoManager.h"
#include "StorehouseManager.h"
#include "FlagManager.h"
#include "DemandManager.h"

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
                c->ticket = NULL;
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

        for (int i = 0; i < MAX_WORLD_CARGO; ++i) {
            Cargo* c = &m_pool[i];
            if (c->id == 0) continue;
            if (c->state != Cargo_OnFlag) continue;
            if (!c->ticket) continue;

            // Reassign cancelled tickets
            if (c->ticket->state == Ticket_Cancelled || !c->ticket->demand) {
                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[Cargo] Reassign id=%u type=%s oldTicket=%u\n",
                    c->id, ResourceTypeToString(c->type), c->ticket->id);
                OutputDebugStringA(buf);
                dm->ReleaseTicket(c->ticket);
                c->ticket = NULL;
                DemandTicket* newTicket = dm->Reserve(c->type);
                if (newTicket) {
                    c->ticket = newTicket;
                    _snprintf(buf, sizeof(buf),
                        "[Cargo] Reassigned id=%u type=%s ticket=%u\n",
                        c->id, ResourceTypeToString(c->type), newTicket->id);
                    OutputDebugStringA(buf);
                }
                continue;
            }

            // Deliver if cargo has reached its destination flag
            if (c->ticket->state == Ticket_Active && c->ticket->demand &&
                c->ticket->demand->targetFlag.index == c->currentFlag.index &&
                c->ticket->demand->targetFlag.generation == c->currentFlag.generation)
            {
                Flag* flag = fm->ResolveFlag(c->currentFlag);
                if (flag) {
                    if (!flag->AddResource(c->type, c->amount)) {
                        continue; // all 8 slots full — retry next frame
                    }
                }

                DemandTicket* ticket = c->ticket;
                uint32_t id = c->id;
                dm->Deliver(ticket);
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
