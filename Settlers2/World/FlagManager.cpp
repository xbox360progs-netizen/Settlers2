#include "stdafx.h"
#include "FlagManager.h"
#include <queue>
#include <map>
#include <algorithm>

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
        return flag;
    }

    Flag* FlagManager::GetFlagAt(int x, int y) const
    {
        for (size_t i = 0; i < m_flags.size(); ++i) {
            if (m_flags[i]->pos.x == x && m_flags[i]->pos.y == y) {
                return m_flags[i];
            }
        }
        return NULL;
    }

    Flag* FlagManager::GetFlagById(uint32_t id) const
    {
        for (size_t i = 0; i < m_flags.size(); ++i) {
            if (m_flags[i]->id == id) {
                return m_flags[i];
            }
        }
        return NULL;
    }

    void FlagManager::RemoveFlag(Flag* flag)
    {
        // Remove from neighbors' neighbor lists
        for (size_t ni = 0; ni < flag->neighbors.size(); ++ni) {
            Flag* neighbor = flag->neighbors[ni];
            if (neighbor) {
                for (size_t nn = 0; nn < neighbor->neighbors.size(); ++nn) {
                    if (neighbor->neighbors[nn] == flag) {
                        neighbor->neighbors.erase(neighbor->neighbors.begin() + nn);
                        break;
                    }
                }
            }
        }
        flag->neighbors.clear();

        for (size_t i = 0; i < m_flags.size(); ++i) {
            if (m_flags[i] == flag) {
                delete m_flags[i];
                m_flags.erase(m_flags.begin() + i);
                return;
            }
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
    }

    std::vector<std::pair<int,int>> FlagManager::GetFlagPairs() const
    {
        std::vector<std::pair<int,int>> pairs;
        pairs.reserve(m_flags.size());
        for (size_t i = 0; i < m_flags.size(); ++i) {
            pairs.push_back(std::make_pair(m_flags[i]->pos.x, m_flags[i]->pos.y));
        }
        return pairs;
    }

    std::vector<Flag*> FlagManager::FindFlagPath(Flag* start, Flag* goal) const
    {
        std::vector<Flag*> path;
        if (!start || !goal || start == goal) {
            if (start == goal && start) path.push_back(start);
            return path;
        }

        std::queue<Flag*> q;
        std::map<uint32_t, Flag*> parent;
        std::map<uint32_t, bool> visited;

        q.push(start);
        visited[start->id] = true;
        parent[start->id] = NULL;

        while (!q.empty()) {
            Flag* current = q.front();
            q.pop();

            if (current == goal) {
                // Reconstruct path
                Flag* node = goal;
                while (node) {
                    path.push_back(node);
                    node = parent[node->id];
                }
                std::reverse(path.begin(), path.end());
                return path;
            }

            for (size_t i = 0; i < current->neighbors.size(); ++i) {
                Flag* next = current->neighbors[i];
                if (next && !visited[next->id]) {
                    visited[next->id] = true;
                    parent[next->id] = current;
                    q.push(next);
                }
            }
        }

        // No path found
        return path;
    }

    void FlagManager::LoadFromPairs(const std::vector<std::pair<int,int>>& pairs)
    {
        Clear();
        for (size_t i = 0; i < pairs.size(); ++i) {
            CreateFlag(pairs[i].first, pairs[i].second);
        }
    }

    std::vector<FlagData> FlagManager::GetFlagData() const
    {
        std::vector<FlagData> data;
        data.reserve(m_flags.size());
        for (size_t i = 0; i < m_flags.size(); ++i) {
            Flag* f = m_flags[i];
            FlagData fd;
            fd.x = f->pos.x;
            fd.y = f->pos.y;
            fd.id = f->id;
            fd.type = f->type;
            fd.pendingBuilding = f->pendingBuilding;
            fd.hasBuilding = f->hasBuilding;
            // Save neighbor IDs
            fd.neighborIds.reserve(f->neighbors.size());
            for (size_t ni = 0; ni < f->neighbors.size(); ++ni) {
                fd.neighborIds.push_back(f->neighbors[ni]->id);
            }
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
            Flag* flag = new Flag(fd.x, fd.y, fd.id);
            flag->type = fd.type;
            flag->pendingBuilding = fd.pendingBuilding;
            flag->hasBuilding = fd.hasBuilding;
            m_flags.push_back(flag);
            if (fd.id > maxId) maxId = fd.id;
        }
        // Advance next ID past all loaded IDs
        if (maxId >= s_nextId) {
            s_nextId = maxId + 1;
        }
        // Restore neighbor graph (second pass — all flags exist)
        for (size_t i = 0; i < data.size(); ++i) {
            Flag* flag = m_flags[i];
            const FlagData& fd = data[i];
            for (size_t ni = 0; ni < fd.neighborIds.size(); ++ni) {
                Flag* neighbor = GetFlagById(fd.neighborIds[ni]);
                if (neighbor) {
                    flag->neighbors.push_back(neighbor);
                }
            }
        }
    }

}
