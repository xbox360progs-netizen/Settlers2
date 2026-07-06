#include "TransportNode.h"

namespace World {

// ─── ResourceBuffer ────────────────────────────────────────────────────

ResourceBuffer::ResourceBuffer()
    : slotCount(0)
{
    for (int i = 0; i < kNodeBufferSlots; ++i) {
        slots[i].type = ResourceType_None;
        slots[i].amount = 0;
    }
}

int ResourceBuffer::FindSlot(ResourceType resource) const
{
    for (int i = 0; i < slotCount; ++i) {
        if (slots[i].type == resource) return i;
    }
    return -1;
}

int ResourceBuffer::FindEmptySlot() const
{
    for (int i = 0; i < kNodeBufferSlots; ++i) {
        if (slots[i].type == ResourceType_None || slots[i].amount == 0) return i;
    }
    return -1;
}

void ResourceBuffer::Add(ResourceType resource, int amount)
{
    if (resource == ResourceType_None || amount <= 0) return;

    int idx = FindSlot(resource);
    if (idx >= 0) {
        slots[idx].amount += amount;
        return;
    }

    idx = FindEmptySlot();
    if (idx < 0) return; // buffer full

    slots[idx].type = resource;
    slots[idx].amount = amount;
    if (idx >= slotCount) slotCount = idx + 1;
}

int ResourceBuffer::Remove(ResourceType resource, int amount)
{
    if (resource == ResourceType_None || amount <= 0) return 0;

    int idx = FindSlot(resource);
    if (idx < 0) return 0;

    int removed = (amount < slots[idx].amount) ? amount : slots[idx].amount;
    slots[idx].amount -= removed;

    if (slots[idx].amount == 0) {
        slots[idx].type = ResourceType_None;
        // Compact if last slot
        if (idx == slotCount - 1) {
            --slotCount;
            while (slotCount > 0 && slots[slotCount - 1].type == ResourceType_None) {
                --slotCount;
            }
        }
    }

    return removed;
}

bool ResourceBuffer::Has(ResourceType resource, int amount) const
{
    return Count(resource) >= amount;
}

int ResourceBuffer::Count(ResourceType resource) const
{
    int idx = FindSlot(resource);
    return (idx >= 0) ? slots[idx].amount : 0;
}

// ─── TransportNode ─────────────────────────────────────────────────────

TransportNode::TransportNode()
    : id(0)
    , attachmentCount(0)
    , outgoingCount(0)
{
}

int TransportNode::FindAttachment(uint8_t buildingId) const
{
    for (int i = 0; i < attachmentCount; ++i) {
        if (attachments[i].buildingId == buildingId) return i;
    }
    return -1;
}

int TransportNode::FindDemand(ResourceType resource) const
{
    for (int i = 0; i < kMaxNodeDemands; ++i) {
        if (pendingDemand[i].active && pendingDemand[i].resource == resource) return i;
    }
    return -1;
}

void TransportNode::AttachBuilding(uint8_t buildingId, const ResourceType inputs[], int inputCount, AttachmentRole role)
{
    if (attachmentCount >= kMaxNodeAttachments) return;
    if (FindAttachment(buildingId) >= 0) return; // already attached

    BuildingAttachment& a = attachments[attachmentCount];
    a.buildingId = buildingId;
    a.role = role;
    a.inputCount = (inputCount < kMaxNodeInputs) ? inputCount : kMaxNodeInputs;
    for (int i = 0; i < a.inputCount; ++i) {
        a.inputs[i] = inputs[i];
    }
    ++attachmentCount;
}

void TransportNode::DetachBuilding(uint8_t buildingId)
{
    int idx = FindAttachment(buildingId);
    if (idx < 0) return;

    // Compact by swapping with last
    --attachmentCount;
    if (idx < attachmentCount) {
        attachments[idx] = attachments[attachmentCount];
    }
    // Clear the vacated slot
    attachments[attachmentCount].buildingId = 0;
    attachments[attachmentCount].role = AR_Consumer;
    attachments[attachmentCount].inputCount = 0;
}

void TransportNode::ReceiveCargo(ResourceType resource, int amount)
{
    buffer.Add(resource, amount);
}

void TransportNode::ReceiveExport(ResourceType resource, int amount)
{
    buffer.Add(resource, amount);
}

bool TransportNode::TakeForBuilding(uint8_t buildingId, ResourceType resource, int amount)
{
    (void)buildingId;
    if (!buffer.Has(resource, amount)) return false;
    buffer.Remove(resource, amount);
    return true;
}

int TransportNode::GetBufferAmount(ResourceType resource) const
{
    return buffer.Count(resource);
}

bool TransportNode::HasCapacity(ResourceType resource, int amount) const
{
    if (amount <= 0) return false;
    int current = buffer.Count(resource);
    if (current > 0) {
        return current + amount <= kNodeBufferSlots;
    }
    // New resource type — need an empty slot
    int empty = buffer.FindEmptySlot();
    return empty >= 0 && amount <= kNodeBufferSlots;
}

bool TransportNode::HasDemandFor(ResourceType resource) const
{
    // A node has demand if any Consumer/ProducerConsumer attachment needs this resource
    // and the buffer doesn't have enough
    for (int i = 0; i < attachmentCount; ++i) {
        if (attachments[i].role == AR_Producer) continue;
        for (int j = 0; j < attachments[i].inputCount; ++j) {
            if (attachments[i].inputs[j] == resource && buffer.Count(resource) < 1) {
                return true;
            }
        }
    }
    return false;
}

void TransportNode::Tick()
{
    // Step 2-3: Evaluate deficits against attachments, populate pendingDemand
    for (int i = 0; i < kMaxNodeDemands; ++i) {
        pendingDemand[i].active = false;
    }
    int demandCount = 0;

    for (int i = 0; i < attachmentCount && demandCount < kMaxNodeDemands; ++i) {
        if (attachments[i].role == AR_Producer) continue;

        for (int j = 0; j < attachments[i].inputCount; ++j) {
            ResourceType r = attachments[i].inputs[j];
            if (r == ResourceType_None) continue;
            if (buffer.Count(r) < 1 && FindDemand(r) < 0) {
                pendingDemand[demandCount].resource = r;
                pendingDemand[demandCount].amount = 1;
                pendingDemand[demandCount].active = true;
                ++demandCount;
            }
        }
    }

    // Step 4: outgoingCount = buffer resources not in active deficit
    outgoingCount = 0;
    for (int s = 0; s < buffer.slotCount; ++s) {
        ResourceType r = buffer.slots[s].type;
        if (r == ResourceType_None) continue;
        if (FindDemand(r) < 0) {
            outgoingCount += buffer.slots[s].amount;
        }
    }
}

} // namespace World
