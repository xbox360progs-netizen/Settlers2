#pragma once
#include <vector>
#include "RenderFlagResource.h"
#include "../Presentation/Migration/IFlagSource.h"

// Reads flag inventory from IFlagSource and produces RenderFlagResource DTOs
// with world coords. ProjectionSystem later transforms to screen coords.

namespace Scene {

class FlagResourcePresentationSystem {
public:
    void SetFlagSource(IFlagSource* source) { m_flagSource = source; }

    void BuildRenderFrame(std::vector<RenderFlagResource>& outResources);

private:
    IFlagSource* m_flagSource;
};

} // namespace Scene