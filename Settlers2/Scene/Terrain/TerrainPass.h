#pragma once
#include "../Rendering/RenderPass.h"

class TileRenderer;

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

// Thin pass wrapper that delegates terrain rendering to TileRenderer.
// Reads RenderFrame.terrain DTOs (screen coords pre-computed by
// ProjectionSystem) and pushes screen-space sprites to CommandBuffer.
class TerrainPass : public RenderPass {
public:
    explicit TerrainPass(TileRenderer& renderer)
        : m_tileRenderer(renderer) {}

    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

private:
    TileRenderer& m_tileRenderer;
};

}
