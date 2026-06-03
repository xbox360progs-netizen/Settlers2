#pragma once
#include <vector>
#include "../Core/Vector2i.h"
#include "ResourceNode.h"

namespace World {
    class Road;

    struct ResourceSlot {
        ResourceType type;
        int amount;
        int reserved; // committed to pending TransportJobs, not available for new requests

        ResourceSlot() : type(ResourceType_None), amount(0), reserved(0) {}
    };

    class Flag {
    public:
        Vector2i pos;
        ResourceSlot slots[8];
        std::vector<Road*> roads;
        std::vector<Flag*> neighbors;

        Flag(int x, int y) {
            pos.x = x;
            pos.y = y;
        }

        int FindSlot(ResourceType type) const {
            for (int i = 0; i < 8; ++i) {
                if (slots[i].type == type)
                    return i;
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
            int idx = FindSlot(type);
            if (idx < 0) return 0;
            return slots[idx].amount - slots[idx].reserved;
        }

        bool Reserve(ResourceType type, int amount) {
            int idx = FindSlot(type);
            if (idx < 0) return false;
            if (slots[idx].amount - slots[idx].reserved < amount) return false;
            slots[idx].reserved += amount;
            return true;
        }

        void Unreserve(ResourceType type, int amount) {
            int idx = FindSlot(type);
            if (idx >= 0) {
                slots[idx].reserved -= amount;
                if (slots[idx].reserved < 0) slots[idx].reserved = 0;
            }
        }

        bool AddResource(ResourceType type, int amount) {
            int idx = FindSlot(type);
            if (idx >= 0) {
                slots[idx].amount += amount;
                return true;
            }
            idx = FindEmptySlot();
            if (idx >= 0) {
                slots[idx].type = type;
                slots[idx].amount = amount;
                slots[idx].reserved = 0;
                return true;
            }
            return false;
        }

        bool RemoveResource(ResourceType type, int amount) {
            int idx = FindSlot(type);
            if (idx < 0) return false;
            if (slots[idx].amount - slots[idx].reserved < amount) return false;
            slots[idx].amount -= amount;
            if (slots[idx].amount <= 0) {
                slots[idx].type = ResourceType_None;
                slots[idx].amount = 0;
                slots[idx].reserved = 0;
            }
            return true;
        }

        void CommitPickup(ResourceType type, int amount) {
            int idx = FindSlot(type);
            if (idx < 0) return;
            if (slots[idx].reserved > 0) {
                int unreserve = (amount < slots[idx].reserved) ? amount : slots[idx].reserved;
                slots[idx].reserved -= unreserve;
                slots[idx].amount -= unreserve;
                if (slots[idx].amount <= 0) {
                    slots[idx].type = ResourceType_None;
                    slots[idx].amount = 0;
                    slots[idx].reserved = 0;
                }
            }
        }
    };
}
