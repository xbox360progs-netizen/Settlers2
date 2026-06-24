#include "stdafx.h"
#include "EventBus.h"
#include <cstring>

namespace Core {

EventBus::EventBus()
    : m_frameEventCount(0)
{
    for (int t = 0; t < Event_MAX; ++t) {
        for (int i = 0; i < MAX_LISTENERS_PER_EVENT; ++i) {
            m_listeners[t][i].listener = NULL;
            m_listeners[t][i].active = false;
        }
    }
}

EventBus::~EventBus()
{
}

void EventBus::Register(EventType type, EventListener* listener)
{
    if (!listener || type < 0 || type >= Event_MAX) return;
    for (int i = 0; i < MAX_LISTENERS_PER_EVENT; ++i) {
        if (!m_listeners[type][i].active) {
            m_listeners[type][i].listener = listener;
            m_listeners[type][i].active = true;
            return;
        }
    }
}

void EventBus::Unregister(EventType type, EventListener* listener)
{
    if (!listener || type < 0 || type >= Event_MAX) return;
    for (int i = 0; i < MAX_LISTENERS_PER_EVENT; ++i) {
        if (m_listeners[type][i].listener == listener && m_listeners[type][i].active) {
            m_listeners[type][i].active = false;
            m_listeners[type][i].listener = NULL;
            return;
        }
    }
}

void EventBus::UnregisterAll(EventListener* listener)
{
    if (!listener) return;
    for (int t = 0; t < Event_MAX; ++t) {
        for (int i = 0; i < MAX_LISTENERS_PER_EVENT; ++i) {
            if (m_listeners[t][i].listener == listener && m_listeners[t][i].active) {
                m_listeners[t][i].active = false;
                m_listeners[t][i].listener = NULL;
            }
        }
    }
}

void EventBus::Broadcast(EventType type, void* data)
{
    if (type < 0 || type >= Event_MAX) return;
    for (int i = 0; i < MAX_LISTENERS_PER_EVENT; ++i) {
        if (m_listeners[type][i].active && m_listeners[type][i].listener) {
            m_listeners[type][i].listener->OnEvent(type, data);
        }
    }
}

void EventBus::Post(EventType type, void* data)
{
    if (m_frameEventCount < MAX_FRAME_EVENTS) {
        m_frameEvents[m_frameEventCount].type = type;
        m_frameEvents[m_frameEventCount].data = data;
        m_frameEventCount++;
    }
}

void EventBus::Flush()
{
    for (int i = 0; i < m_frameEventCount; ++i) {
        Broadcast(m_frameEvents[i].type, m_frameEvents[i].data);
    }
    m_frameEventCount = 0;
}

} // namespace Core
