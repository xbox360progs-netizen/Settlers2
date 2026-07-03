#include "stdafx.h"
#include "ConfirmationMenuPresentationSystem.h"
#include "../../UI/UIMenu.h"

namespace Scene {

void ConfirmationMenuPresentationSystem::BuildRenderFrame(RenderUiFrame& out)
{
    out.confirmation.visible = (m_geologistMenu && m_geologistMenu->IsVisible());
    out.confirmation.selected = 0;
    out.confirmation.style = 0;
    if (out.confirmation.visible) {
        strncpy_s(out.confirmation.titleText, sizeof(out.confirmation.titleText), "Геолог", _TRUNCATE);
        strncpy_s(out.confirmation.bodyText, sizeof(out.confirmation.bodyText),
            "Отправить геолога для поиска полезных ископаемых", _TRUNCATE);
    }
}

} // namespace Scene
