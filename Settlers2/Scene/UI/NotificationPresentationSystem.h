#pragma once
#include "RenderUiFrame.h"

namespace Scene {

struct UiFrameState;

class NotificationPresentationSystem {
public:
    void BuildRenderFrame(const UiFrameState& uiState, RenderUiFrame& out);
};

} // namespace Scene
