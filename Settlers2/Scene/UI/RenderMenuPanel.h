#pragma once
#include <stdint.h>
#include <string.h>

namespace Scene {

struct RenderMenuQuad {
    float x, y, w, h;
    float u0, v0, u1, v1;
    uint32_t color;
    uint16_t textureSlot;  // which pre-bound slot to use
    uint16_t depthLayer;

    RenderMenuQuad()
        : x(0), y(0), w(0), h(0)
        , u0(0), v0(0), u1(0), v1(0)
        , color(0xFFFFFFFF), textureSlot(0), depthLayer(10) {}
};

struct RenderMenuLabel {
    float x, y;
    uint32_t color;
    float size;
    char text[64];

    RenderMenuLabel()
        : x(0), y(0), color(0xFFFFFFFF), size(0.07f)
    { text[0] = '\0'; }
};

struct RenderMenuPanel {
    static const int MAX_QUADS = 64;
    static const int MAX_LABELS = 16;

    bool   buildMenuVisible;
    bool   flagMenuVisible;
    int    buildQuadCount;
    int    flagQuadCount;
    int    buildLabelCount;
    int    flagLabelCount;
    RenderMenuQuad quads[MAX_QUADS];
    RenderMenuLabel labels[MAX_LABELS];

    RenderMenuPanel()
        : buildMenuVisible(false), flagMenuVisible(false)
        , buildQuadCount(0), flagQuadCount(0)
        , buildLabelCount(0), flagLabelCount(0) {}
};

} // namespace Scene
