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
    Event_FlagTopologyChanged,
    Event_MAX
};

struct ConstructionCompleteData {
    int siteX, siteY;
    int buildingType;
    uint32_t flagId;
    unsigned int siteId;
};

struct BuildingPlacedData {
    int posX, posY;
    int buildingType;
    uint32_t flagId;
};

struct FlagPlacedData {
    uint32_t flagId;
    int posX, posY;
    int buildingType;
    int buildX, buildY;
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

struct BuildingProductionData {
    int buildingType;
    int outputResourceType;
    int outputAmount;
};

struct WorkerArrivedData {
    uint32_t flagId;
    int buildingType;
};

struct WarehouseTransferData {
    uint32_t srcFlagId;
    uint32_t dstFlagId;
    int resourceType;
    int amount;
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

template<typename T>
struct EventTraits {
    enum { Allowed = false };
};

#define DECLARE_EVENT_TYPE(T) \
    template<> struct EventTraits<T> { enum { Allowed = true }; }

DECLARE_EVENT_TYPE(ConstructionCompleteData);
DECLARE_EVENT_TYPE(BuildingPlacedData);
DECLARE_EVENT_TYPE(FlagPlacedData);
DECLARE_EVENT_TYPE(RoadBuiltData);
DECLARE_EVENT_TYPE(ResourceDeliveredData);
DECLARE_EVENT_TYPE(BuildingProductionData);
DECLARE_EVENT_TYPE(WorkerArrivedData);
DECLARE_EVENT_TYPE(WarehouseTransferData);

union EventData {
    ConstructionCompleteData constructionComplete;
    BuildingPlacedData buildingPlaced;
    FlagPlacedData flagPlaced;
    RoadBuiltData roadBuilt;
    ResourceDeliveredData resourceDelivered;
    BuildingProductionData buildingProduction;
    WorkerArrivedData workerArrived;
    WarehouseTransferData warehouseTransfer;
};

#define EVENT_STATIC_ASSERT(cond, msg) typedef char EVENT_ASSERT_##msg[(cond) ? 1 : -1]

EVENT_STATIC_ASSERT(sizeof(ConstructionCompleteData) <= sizeof(EventData), ConstructionCompleteData_fits);
EVENT_STATIC_ASSERT(sizeof(BuildingPlacedData) <= sizeof(EventData), BuildingPlacedData_fits);
EVENT_STATIC_ASSERT(sizeof(FlagPlacedData) <= sizeof(EventData), FlagPlacedData_fits);
EVENT_STATIC_ASSERT(sizeof(RoadBuiltData) <= sizeof(EventData), RoadBuiltData_fits);
EVENT_STATIC_ASSERT(sizeof(ResourceDeliveredData) <= sizeof(EventData), ResourceDeliveredData_fits);
EVENT_STATIC_ASSERT(sizeof(BuildingProductionData) <= sizeof(EventData), BuildingProductionData_fits);
EVENT_STATIC_ASSERT(sizeof(WorkerArrivedData) <= sizeof(EventData), WorkerArrivedData_fits);
EVENT_STATIC_ASSERT(sizeof(WarehouseTransferData) <= sizeof(EventData), WarehouseTransferData_fits);

enum { EVENT_DATA_SIZE = sizeof(EventData) };

struct FrameEvent {
    EventType type;
    EventData data;
};

class EventBus {
public:
    EventBus();
    ~EventBus();

    // True when this EventBus is currently dispatching events.
    // Checked by CommandBus::Post to enforce the Event→Command barrier
    // (commands must not be posted during event dispatch).
    bool IsDispatching() const { return m_dispatchingCount > 0; }

    void Register(EventType type, EventListener* listener);
    void Unregister(EventType type, EventListener* listener);
    void UnregisterAll(EventListener* listener);

    void Broadcast(EventType type, void* data);

    template<typename T>
    void Post(EventType type, const T& data)
    {
        EVENT_STATIC_ASSERT(EventTraits<T>::Allowed, Type_not_registered_as_event);
        if (m_frameEventCount >= MAX_FRAME_EVENTS) return;
        FrameEvent& ev = m_frameEvents[m_frameEventCount];
        ev.type = type;
        memcpy(&ev.data, &data, sizeof(T));
        m_frameEventCount++;
    }

    void Post(EventType type)
    {
        if (m_frameEventCount >= MAX_FRAME_EVENTS) return;
        m_frameEvents[m_frameEventCount].type = type;
        memset(&m_frameEvents[m_frameEventCount].data, 0, sizeof(EventData));
        m_frameEventCount++;
    }

    bool Flush();

    static const int MAX_FLUSH_DEPTH = 8;
    static const int MAX_EVENTS_PER_FRAME = 4096;

private:
    ListenerSlot m_listeners[Event_MAX][MAX_LISTENERS_PER_EVENT];
    FrameEvent m_frameEvents[MAX_FRAME_EVENTS];
    int m_frameEventCount;
    int m_dispatchingCount;
};

} // namespace Core
