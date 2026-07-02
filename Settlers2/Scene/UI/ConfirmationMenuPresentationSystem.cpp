#include "stdafx.h"
#include "ConfirmationMenuPresentationSystem.h"
#include "../../UI/UIMenu.h"

namespace Scene {

void ConfirmationMenuPresentationSystem::BuildRenderFrame(RenderUiFrame& out)
{
    out.confirmation.visible = (m_geologistMenu && m_geologistMenu->IsVisible());
    out.confirmation.selected = 0;
    out.confirmation.style = 0;
}

} // namespace Scene
