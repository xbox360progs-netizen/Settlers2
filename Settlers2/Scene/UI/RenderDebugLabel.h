#pragma once
#include <stdint.h>

namespace Scene {

// World-space or screen-space debug text label.
// Produced by LogisticsDebugPresentationSystem, rendered by LogisticsDebugPass.
struct RenderDebugLabel {
    float   worldX, worldY;     // position (world or screen)
    char    text[64];           // pre-resolved text
    uint32_t color;             // D3DCOLOR_ARGB
    float   scale;              // font scale
    uint8_t fontId;             // FONT_MENU=0, FONT_DEBUG=1
    uint8_t style;              // FONT_STYLE_NORMAL=0
    float   depth;              // z-depth for world text
    uint8_t layer;              // render layer
    bool    isScreenSpace;      // true = screen coords, false = world coords

    RenderDebugLabel()
        : worldX(0), worldY(0)
        , color(0xFFFFFFFF)
        , scale(0.06f)
        , fontId(0)
        , style(0)
        , depth(0.05f)
        , layer(0)
        , isScreenSpace(false)
    {
        text[0] = '\0';
    }
};

} // namespace Scene
