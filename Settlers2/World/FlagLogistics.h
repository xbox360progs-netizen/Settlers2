#pragma once
#include "ResourceNode.h"
#include <vector>

namespace World {

struct LogisticsRequest {
    ResourceType type;
    int amount;
    int delivered;
    int priority;

    LogisticsRequest() : type(ResourceType_None), amount(0), delivered(0), priority(0) {}
};

struct LogisticsInventory {
    // Future home for ResourceSlot[8] and its methods.
    // Currently empty — data still lives directly on Flag.
};

struct LogisticsRequests {
    std::vector<LogisticsRequest> items;

    void Clear() { items.clear(); }
};

struct LogisticsOffer {
    ResourceType type;
    int amount;

    LogisticsOffer() : type(ResourceType_None), amount(0) {}
};

struct LogisticsSupply {
    std::vector<LogisticsOffer> items;

    void Clear() { items.clear(); }
};

struct FlagLogistics {
    LogisticsInventory inventory;
    LogisticsRequests requests;
    LogisticsSupply supply;
};

} // namespace World
