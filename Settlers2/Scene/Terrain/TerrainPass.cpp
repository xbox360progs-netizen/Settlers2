#include "stdafx.h"
#include "TerrainPass.h"
#include "RenderTerrainTile.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/TileRenderer.h"

namespace Scene {

void TerrainPass::Execute(const RenderFrame& frame, const RenderContext& context, RenderCommandBuffer& buffer)
{
    m_tileRenderer.RenderTerrainTiles(frame.terrain, buffer);
}

}
