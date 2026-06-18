#include "stdafx.h"
#include "CargoManager.h"

namespace World {

    CargoManager::CargoManager()
        : m_nextId(1)
    {
    }

    Cargo* CargoManager::Allocate(ResourceType type, uint32_t amount, Handle<Flag> onFlag)
    {
        Cargo* c = new Cargo();
        c->id = m_nextId++;
        c->type = type;
        c->amount = amount;
        c->state = Cargo_OnFlag;
        c->currentFlag = onFlag;
        c->ticket = NULL;
        m_cargo.push_back(c);

        char buf[256];
        _snprintf(buf, sizeof(buf),
            "[Cargo] Allocate id=%u %s amount=%u onFlag(handleIdx=%u)\n",
            c->id, ResourceTypeToString(type), amount, onFlag.index);
        OutputDebugStringA(buf);

        return c;
    }

    void CargoManager::Release(uint32_t id)
    {
        for (size_t i = 0; i < m_cargo.size(); ++i) {
            if (m_cargo[i]->id == id) {
                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[Cargo] Release id=%u %s\n",
                    id, ResourceTypeToString(m_cargo[i]->type));
                OutputDebugStringA(buf);

                delete m_cargo[i];
                m_cargo.erase(m_cargo.begin() + i);
                return;
            }
        }
    }

    void CargoManager::ReleaseAllForFlag(Handle<Flag> flag)
    {
        for (size_t i = 0; i < m_cargo.size(); ) {
            if (m_cargo[i]->currentFlag.index == flag.index) {
                delete m_cargo[i];
                m_cargo.erase(m_cargo.begin() + i);
            } else {
                ++i;
            }
        }
    }

    Cargo* CargoManager::GetById(uint32_t id) const
    {
        for (size_t i = 0; i < m_cargo.size(); ++i) {
            if (m_cargo[i]->id == id)
                return m_cargo[i];
        }
        return NULL;
    }

    Cargo* CargoManager::GetByIndex(size_t i) const
    {
        return (i < m_cargo.size()) ? m_cargo[i] : NULL;
    }

    void CargoManager::Clear()
    {
        for (size_t i = 0; i < m_cargo.size(); ++i)
            delete m_cargo[i];
        m_cargo.clear();
    }

    Cargo* CargoManager::FindFreeSlot()
    {
        for (size_t i = 0; i < m_cargo.size(); ++i) {
            if (m_cargo[i]->state == Cargo_Delivered)
                return m_cargo[i];
        }
        return NULL;
    }

}
