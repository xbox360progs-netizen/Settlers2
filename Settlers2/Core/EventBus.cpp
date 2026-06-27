#include "stdafx.h"
#include "EventBus.h"
#include <cstring>
#include <cstdio>

namespace Core {

EventBus::EventBus()
    : m_frameEventCount(0)
    , m_dispatchingCount(0)
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
    ++m_dispatchingCount;
    for (int i = 0; i < MAX_LISTENERS_PER_EVENT; ++i) {
        if (m_listeners[type][i].active && m_listeners[type][i].listener) {
            m_listeners[type][i].listener->OnEvent(type, data);
        }
    }
    --m_dispatchingCount;
}

bool EventBus::Flush()
{
    // Depth guard prevents runaway recursion when a listener calls Flush()
    // from inside a Broadcast handler.  Sequential calls from Simulation
    // (while(Flush())) are not affected — depth resets at the top level.
    static int s_flushDepth = 0;
    if (++s_flushDepth > MAX_FLUSH_DEPTH) {
        --s_flushDepth;
        return m_frameEventCount > 0;
    }

    // Per-frame event limit — resets at top-level Flush, protects against
    // infinite A→B→A→B loops that never drain the queue.
    static int s_frameProcessed = 0;
    if (s_flushDepth == 1) {
        s_frameProcessed = 0; // reset at the beginning of a flush cycle
    }

    // Snapshot the current batch then immediately drain so new Posts
    // during Broadcast go into a fresh buffer, not the current one.
    int count = m_frameEventCount;
    m_frameEventCount = 0;
    bool limitHit = false;
    for (int i = 0; i < count; ++i) {
        if (++s_frameProcessed > MAX_EVENTS_PER_FRAME) {
            limitHit = true;
            break;
        }
        Broadcast(m_frameEvents[i].type, &m_frameEvents[i].data);
    }

    --s_flushDepth;

    if (limitHit) {
        // Hard limit hit — discard the rest of the current batch AND any
        // events posted during Broadcast (they'll be re-posted next frame).
        m_frameEventCount = 0;
        return false;
    }

    // Return true if more events arrived during dispatch
    return m_frameEventCount > 0;
}

} // namespace Core
