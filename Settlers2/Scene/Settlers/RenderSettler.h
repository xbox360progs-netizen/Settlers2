#pragma once
#include "../Shared/RenderTransform.h"
#include "../Shared/SettlerVisual.h"

namespace Scene {

// Composed DTO: spatial identity + visual identity.
// No simulation pointers, no renderer state.
struct RenderSettler {
    RenderTransform transform;
    SettlerVisual   visual;
};

} // namespace Scene
