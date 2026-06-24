#pragma once

namespace Core {

enum EventType {
    Event_ConstructionComplete,
    Event_BuildingPlaced,
    Event_FlagPlaced,
    Event_FlagDeleted,
    Event_RoadBuilt,
    Event_ResourceDelivered,
    Event_BuildingProduction,
    Event_WorkerArrived,
    Event_WarehouseTransfer,
    Event_MAX
};

struct ConstructionCompleteData {
    int siteX, siteY;
    int buildingType;
    uint32_t flagId;
};

struct BuildingPlacedData {
    int posX, posY;
    int buildingType;
    uint32_t flagId;
};

struct FlagPlacedData {
    uint32_t flagId;
    int posX, posY;
};

struct RoadBuiltData {
    int startX, startY;
    int endX, endY;
    int tileCount;
};

struct ResourceDeliveredData {
    int resourceType;
    int amount;
    uint32_t destFlagId;
};

class EventListener {
public:
    virtual ~EventListener() {}
    virtual void OnEvent(EventType type, void* data) = 0;
};

static const int MAX_LISTENERS_PER_EVENT = 16;
static const int MAX_FRAME_EVENTS = 128;

struct ListenerSlot {
    EventListener* listener;
    bool active;
};

struct FrameEvent {
    EventType type;
    void* data;
};

class EventBus {
public:
    EventBus();
    ~EventBus();

    void Register(EventType type, EventListener* listener);
    void Unregister(EventType type, EventListener* listener);
    void UnregisterAll(EventListener* listener);

    void Broadcast(EventType type, void* data);

    void Post(EventType type, void* data);
    void Flush();

private:
    ListenerSlot m_listeners[Event_MAX][MAX_LISTENERS_PER_EVENT];
    FrameEvent m_frameEvents[MAX_FRAME_EVENTS];
    int m_frameEventCount;
};

} // namespace Core
