#pragma once
#include <vector>
#include <stdint.h>
#include "Flag.h"

namespace World {

    class FlagManager {
    public:
        FlagManager();
        ~FlagManager();

        Flag* CreateFlag(int x, int y);
        Flag* GetFlagAt(int x, int y) const;
        Flag* GetFlagById(uint32_t id) const;
        void RemoveFlag(Flag* flag);
        void RemoveFlagAt(int x, int y);
        void Clear();

        size_t GetCount() const { return m_flags.size(); }
        Flag* GetFlag(size_t index) const { return (index < m_flags.size()) ? m_flags[index] : NULL; }

        // Legacy pair format (position only, no ID)
        std::vector<std::pair<int,int>> GetFlagPairs() const;
        void LoadFromPairs(const std::vector<std::pair<int,int>>& pairs);

        // New data format with IDs
        std::vector<FlagData> GetFlagData() const;
        void LoadFromData(const std::vector<FlagData>& data);
        void SetNextId(uint32_t id) { s_nextId = id; }
        uint32_t GetNextId() const { return s_nextId; }

        // BFS pathfinding over flag neighbor graph
        std::vector<Flag*> FindFlagPath(Flag* start, Flag* goal) const;

        static uint32_t s_nextId;

    private:
        std::vector<Flag*> m_flags;
    };

}
