#pragma once
#include <string.h>

namespace Scene {

// Notification banner + status text DTO.
struct RenderBanner {
    bool  loaded;
    float slideX;
    float w, h;
    float u0, v0, u1, v1;
    char  statusText[256];

    RenderBanner()
        : loaded(false), slideX(1280.0f)
        , w(0), h(0), u0(0), v0(0), u1(0), v1(0)
    {
        statusText[0] = '\0';
    }
};

} // namespace Scene
