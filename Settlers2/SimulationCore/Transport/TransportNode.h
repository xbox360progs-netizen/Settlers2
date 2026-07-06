#pragma once
#include <stdint.h>
#include "../Core/ResourceTypes.h"
#include "../Transport/TransportTypes.h"
#include "ResourceBuffer.h"

namespace World {

static const int kMaxNodeAttachments = 4;
static const int kMaxNodeDemands = 4;
static const int kMaxNodeInputs = 4;
static const int kMaxTransportNodes = 64;

enum AttachmentRole {
    AR_Consumer = 0,
    AR_Producer,
    AR_ProducerConsumer
};

struct BuildingAttachment {
    uint8_t buildingId;
    AttachmentRole role;
    ResourceType inputs[kMaxNodeInputs];
    int inputCount;
};

struct DemandSlot {
    ResourceType resource;
    int amount;
    bool active;
    FlagId targetFlag;

    DemandSlot() : resource(ResourceType_None), amount(0), active(false), targetFlag(0) {}
};

// Domain aggregate — attachment point between Production and Transport.
// Owns buffer, attachments, pending demand. Does NOT decide allocation.
struct TransportNode {
    uint8_t id;
    ResourceBuffer buffer;
    BuildingAttachment attachments[kMaxNodeAttachments];
    int attachmentCount;
    DemandSlot pendingDemand[kMaxNodeDemands];
    int outgoingCount;

    TransportNode();

    // Lifecycle
    void AttachBuilding(uint8_t buildingId, const ResourceType inputs[], int inputCount, AttachmentRole role);
    void DetachBuilding(uint8_t buildingId);

    // Resource flow — incoming
    void ReceiveCargo(ResourceType resource, int amount);
    void ReceiveExport(ResourceType resource, int amount);

    // Resource flow — outgoing (called by LocalTransferSystem)
    bool TakeForBuilding(uint8_t buildingId, ResourceType resource, int amount);

    // Queries
    int GetBufferAmount(ResourceType resource) const;
    bool HasCapacity(ResourceType resource, int amount) const;
    bool HasDemandFor(ResourceType resource) const;

    // Heartbeat — evaluates buffer vs attachments, populates pendingDemand
    void Tick();

private:
    int FindAttachment(uint8_t buildingId) const;

public:
    // Returns index of active pending demand for resource, or -1
    int FindDemand(ResourceType resource) const;
};

} // namespace World
