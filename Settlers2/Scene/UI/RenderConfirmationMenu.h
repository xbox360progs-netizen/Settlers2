#pragma once
#include <stdint.h>

namespace Scene {

// Describes a screen-space confirmation dialog overlay.
// Produced by ConfirmationMenuPresentationSystem from game state (e.g. geologist menu).
struct RenderConfirmationMenu {
    bool    visible;        // true when the confirmation dialog is active
    uint8_t selected;       // currently highlighted option index
    uint8_t style;          // dialog style (e.g. GEOLOGIST, DEMOLISH, ...)
    char    titleText[64];  // pre-resolved title (e.g. "Геолог")
    char    bodyText[128];  // pre-resolved body/description

    RenderConfirmationMenu()
        : visible(false)
        , selected(0)
        , style(0)
    {
        titleText[0] = '\0';
        bodyText[0] = '\0';
    }
};

} // namespace Scene
