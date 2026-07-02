#pragma once

namespace Scene {

struct RenderNotification {
    bool  isActive;
    float alpha;
    float offsetY;
    char  title[32];
    char  line1[32];
    char  line2[32];
};

} // namespace Scene
