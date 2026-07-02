#pragma once
#include <stdint.h>

namespace Scene {

enum OverlayMarkerType : uint8_t {
    OVERLAY_MARKER_MOUNTAIN = 0,     // yellow highlight on mountain/rock tile
    OVERLAY_MARKER_DEPOSIT = 1,      // surveyed resource deposit icon
    OVERLAY_MARKER_WORKING = 2,      // geologist working indicator
};

} // namespace Scene
