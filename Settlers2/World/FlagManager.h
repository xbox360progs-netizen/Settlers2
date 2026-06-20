#pragma once
#include <vector>
#include <stdint.h>
#include "Flag.h"

namespace World {
    class CarrierManager;
    class TransportJobManager;
    class RoadManager;

    class FlagManager {
    public:
        FlagManager();
        ~FlagManager();

        Flag* CreateFlag(int x, int y);
    FlagHandle CreateFlagHandle(int x, int y);
        Flag* GetFlagAt(int x, int y) const;
        Flag* GetFlagById(uint32_t id) const;
    Flag* GetFlagByIndex(uint32_t idx) const; // dense handle-index lookup (for routing table)
    Flag* ResolveFlag(FlagHandle h) const;
    FlagHandle GetFlagHandle(Flag* flag) const;
    void RemoveFlag(Flag* flag);
    void RemoveFlagAt(int x, int y);
    void RemoveFlag(FlagHandle h);
    void MarkForDeletion(Flag* flag);
    bool CanDestroy(Flag* flag, CarrierManager* cm, TransportJobManager* jm, RoadManager* rm) const;
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

        static uint32_t s_nextId;

    HandleRegistry& GetHandleRegistry() { return m_handleRegistry; }

    private:
        std::vector<Flag*> m_flags;
        HandleRegistry m_handleRegistry;
    };

}
