#pragma once
#include "UiAction.h"

namespace UI {

    class ICommandDispatcher {
    public:
        virtual ~ICommandDispatcher() {}
        virtual void Dispatch(const UiAction& action) = 0;
    };

} // namespace UI
