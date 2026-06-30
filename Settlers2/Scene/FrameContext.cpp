#include "stdafx.h"
#include "FrameContext.h"

namespace Scene {

    OverlayFrameState::OverlayFrameState()
        : geologistState(GEOLOGIST_NONE)
        , geologistTileX(-1)
        , geologistTileY(-1)
        , bannerSlideX(1280.0f)
        , bannerW(0.0f)
        , bannerH(0.0f)
        , bannerU0(0.0f)
        , bannerV0(0.0f)
        , bannerU1(0.0f)
        , bannerV1(0.0f)
        , bannerLoaded(false)
        , townHallPanelBgIdx(-1)
        , townHallPanelU0(0.0f)
        , townHallPanelV0(0.0f)
        , townHallPanelU1(0.0f)
        , townHallPanelV1(0.0f)
        , townHallPanelW(0.0f)
        , townHallPanelH(0.0f)
        , resourceHudLoaded(false)
    {
        for (int i = 0; i < RESOURCE_HUD_COUNT; ++i) {
            resourceHud[i].type = World::ResourceType_None;
            resourceHud[i].iconName = NULL;
            resourceHud[i].iconIdx = -1;
            resourceHud[i].showOrder = 0;
        }
    }

} // namespace Scene
