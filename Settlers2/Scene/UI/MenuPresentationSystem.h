#pragma once
#include "RenderMenuPanel.h"

class GridMenu;
class UIMenu;

namespace Scene {

class MenuPresentationSystem {
public:
    MenuPresentationSystem();
    ~MenuPresentationSystem();

    void SetMenus(const GridMenu* buildMenu, const UIMenu* flagMenu);

    void BuildRenderFrame(RenderMenuPanel& panel);

private:
    void BuildGridMenuQuads(RenderMenuPanel& panel);
    void BuildFlagMenuQuads(RenderMenuPanel& panel);

    const GridMenu* m_buildMenu;
    const UIMenu* m_flagMenu;
};

} // namespace Scene
