#pragma once
#include <stdint.h>
#include "EventBus.h"

namespace Core {

enum CommandType {
    Cmd_PlaceFlag,
    Cmd_DeleteFlag,
    Cmd_DeleteBuilding,
    Cmd_RemoveConstructionSite,
    Cmd_MAX
};

struct PlaceFlagCmd {
    int tileX, tileY;
    int buildingType;
    bool isFreeFlag;
    int buildX, buildY;
    bool autoConnectRoad;
};

struct DeleteFlagCmd {
    uint32_t flagId;
};

struct DeleteBuildingCmd {
    uint32_t flagId;
};

struct RemoveConstructionSiteCmd {
    unsigned int siteId;
};

class CommandListener {
public:
    virtual ~CommandListener() {}
    virtual void OnCommand(CommandType type, void* data) = 0;
};

static const int MAX_CMD_LISTENERS = 16;
static const int MAX_FRAME_CMDS = 32;

union CommandData {
    PlaceFlagCmd placeFlag;
    DeleteFlagCmd deleteFlag;
    DeleteBuildingCmd deleteBuilding;
    RemoveConstructionSiteCmd removeConstructionSite;
};

struct FrameCommand {
    CommandType type;
    CommandData data;
};

// Simple static-assert via negative-size array
#define CMD_ASSERT(cond, msg) typedef char CMD_ASSERT_##msg[(cond) ? 1 : -1]

CMD_ASSERT(sizeof(PlaceFlagCmd) <= sizeof(CommandData), PlaceFlagCmd_fits);
CMD_ASSERT(sizeof(DeleteFlagCmd) <= sizeof(CommandData), DeleteFlagCmd_fits);
CMD_ASSERT(sizeof(DeleteBuildingCmd) <= sizeof(CommandData), DeleteBuildingCmd_fits);
CMD_ASSERT(sizeof(RemoveConstructionSiteCmd) <= sizeof(CommandData), RemoveConstructionSiteCmd_fits);

class CommandBus {
public:
    CommandBus();
    ~CommandBus();

    void SetEventBus(EventBus* bus) { m_eventBus = bus; }

    void Register(CommandType type, CommandListener* listener);
    void Unregister(CommandType type, CommandListener* listener);
    void UnregisterAll(CommandListener* listener);

    // Post a command (queued, dispatched on Flush).
    // Returns true if queued, false if rejected.
    //
    // Hard contract: no posting commands while events are being dispatched.
    // Event->Command feedback creates causal loops that bypass the phase barrier.
    // If IsDispatching() is true, the command is rejected with a diagnostic
    // (OutputDebugString + debug assert) to catch the violation early.
    template<typename T>
    bool Post(CommandType type, const T& data)
    {
        if (m_eventBus && m_eventBus->IsDispatching()) {
            return RejectCommand(type);
        }
        if (m_frameCmdCount >= MAX_FRAME_CMDS) return false;
        FrameCommand& cmd = m_frameCmds[m_frameCmdCount];
        cmd.type = type;
        memcpy(&cmd.data, &data, sizeof(T));
        m_frameCmdCount++;
        return true;
    }

    bool Post(CommandType type);

    // Dispatch all queued commands
    // Returns true if more were posted during dispatch
    bool Flush();

    static const int MAX_FLUSH_DEPTH = 8;
    static const int MAX_CMDS_PER_FRAME = 1024;

private:
    // Reject a command posted during event dispatch — log + debug assert
    static bool RejectCommand(CommandType type);

    struct ListenerSlot {
        CommandListener* listener;
        bool active;
    };

    ListenerSlot m_listeners[Cmd_MAX][MAX_CMD_LISTENERS];
    FrameCommand m_frameCmds[MAX_FRAME_CMDS];
    int m_frameCmdCount;
    int m_flushDepth;
    int m_frameProcessed;
    EventBus* m_eventBus;
};

} // namespace Core
