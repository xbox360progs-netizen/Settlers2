#pragma once
#include <stdint.h>

namespace Scene {

// Screen-space town hall info panel.
// Produced by TownHallPresentationSystem from EconomyManager.
struct RenderTownHallPanel {
    bool    visible;
    float   panelX, panelY;      // screen position
    float   panelW, panelH;      // panel dimensions
    float   panelU0, panelV0;    // background UV coords
    float   panelU1, panelV1;

    static const int STOCK_COUNT = 6;
    int     stockValues[STOCK_COUNT]; // Wood, Planks, Stone, Fish, Meat, Coal

    RenderTownHallPanel()
        : visible(false)
        , panelX(0), panelY(0)
        , panelW(0), panelH(0)
        , panelU0(0), panelV0(0), panelU1(0), panelV1(0)
    {
        for (int i = 0; i < STOCK_COUNT; ++i)
            stockValues[i] = 0;
    }
};

} // namespace Scene
