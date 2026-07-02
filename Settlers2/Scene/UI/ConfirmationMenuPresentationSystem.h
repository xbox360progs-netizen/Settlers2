#pragma once
#include "RenderUiFrame.h"

class UIMenu;

namespace Scene {

class ConfirmationMenuPresentationSystem {
public:
    void SetGeologistMenu(const UIMenu* menu) { m_geologistMenu = menu; }

    void BuildRenderFrame(RenderUiFrame& out);

private:
    const UIMenu* m_geologistMenu;
};

} // namespace Scene
