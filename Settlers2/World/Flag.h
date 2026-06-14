#pragma once
#include <vector>
#include <stdint.h>
#include "../Core/Vector2i.h"
#include "ResourceNode.h"
#include "Components/Building.h"
#include "ObjectState.h"
#include "Handle.h"

namespace World {
    static const uint32_t INVALID_FLAG_ID = 0xFFFFFFFF;

    class Building;
    struct Road;

    enum FlagType {
        FLAG_NORMAL,
        FLAG_BUILDING,
        FLAG_WAREHOUSE,
        FLAG_MILITARY
    };

    struct FlagData {
        int x, y;
        uint32_t id;
        FlagType type;
        BuildingType pendingBuilding;
        bool hasBuilding;
    };

    struct ResourceSlot {
        ResourceType type;
        int amount;
        int reserved; // committed to pending TransportJobs, not available for new requests
        uint32_t destFlagId; // ID of ultimate destination flag (0 = no routing, safe across flag deletion)

        ResourceSlot() : type(ResourceType_None), amount(0), reserved(0), destFlagId(INVALID_FLAG_ID) {}
    };

    class Flag {
    public:
        uint32_t id;
        Vector2i pos;
        FlagType type;
        Building* building;
        ResourceSlot slots[8];
        std::vector<Road*> roads;
        BuildingType pendingBuilding;
        bool hasBuilding;
        ObjectState state;

        Flag(int x, int y, uint32_t id)
            : id(id), type(FLAG_NORMAL), building(NULL), pendingBuilding(static_cast<BuildingType>(0)), hasBuilding(false), state(Active)
        {
            pos.x = x;
            pos.y = y;
        }

        int FindSlot(ResourceType type, uint32_t destFlagId = INVALID_FLAG_ID) const {
            for (int i = 0; i < 8; ++i) {
                if (slots[i].type == type) {
                    if (destFlagId == INVALID_FLAG_ID || slots[i].destFlagId == destFlagId)
                        return i;
                }
            }
            return -1;
        }

        int FindEmptySlot() const {
            for (int i = 0; i < 8; ++i) {
                if (slots[i].type == ResourceType_None)
                    return i;
            }
            return -1;
        }

        int GetAvailable(ResourceType type) const {
            int total = 0;
            for (int i = 0; i < 8; ++i) {
                if (slots[i].type == type)
                    total += slots[i].amount - slots[i].reserved;
            }
            return total;
        }

        bool Reserve(ResourceType type, int amount, uint32_t destFlagId = INVALID_FLAG_ID) {
            int idx = FindSlot(type, destFlagId);
            if (idx < 0) return false;
            if (slots[idx].amount - slots[idx].reserved < amount) return false;
            slots[idx].reserved += amount;
            return true;
        }

        void Unreserve(ResourceType type, int amount, uint32_t destFlagId = INVALID_FLAG_ID) {
            int idx = FindSlot(type, destFlagId);
            if (idx >= 0) {
                slots[idx].reserved -= amount;
                if (slots[idx].reserved < 0) slots[idx].reserved = 0;
            }
        }

        bool AddResource(ResourceType type, int amount, uint32_t destFlagId = INVALID_FLAG_ID) {
            int idx = FindSlot(type, destFlagId);
            if (idx >= 0) {
                slots[idx].amount += amount;
                return true;
            }
            idx = FindEmptySlot();
            if (idx >= 0) {
                slots[idx].type = type;
                slots[idx].amount = amount;
                slots[idx].reserved = 0;
                slots[idx].destFlagId = destFlagId;
                return true;
            }
            return false;
        }

        bool RemoveResource(ResourceType type, int amount, uint32_t destFlagId = INVALID_FLAG_ID) {
            int idx = FindSlot(type, destFlagId);
            if (idx < 0) return false;
            if (slots[idx].amount - slots[idx].reserved < amount) return false;
            slots[idx].amount -= amount;
            if (slots[idx].amount <= 0) {
                slots[idx].type = ResourceType_None;
                slots[idx].amount = 0;
                slots[idx].reserved = 0;
                slots[idx].destFlagId = INVALID_FLAG_ID;
            }
            return true;
        }

        void CommitPickup(ResourceType type, int amount, uint32_t destFlagId = INVALID_FLAG_ID) {
            int idx = FindSlot(type, destFlagId);
            if (idx < 0) return;
            if (amount <= 0) return;
            int toRemove = (amount < slots[idx].amount) ? amount : slots[idx].amount;
            if (toRemove <= 0) return;
            if (slots[idx].reserved >= toRemove) {
                slots[idx].reserved -= toRemove;
            } else {
                slots[idx].reserved = 0;
            }
            slots[idx].amount -= toRemove;
            if (slots[idx].amount <= 0) {
                slots[idx].type = ResourceType_None;
                slots[idx].amount = 0;
                slots[idx].reserved = 0;
                slots[idx].destFlagId = INVALID_FLAG_ID;
            }
        }
    };

    class Flag;
    typedef Handle<Flag> FlagHandle;
}
