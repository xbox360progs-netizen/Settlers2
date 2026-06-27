#include "stdafx.h"
#include "CommandBus.h"
#include "EventBus.h"
#include <cstring>
#include <cstdio>
#include <cassert>

namespace Core {

CommandBus::CommandBus()
    : m_frameCmdCount(0)
    , m_flushDepth(0)
    , m_frameProcessed(0)
    , m_eventBus(NULL)
{
    for (int t = 0; t < Cmd_MAX; ++t) {
        for (int i = 0; i < MAX_CMD_LISTENERS; ++i) {
            m_listeners[t][i].listener = NULL;
            m_listeners[t][i].active = false;
        }
    }
}

CommandBus::~CommandBus()
{
}

void CommandBus::Register(CommandType type, CommandListener* listener)
{
    if (!listener || type < 0 || type >= Cmd_MAX) return;
    for (int i = 0; i < MAX_CMD_LISTENERS; ++i) {
        if (!m_listeners[type][i].active) {
            m_listeners[type][i].listener = listener;
            m_listeners[type][i].active = true;
            return;
        }
    }
}

void CommandBus::Unregister(CommandType type, CommandListener* listener)
{
    if (!listener || type < 0 || type >= Cmd_MAX) return;
    for (int i = 0; i < MAX_CMD_LISTENERS; ++i) {
        if (m_listeners[type][i].listener == listener && m_listeners[type][i].active) {
            m_listeners[type][i].active = false;
            m_listeners[type][i].listener = NULL;
            return;
        }
    }
}

void CommandBus::UnregisterAll(CommandListener* listener)
{
    if (!listener) return;
    for (int t = 0; t < Cmd_MAX; ++t) {
        for (int i = 0; i < MAX_CMD_LISTENERS; ++i) {
            if (m_listeners[t][i].listener == listener && m_listeners[t][i].active) {
                m_listeners[t][i].active = false;
                m_listeners[t][i].listener = NULL;
            }
        }
    }
}

bool CommandBus::RejectCommand(CommandType type)
{
    (void)type;
    char buf[128];
    _snprintf(buf, sizeof(buf), "[CommandBus] REJECTED Post(type=%d): "
              "event dispatch in progress (Event→Command forbidden)\n", (int)type);
    OutputDebugStringA(buf);
    assert(false && "CommandBus: rejected Post() during event dispatch");
    return false;
}

bool CommandBus::Post(CommandType type)
{
    // Hard contract: no posting commands while events are being dispatched.
    // Event→Command feedback creates causal loops that bypass the phase barrier.
    // Listeners must not issue commands in response to world events.
    if (m_eventBus && m_eventBus->IsDispatching()) {
        return RejectCommand(type);
    }
    if (m_frameCmdCount >= MAX_FRAME_CMDS) return false;
    m_frameCmds[m_frameCmdCount].type = type;
    memset(&m_frameCmds[m_frameCmdCount].data, 0, sizeof(CommandData));
    m_frameCmdCount++;
    return true;
}

bool CommandBus::Flush()
{
    if (++m_flushDepth > MAX_FLUSH_DEPTH) {
        --m_flushDepth;
        return m_frameCmdCount > 0;
    }

    if (m_flushDepth == 1) {
        m_frameProcessed = 0;
    }

    int count = m_frameCmdCount;
    m_frameCmdCount = 0;
    bool limitHit = false;
    for (int i = 0; i < count; ++i) {
        if (++m_frameProcessed > MAX_CMDS_PER_FRAME) {
            limitHit = true;
            break;
        }
        // Dispatch to listeners
        CommandType t = m_frameCmds[i].type;
        if (t < 0 || t >= Cmd_MAX) continue;
        for (int li = 0; li < MAX_CMD_LISTENERS; ++li) {
            if (m_listeners[t][li].active && m_listeners[t][li].listener) {
                m_listeners[t][li].listener->OnCommand(t, &m_frameCmds[i].data);
            }
        }
    }

    --m_flushDepth;

    if (limitHit) {
        m_frameCmdCount = 0;
        return false;
    }

    return m_frameCmdCount > 0;
}

} // namespace Core
