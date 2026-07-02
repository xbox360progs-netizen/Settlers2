#include "stdafx.h"
#include "SettlerRenderer.h"
#include "RenderSettler.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/SpriteAtlas.h"

namespace Scene {

SettlerRenderer::SettlerRenderer()
    : m_unitsAtlas(NULL)
    , m_iconAtlas(NULL)
    , m_unitSlot(0)
{
}

void SettlerRenderer::SetAtlases(
    SpriteAtlas* unitsAtlas,
    SpriteAtlas* iconAtlas,
    int unitTextureSlot)
{
    m_unitsAtlas = unitsAtlas;
    m_iconAtlas = iconAtlas;
    m_unitSlot = unitTextureSlot;
}

void SettlerRenderer::Render(
    RenderCommandBuffer& buffer,
    const RenderFrame& frame)
{
    // DEPRECATED: WorkerPass + WorkerRenderer now handle all worker rendering.
    // Kept only because SettlerPass is still registered in RenderGraph.
    // All rendering delegated to WorkerPass; this method does nothing.
    (void)buffer;
    (void)frame;
}

} // namespace Scene
