#include "stdafx.h"
#include "BannerPresentationSystem.h"
#include "../FrameContext.h"
#include <string.h>

namespace Scene {

void BannerPresentationSystem::BuildRenderFrame(
    const FrameContext& frame, RenderBanner& out)
{
    out.loaded = frame.overlay.bannerLoaded;
    out.slideX = frame.overlay.bannerSlideX;
    out.w = frame.overlay.bannerW;
    out.h = frame.overlay.bannerH;
    out.u0 = frame.overlay.bannerU0;
    out.v0 = frame.overlay.bannerV0;
    out.u1 = frame.overlay.bannerU1;
    out.v1 = frame.overlay.bannerV1;

    strncpy_s(out.statusText, sizeof(out.statusText),
        frame.input.statusText, _TRUNCATE);
}

} // namespace Scene
