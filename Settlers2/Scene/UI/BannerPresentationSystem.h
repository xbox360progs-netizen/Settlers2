#pragma once
#include "RenderBanner.h"

namespace Scene {

struct FrameContext;

// Reads FrameContext overlay + input state and produces RenderBanner DTO.
class BannerPresentationSystem {
public:
    void BuildRenderFrame(const FrameContext& frame, RenderBanner& out);
};

} // namespace Scene
