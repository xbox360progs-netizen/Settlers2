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
    if (!m_atlasLoaded) {
        LoadAtlas();
        if (!m_atlasLoaded) return;
    }

    // Depth: 0.99f * 65535 — rendered above world entities (matches legacy depth).
    static const uint16_t kCursorDepth = static_cast<uint16_t>(0.99f * 65535.0f);

    // 1. Normal tile cursor
    if (frame.cursor.valid) {
        int sx = frame.cursor.screenX - static_cast<int>(m_pivotX);
        int sy = frame.cursor.screenY - static_cast<int>(m_pivotY);

        buffer.PushSprite(
            sx, sy,
            m_w, m_h,
            m_u0, m_v0, m_u1, m_v1,
            m_textureSlot,
            kCursorDepth,
            0xFFFFFFFF,                  // color: white
            0xFFFF,                      // shaderId: default
            0xFF,                        // blendMode: default (alpha)
            0xFF                         // layer: default
        );
    }

    // 2. Gamepad cursor (green-tinted, independent of menu state)
    if (frame.cursor.gamepadActive) {
        int gsx = frame.cursor.gamepadScreenX - static_cast<int>(m_pivotX);
        int gsy = frame.cursor.gamepadScreenY - static_cast<int>(m_pivotY);

        buffer.PushSprite(
            gsx, gsy,
            m_w, m_h,
            m_u0, m_v0, m_u1, m_v1,
            m_textureSlot,
            kCursorDepth,
            0xFF00FF00,                  // color: ARGB green
            0xFFFF,
            0xFF,
            0xFF
        );
    }
}

} // namespace Scene
