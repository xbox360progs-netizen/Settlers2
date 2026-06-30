#pragma once
#include "UiCommand.h"

namespace UI {

    struct UiAction {
        UiCommand command;
        int       value;

        UiAction()
            : command(UI_CMD_NONE)
            , value(0)
        {
        }

        UiAction(UiCommand cmd, int val = 0)
            : command(cmd)
            , value(val)
        {
        }

        bool IsValid() const
        {
            return command != UI_CMD_NONE;
        }
    };

} // namespace UI
