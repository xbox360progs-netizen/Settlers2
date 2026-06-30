#pragma once

namespace UI {

    enum UiCommand {
        UI_CMD_NONE,
        UI_CMD_NEW_GAME,
        UI_CMD_MAP_EDITOR,
        UI_CMD_SETTINGS,
        UI_CMD_EXIT,
        UI_CMD_SELECT,    // Accept/confirm with parameter
        UI_CMD_BACK,      // Navigate back one level
        UI_CMD_BUILD,     // Build a building (value = sprite index)
    };

} // namespace UI
