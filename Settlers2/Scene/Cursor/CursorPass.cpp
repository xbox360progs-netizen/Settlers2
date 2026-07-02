#include "stdafx.h"
#include "CursorPass.h"
#include "RenderCursor.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/TextureRegistry.h"

namespace Scene {

CursorPass::CursorPass()
    : m_atlasLoaded(false)
    , m_u0(0), m_v0(0), m_u1(0), m_v1(0)
    , m_w(0), m_h(0)
    , m_pivotX(0), m_pivotY(0)
    , m_textureSlot(0)
{
}

void CursorPass::LoadAtlas()
{
    TextureRegistry& reg = TextureRegistry::instance();
    std::tr1::shared_ptr<SpriteAtlas> uiAtlas = reg.getAtlas("ui");
    if (!uiAtlas) return;

    uint32_t cursorIdx = uiAtlas->GetIndex("cursor");
    if (cursorIdx == 0xFFFFFFFF) return;

    const SpriteRegion* region = uiAtlas->GetRegion(cursorIdx);
    if (!region) return;

    m_u0 = region->u0;
    m_v0 = region->v0;
    m_u1 = region->u1;
    m_v1 = region->v1;
    m_w  = static_cast<float>(region->width);
    m_h  = static_cast<float>(region->height);
    m_pivotX = region->pivotX;
    m_pivotY = region->pivotY;
    m_atlasLoaded = true;
}

void CursorPass::Execute(const RenderFrame& frame, const RenderContext& context, RenderCommandBuffer& buffer)
{
    if (!frame.cursor.valid) return;

    if (!m_atlasLoaded) {
        LoadAtlas();
        if (!m_atlasLoaded) return;
    }

    // Apply pivot offset in screen space (SHADER_WORLD_SCREEN is 1:1 with pixels).
    int sx = frame.cursor.screenX - static_cast<int>(m_pivotX);
    int sy = frame.cursor.screenY - static_cast<int>(m_pivotY);

    // Depth: 0.99f * 65535 — rendered above world entities (matches legacy depth).
    static const uint16_t kCursorDepth = static_cast<uint16_t>(0.99f * 65535.0f);

    buffer.PushSprite(
        sx, sy,
        m_w, m_h,
        m_u0, m_v0, m_u1, m_v1,
        m_textureSlot,               // set by GameRenderer from SLOT_UI_CURSOR
        kCursorDepth,
        0xFFFFFFFF,                  // color
        0xFFFF,                      // shaderId: default (SHADER_WORLD_SCREEN)
        0xFF,                        // blendMode: default (alpha)
        0xFF                         // layer: default (LAYER_WORLD — cursor uses LAYER_FOREGROUND in legacy)
    );
}

} // namespace Scene
