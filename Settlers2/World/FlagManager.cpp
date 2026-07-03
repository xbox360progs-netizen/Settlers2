#include "stdafx.h"
#include "FlagManager.h"
#include "Road.h"
#include "CarrierManager.h"
#include "RoadManager.h"

namespace World {

    uint32_t FlagManager::s_nextId = 1;

    FlagManager::FlagManager()
    {
    }

    FlagManager::~FlagManager()
    {
        Clear();
    }

    Flag* FlagManager::CreateFlag(int x, int y)
    {
        uint32_t id = s_nextId++;
        Flag* flag = new Flag(x, y, id);
        m_flags.push_back(flag);
        flag->handle = m_handleRegistry.Register<Flag>(flag);
        return flag;
    }

    FlagHandle FlagManager::CreateFlagHandle(int x, int y)
    {
        Flag* flag = CreateFlag(x, y);
        return flag ? flag->handle : FlagHandle();
    }

    Flag* FlagManager::GetFlagAt(int x, int y) const
    {
        for (size_t i = 0; i < m_flags.size(); ++i) {
            if (m_flags[i]->state != Active) continue;
            if (m_flags[i]->pos.x == x && m_flags[i]->pos.y == y) {
                return m_flags[i];
            }
        }
        return NULL;
    }

    Flag* FlagManager::GetFlagByIndex(uint32_t idx) const
    {
        return static_cast<Flag*>(m_handleRegistry.UnsafeResolveByIndex(idx));
    }

    Flag* FlagManager::ResolveFlag(FlagHandle h) const
    {
        return m_handleRegistry.Resolve<Flag>(h);
    }

    FlagHandle FlagManager::GetFlagHandle(Flag* flag) const
    {
        if (!flag) return FlagHandle();
        return m_handleRegistry.FindHandle<Flag>(flag);
    }

    Flag* FlagManager::GetFlagById(uint32_t id) const
    {
        for (size_t i = 0; i < m_flags.size(); ++i) {
            if (m_flags[i]->state != Active) continue;
            if (m_flags[i]->id == id) {
                return m_flags[i];
            }
        }
        return NULL;
    }

    void FlagManager::RemoveFlag(Flag* flag)
    {
        if (!flag) return;

        // Unregister handle first
        FlagHandle h = m_handleRegistry.FindHandle<Flag>(flag);
        if (h.IsValid()) m_handleRegistry.Unregister<Flag>(h);

        // Disown the building reference
        if (flag->building) {
            flag->building->connectedFlag = NULL;
            flag->building = NULL;
        }
        flag->hasBuilding = false;

        flag->roads.clear();

        for (size_t i = 0; i < m_flags.size(); ++i) {
            if (m_flags[i] == flag) {
                delete m_flags[i];
                m_flags.erase(m_flags.begin() + i);
                return;
            }
        }
    }

    void FlagManager::MarkForDeletion(Flag* flag) {
        if (flag) flag->state = PendingDelete;
    }

    bool FlagManager::CanDestroy(Flag* flag, CarrierManager* cm, RoadManager* rm) const {
        if (!flag) return true;
        return !cm->IsFlagInUse(flag) && !rm->HasRoadsConnectedToFlag(flag);
    }

    void FlagManager::RemoveFlag(FlagHandle h)
    {
        Flag* flag = m_handleRegistry.Resolve<Flag>(h);
        if (flag) {
            m_handleRegistry.Unregister<Flag>(h);
            RemoveFlag(flag);
        }
    }

    void FlagManager::RemoveFlagAt(int x, int y)
    {
        Flag* flag = GetFlagAt(x, y);
        if (flag) RemoveFlag(flag);
    }

    void FlagManager::Clear()
    {
        for (size_t i = 0; i < m_flags.size(); ++i) {
            delete m_flags[i];
        }
        m_flags.clear();
        m_handleRegistry.Clear();
    }

    std::vector<std::pair<int,int>> FlagManager::GetFlagPairs() const
    {
        std::vector<std::pair<int,int>> pairs;
        pairs.reserve(m_flags.size());
        for (size_t i = 0; i < m_flags.size(); ++i) {
            if (m_flags[i]->state != Active) continue;
            pairs.push_back(std::make_pair(m_flags[i]->pos.x, m_flags[i]->pos.y));
        }
        return pairs;
    }

    std::vector<FlagData> FlagManager::GetFlagData() const
    {
        std::vector<FlagData> data;
        data.reserve(m_flags.size());
        for (size_t i = 0; i < m_flags.size(); ++i) {
            Flag* f = m_flags[i];
            if (f->state != Active) continue;
            FlagData fd;
            fd.x = f->pos.x;
            fd.y = f->pos.y;
            fd.id = f->id;
            fd.type = f->type;
            fd.pendingBuilding = f->pendingBuilding;
            fd.hasBuilding = f->hasBuilding;
            data.push_back(fd);
        }
        return data;
    }

    void FlagManager::LoadFromData(const std::vector<FlagData>& data)
    {
        Clear();
        uint32_t maxId = 0;
        for (size_t i = 0; i < data.size(); ++i) {
            const FlagData& fd = data[i];
            uint32_t loadId = fd.id;
            if (loadId == 0) {
                loadId = s_nextId++;
            }
            Flag* flag = new Flag(fd.x, fd.y, loadId);
            flag->type = fd.type;
            flag->pendingBuilding = fd.pendingBuilding;
            flag->hasBuilding = fd.hasBuilding;
            m_flags.push_back(flag);
            flag->handle = m_handleRegistry.Register<Flag>(flag);
            if (loadId > maxId) maxId = loadId;
        }
        if (maxId >= s_nextId) {
            s_nextId = maxId + 1;
        }
    }

}
