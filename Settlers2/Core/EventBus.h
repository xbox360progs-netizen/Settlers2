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

// Union large enough to hold any event data struct by value.
// Post() copies into this union so the caller's data can be stack-local.
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

// Compile-time checks: every event data struct must fit inside EventData.
// C++03 static assert via negative-size array typedef.
#define EVENT_STATIC_ASSERT(cond, msg) typedef char EVENT_ASSERT_##msg[(cond) ? 1 : -1]

EVENT_STATIC_ASSERT(sizeof(ConstructionCompleteData) <= sizeof(EventData), ConstructionCompleteData_fits);
EVENT_STATIC_ASSERT(sizeof(BuildingPlacedData) <= sizeof(EventData), BuildingPlacedData_fits);
EVENT_STATIC_ASSERT(sizeof(FlagPlacedData) <= sizeof(EventData), FlagPlacedData_fits);
EVENT_STATIC_ASSERT(sizeof(RoadBuiltData) <= sizeof(EventData), RoadBuiltData_fits);
EVENT_STATIC_ASSERT(sizeof(ResourceDeliveredData) <= sizeof(EventData), ResourceDeliveredData_fits);
EVENT_STATIC_ASSERT(sizeof(BuildingProductionData) <= sizeof(EventData), BuildingProductionData_fits);
EVENT_STATIC_ASSERT(sizeof(WorkerArrivedData) <= sizeof(EventData), WorkerArrivedData_fits);
EVENT_STATIC_ASSERT(sizeof(WarehouseTransferData) <= sizeof(EventData), WarehouseTransferData_fits);

// Calculate the value that sizeof(EventData) has at compile time
enum { EVENT_DATA_SIZE = sizeof(EventData) };

struct FrameEvent {
    EventType type;
    EventData data; // stored by value — valid until Flush
};

class EventBus {
public:
    EventBus();
    ~EventBus();

    void Register(EventType type, EventListener* listener);
    void Unregister(EventType type, EventListener* listener);
    void UnregisterAll(EventListener* listener);

    void Broadcast(EventType type, void* data);

    // Post a typed event.  T must be a POD struct matching the event type.
    // The data is copied by value into an internal buffer so the caller's
    // stack frame is safe to leave.
    template<typename T>
    void Post(EventType type, const T& data)
    {
        if (m_frameEventCount >= MAX_FRAME_EVENTS) return;
        FrameEvent& ev = m_frameEvents[m_frameEventCount];
        ev.type = type;
        memcpy(&ev.data, &data, sizeof(T));
        m_frameEventCount++;
    }

    // Post a notification with no payload data.
    void Post(EventType type)
    {
        if (m_frameEventCount >= MAX_FRAME_EVENTS) return;
        m_frameEvents[m_frameEventCount].type = type;
        memset(&m_frameEvents[m_frameEventCount].data, 0, sizeof(EventData));
        m_frameEventCount++;
    }

    // Dispatch all pending events. Returns true if more events were posted
    // during dispatch (caller should loop until false).
    // Depth guard prevents runaway recursion when a listener calls Flush().
    bool Flush();

    static const int MAX_FLUSH_DEPTH = 8;
    static const int MAX_EVENTS_PER_FRAME = 4096;

private:
    ListenerSlot m_listeners[Event_MAX][MAX_LISTENERS_PER_EVENT];
    FrameEvent m_frameEvents[MAX_FRAME_EVENTS];
    int m_frameEventCount;
};

} // namespace Core
