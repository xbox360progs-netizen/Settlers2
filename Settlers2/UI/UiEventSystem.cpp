#include "stdafx.h"
#include "UiEventSystem.h"
#include "NotificationManager.h"
#include "../World/Map.h"
#include "../World/Components/Building.h"
#include "../World/ResourceNode.h"

namespace UI {

    // ─── Static helpers: ResourceType/BuildingType → UiMessageId ───

    static UiMessageId GetResourceNameId(int resourceType)
    {
        switch (resourceType) {
            case World::ResourceType_Wood:     return MSG_RESOURCE_WOOD;
            case World::ResourceType_Planks:   return MSG_RESOURCE_PLANKS;
            case World::ResourceType_Fish:     return MSG_RESOURCE_FISH;
            case World::ResourceType_Coal:     return MSG_RESOURCE_COAL;
            case World::ResourceType_IronOre:  return MSG_RESOURCE_IRONORE;
            case World::ResourceType_GoldOre:  return MSG_RESOURCE_GOLDORE;
            case World::ResourceType_IronBar:  return MSG_RESOURCE_IRONBAR;
            case World::ResourceType_GoldBar:  return MSG_RESOURCE_GOLDBAR;
            case World::ResourceType_Stone:    return MSG_RESOURCE_STONE;
            case World::ResourceType_Meat:     return MSG_RESOURCE_MEAT;
            case World::ResourceType_Wheat:    return MSG_RESOURCE_WHEAT;
            case World::ResourceType_Flour:    return MSG_RESOURCE_FLOUR;
            case World::ResourceType_Bread:    return MSG_RESOURCE_BREAD;
            case World::ResourceType_Water:    return MSG_RESOURCE_WATER;
            case World::ResourceType_Tools:    return MSG_RESOURCE_TOOLS;
            case World::ResourceType_Trap:     return MSG_RESOURCE_TRAP;
            case World::ResourceType_Field:    return MSG_RESOURCE_FIELD;
            case World::ResourceType_RealWood: return MSG_RESOURCE_REALWOOD;
            case World::ResourceType_ExoticWood: return MSG_RESOURCE_EXOTICWOOD;
            case World::ResourceType_BronzeOre:return MSG_RESOURCE_BRONZEORE;
            case World::ResourceType_Marble:   return MSG_RESOURCE_MARBLE;
            case World::ResourceType_Granite:  return MSG_RESOURCE_GRANITE;
            case World::ResourceType_Titanium: return MSG_RESOURCE_TITANIUM;
            case World::ResourceType_Salpeter: return MSG_RESOURCE_SALPETER;
            case World::ResourceType_BronzeBar:return MSG_RESOURCE_BRONZEBAR;
            default:                           return MSG_BUILDING_GENERIC;
        }
    }

    static UiMessageId GetBuildingNameId(int buildingType)
    {
        switch (buildingType) {
            case World::Woodcutter:    return MSG_BUILDING_WOODCUTTER;
            case World::Forester:      return MSG_BUILDING_FORESTER;
            case World::Sawmill:       return MSG_BUILDING_SAWMILL;
            case World::Stonemason:    return MSG_BUILDING_STONEMASON;
            case World::CoalMine:      return MSG_BUILDING_COALMINE;
            case World::BronzeMine:    return MSG_BUILDING_BRONZEMINE;
            case World::IronMine:      return MSG_BUILDING_IRONMINE;
            case World::GoldMine:      return MSG_BUILDING_GOLDMINE;
            case World::IronSmelter:   return MSG_BUILDING_IRONSMELTER;
            case World::GoldSmelter:   return MSG_BUILDING_GOLDSMELTER;
            case World::BronzeSmelter: return MSG_BUILDING_BRONZESMELTER;
            case World::Farm:          return MSG_BUILDING_FARM;
            case World::Mill:          return MSG_BUILDING_MILL;
            case World::Bakery:        return MSG_BUILDING_BAKERY;
            case World::Fisher:        return MSG_BUILDING_FISHER;
            case World::Hunter:        return MSG_BUILDING_HUNTER;
            case World::ToolWorkshop:  return MSG_BUILDING_TOOLWORKSHOP;
            case World::Storehouse:    return MSG_BUILDING_STOREHOUSE;
            case World::Well:          return MSG_BUILDING_WELL;
            case World::Barracks:      return MSG_BUILDING_BARRACKS;
            default:                   return MSG_BUILDING_GENERIC;
        }
    }

    // ─── Constructor / Destructor ──────────────────────────────────

    UiEventSystem::UiEventSystem(Core::EventBus* eventBus)
        : m_notificationCount(0)
        , m_eventBus(eventBus)
        , m_map(NULL)
        , m_notificationMgr(NULL)
    {
        for (int i = 0; i < MAX_NOTIFICATIONS; ++i) {
            m_notifications[i].isVisible = false;
            m_notifications[i].timer = 0.0f;
            m_notifications[i].title[0] = '\0';
            m_notifications[i].line1[0] = '\0';
            m_notifications[i].line2[0] = '\0';
            m_notifications[i].tileX = 0;
            m_notifications[i].tileY = 0;
        }
        for (int i = 0; i < EVENT_CACHE_SIZE; ++i) {
            m_eventCache[i].title[0] = '\0';
            m_eventCache[i].line1[0] = '\0';
            m_eventCache[i].timer = 0.0f;
            m_eventCache[i].count = 0;
        }

        if (m_eventBus) {
            m_eventBus->Register(Core::Event_ConstructionComplete, this);
            m_eventBus->Register(Core::Event_BuildingPlaced, this);
            m_eventBus->Register(Core::Event_FlagPlaced, this);
            m_eventBus->Register(Core::Event_FlagDeleted, this);
            m_eventBus->Register(Core::Event_ResourceDelivered, this);
            m_eventBus->Register(Core::Event_BuildingProduction, this);
        }
    }

    UiEventSystem::~UiEventSystem()
    {
        if (m_eventBus) {
            m_eventBus->UnregisterAll(this);
        }
    }

    void UiEventSystem::Update(float dt)
    {
        for (int i = m_notificationCount - 1; i >= 0; --i) {
            if (m_notifications[i].isVisible) {
                m_notifications[i].timer -= dt;
                if (m_notifications[i].timer <= 0.0f) {
                    RemoveNotification(i);
                }
            }
        }
        UpdateEventCache(dt);
    }

    int UiEventSystem::FindFreeSlot()
    {
        for (int i = 0; i < MAX_NOTIFICATIONS; ++i) {
            if (!m_notifications[i].isVisible)
                return i;
        }
        RemoveNotification(0);
        return m_notificationCount;
    }

    void UiEventSystem::RemoveNotification(int index)
    {
        if (index < 0 || index >= m_notificationCount) return;
        for (int i = index; i < m_notificationCount - 1; ++i) {
            m_notifications[i] = m_notifications[i + 1];
        }
        m_notificationCount--;
    }

    void UiEventSystem::ShowNotification(const char* title, const char* line1, const char* line2, float duration)
    {
        if (TryCoalesce(title, line1, duration))
            return;

        int slot = FindFreeSlot();
        if (slot < 0 || slot >= MAX_NOTIFICATIONS) return;

        World::PopupUiData& n = m_notifications[slot];
        strncpy_s(n.title, sizeof(n.title), title ? title : "", _TRUNCATE);
        strncpy_s(n.line1, sizeof(n.line1), line1 ? line1 : "", _TRUNCATE);
        strncpy_s(n.line2, sizeof(n.line2), line2 ? line2 : "", _TRUNCATE);
        n.tileX = -1;
        n.tileY = -1;
        n.timer = duration;
        n.isVisible = true;

        if (slot >= m_notificationCount)
            m_notificationCount = slot + 1;

        AddToEventCache(title, line1, duration);
    }

    void UiEventSystem::ShowNotificationAt(int tileX, int tileY, const char* title, const char* line1, const char* line2, float duration)
    {
        if (TryCoalesce(title, line1, duration))
            return;

        int slot = FindFreeSlot();
        if (slot < 0 || slot >= MAX_NOTIFICATIONS) return;

        World::PopupUiData& n = m_notifications[slot];
        strncpy_s(n.title, sizeof(n.title), title ? title : "", _TRUNCATE);
        strncpy_s(n.line1, sizeof(n.line1), line1 ? line1 : "", _TRUNCATE);
        strncpy_s(n.line2, sizeof(n.line2), line2 ? line2 : "", _TRUNCATE);
        n.tileX = tileX;
        n.tileY = tileY;
        n.timer = duration;
        n.isVisible = true;

        if (slot >= m_notificationCount)
            m_notificationCount = slot + 1;

        AddToEventCache(title, line1, duration);
    }

    const World::PopupUiData* UiEventSystem::GetNotification(int index) const
    {
        if (index < 0 || index >= m_notificationCount) return NULL;
        return &m_notifications[index];
    }

    // ─── Dedup cache ─────────────────────────────────────────────────

    bool UiEventSystem::TryCoalesce(const char* title, const char* line1, float duration)
    {
        if (!title || !line1) return false;
        for (int i = 0; i < EVENT_CACHE_SIZE; ++i) {
            EventCacheEntry& e = m_eventCache[i];
            if (e.timer > 0.0f && strcmp(e.title, title) == 0 && strcmp(e.line1, line1) == 0) {
                e.timer = duration;
                e.count++;

                for (int n = 0; n < m_notificationCount; ++n) {
                    if (m_notifications[n].isVisible &&
                        strcmp(m_notifications[n].title, title) == 0) {
                        int display = e.count > 99 ? 99 : e.count;
                        _snprintf(m_notifications[n].line2, sizeof(m_notifications[n].line2),
                            display >= 99 ? "x99+" : "x%d", display);
                        m_notifications[n].line2[sizeof(m_notifications[n].line2) - 1] = '\0';
                        m_notifications[n].timer = duration;
                        break;
                    }
                }
                return true;
            }
        }
        return false;
    }

    void UiEventSystem::AddToEventCache(const char* title, const char* line1, float duration)
    {
        int oldest = 0;
        for (int i = 0; i < EVENT_CACHE_SIZE; ++i) {
            if (m_eventCache[i].timer <= 0.0f) {
                oldest = i;
                break;
            }
            if (m_eventCache[i].timer < m_eventCache[oldest].timer)
                oldest = i;
        }
        EventCacheEntry& e = m_eventCache[oldest];
        strncpy_s(e.title, sizeof(e.title), title ? title : "", _TRUNCATE);
        strncpy_s(e.line1, sizeof(e.line1), line1 ? line1 : "", _TRUNCATE);
        e.timer = duration;
        e.count = 1;
    }

    void UiEventSystem::UpdateEventCache(float dt)
    {
        for (int i = 0; i < EVENT_CACHE_SIZE; ++i) {
            if (m_eventCache[i].timer > 0.0f) {
                m_eventCache[i].timer -= dt;
                if (m_eventCache[i].timer <= 0.0f) {
                    m_eventCache[i].title[0] = '\0';
                    m_eventCache[i].line1[0] = '\0';
                    m_eventCache[i].count = 0;
                }
            }
        }
    }

    // ─── EventListener ────────────────────────────────────────────────

    void UiEventSystem::OnEvent(Core::EventType type, void* data)
    {
        switch (type) {
            case Core::Event_ConstructionComplete:
                OnConstructionComplete(data);
                break;
            case Core::Event_BuildingPlaced:
                OnBuildingPlaced(data);
                break;
            case Core::Event_FlagPlaced:
                OnFlagPlaced(data);
                break;
            case Core::Event_FlagDeleted:
                OnFlagDeleted(data);
                break;
            case Core::Event_ResourceDelivered:
                OnResourceDelivered(data);
                break;
            case Core::Event_BuildingProduction:
                OnBuildingProduction(data);
                break;
            default:
                break;
        }
    }

    // ─── Migrated handlers (via NotificationManager) ──────────────────

    void UiEventSystem::OnConstructionComplete(void* data)
    {
        if (!m_notificationMgr) return;
        Core::ConstructionCompleteData* evt = (Core::ConstructionCompleteData*)data;
        if (!evt) return;
        UiMessageId nameId = GetBuildingNameId(evt->buildingType);
        m_notificationMgr->NotifyAt(evt->siteX, evt->siteY,
            MSG_TITLE_CONSTRUCTION_COMPLETE, nameId, MSG_CONSTRUCTION_COMPLETED, 5.0f);
    }

    void UiEventSystem::OnBuildingPlaced(void* data)
    {
        if (!m_notificationMgr) return;
        Core::BuildingPlacedData* evt = (Core::BuildingPlacedData*)data;
        if (!evt) return;
        UiMessageId nameId = GetBuildingNameId(evt->buildingType);
        m_notificationMgr->NotifyAt(evt->posX, evt->posY,
            MSG_TITLE_BUILDING, nameId, MSG_BUILDING_PLACED_TEXT, 3.0f);
    }

    void UiEventSystem::OnFlagPlaced(void* data)
    {
        if (!m_notificationMgr) return;
        Core::FlagPlacedData* evt = (Core::FlagPlacedData*)data;
        if (!evt) return;
        m_notificationMgr->NotifyAt(evt->posX, evt->posY,
            MSG_TITLE_FLAG, MSG_FLAG_PLACED_TEXT, MSG_NONE, 3.0f);
    }

    void UiEventSystem::OnFlagDeleted(void* data)
    {
        if (!m_notificationMgr) return;
        (void)data;
        m_notificationMgr->Notify(MSG_TITLE_FLAG, MSG_FLAG_REMOVED_TEXT, MSG_NONE, 3.0f);
    }

    void UiEventSystem::OnResourceDelivered(void* data)
    {
        if (!m_notificationMgr) return;
        Core::ResourceDeliveredData* evt = (Core::ResourceDeliveredData*)data;
        if (!evt) return;
        UiMessageId nameId = GetResourceNameId(evt->resourceType);
        m_notificationMgr->Notify(MSG_TITLE_DELIVERY, nameId, MSG_NONE, 3.0f);
    }

    void UiEventSystem::OnBuildingProduction(void* data)
    {
        if (!m_notificationMgr) return;
        Core::BuildingProductionData* evt = (Core::BuildingProductionData*)data;
        if (!evt) return;
        UiMessageId nameId = GetResourceNameId(evt->outputResourceType);
        m_notificationMgr->Notify(MSG_TITLE_PRODUCTION, nameId, MSG_NONE, 3.0f);
    }

} // namespace UI
