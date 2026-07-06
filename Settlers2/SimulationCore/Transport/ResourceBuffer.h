#pragma once
#include "../Core/ResourceTypes.h"

namespace World {

static const int kNodeBufferSlots = 8;

struct BufferSlot {
    ResourceType type;
    int amount;
};

// Passive storage container for TransportNode.
// No logic, no decisions, no knowledge of buildings or transport.
// Only: Add, Remove, Has, Count.
struct ResourceBuffer {
    BufferSlot slots[kNodeBufferSlots];
    int slotCount;

    ResourceBuffer();

    void Add(ResourceType resource, int amount);
    int Remove(ResourceType resource, int amount);
    bool Has(ResourceType resource, int amount) const;
    int Count(ResourceType resource) const;
    int FindEmptySlot() const;

private:
    int FindSlot(ResourceType resource) const;
};

} // namespace World
