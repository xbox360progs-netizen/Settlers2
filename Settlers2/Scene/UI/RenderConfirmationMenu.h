#pragma once
#include <stdint.h>

namespace Scene {

// Describes a screen-space confirmation dialog overlay.
// Produced by ConfirmationMenuPresentationSystem from game state (e.g. geologist menu).
struct RenderConfirmationMenu {
    bool    visible;        // true when the confirmation dialog is active
    uint8_t selected;       // currently highlighted option index
    uint8_t style;          // dialog style (e.g. GEOLOGIST, DEMOLISH, ...)

    RenderConfirmationMenu()
        : visible(false)
        , selected(0)
        , style(0) {}
};

} // namespace Scene
