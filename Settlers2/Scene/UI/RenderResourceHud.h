#pragma once
#include <stdint.h>

namespace Scene {

struct RenderResourceHudItem {
    int  iconIdx;        // pre-resolved atlas index into Icon atlas
    int  stockCount;     // pre-computed from EconomyManager
    float u0, v0, u1, v1; // pre-resolved UV coords

    RenderResourceHudItem()
        : iconIdx(-1), stockCount(0)
        , u0(0), v0(0), u1(0), v1(0) {}
};

struct RenderResourceHud {
    static const int MAX_ITEMS = 11;
    RenderResourceHudItem items[MAX_ITEMS];
    int  count;
    bool loaded;

    RenderResourceHud() : count(0), loaded(false) {}
};

} // namespace Scene
