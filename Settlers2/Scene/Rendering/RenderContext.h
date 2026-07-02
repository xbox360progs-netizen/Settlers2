#pragma once
#include <stdint.h>

namespace Graphics { class Camera; }

namespace Scene {

// Read-only per-frame context for render passes.
// Provides frame-global state without coupling passes to simulation managers.
struct RenderContext {
    const Graphics::Camera* camera;

    float   time;               // elapsed seconds (for animations)
    bool    debugOverlay;

    RenderContext()
        : camera(NULL)
        , time(0.0f)
        , debugOverlay(false) {}
};

} // namespace Scene
