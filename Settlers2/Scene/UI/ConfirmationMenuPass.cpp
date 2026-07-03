#include "stdafx.h"
#include "ConfirmationMenuPass.h"
#include "RenderUiFrame.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/TextureRegistry.h"
#include "../../Graphics/SpriteAtlas.h"
#include "../../Graphics/TextManager.h"
#include "../../Graphics/RenderLayers.h"

namespace Scene {

ConfirmationMenuPass::ConfirmationMenuPass(TextManager* textManager)
    : m_atlasLoaded(false)
    , m_bgSlot(0)
    , m_iconSlot(0)
    , m_textManager(textManager)
{
}

void ConfirmationMenuPass::LoadAtlas()
{
    TextureRegistry& reg = TextureRegistry::instance();
    reg.getTextureOrLoad("ui");
    reg.getTextureOrLoad("Icon");

    std::tr1::shared_ptr<SpriteAtlas> uiAtl = reg.getAtlas("ui");
    std::tr1::shared_ptr<SpriteAtlas> iconAtl = reg.getAtlas("Icon");

    if (!uiAtl) return;

    // Menu panel background (geologist_menu or menu_creat_flag_road from MenuBootstrap)
    {
        const char* bgNames[] = { "geologist_menu", "menu_creat_flag_road" };
        for (int bi = 0; bi < 2; ++bi) {
            uint32_t idx = uiAtl->GetIndex(bgNames[bi]);
            if (idx != 0xFFFFFFFF) {
                const SpriteRegion* r = uiAtl->GetRegion(idx);
                if (r) {
                    m_bgPanel.u0 = r->u0; m_bgPanel.v0 = r->v0;
                    m_bgPanel.u1 = r->u1; m_bgPanel.v1 = r->v1;
                    m_bgPanel.w  = static_cast<float>(r->width);
                    m_bgPanel.h  = static_cast<float>(r->height);
                    break;
                }
            }
        }
    }

    // Icons (try ui atlas first, fall back to Icon atlas)
    auto loadSprite = [&](const char* name, SpriteSlot& out,
                          std::tr1::shared_ptr<SpriteAtlas>& alt) -> bool
    {
        if (uiAtl) {
            uint32_t idx = uiAtl->GetIndex(name);
            if (idx != 0xFFFFFFFF) {
                const SpriteRegion* r = uiAtl->GetRegion(idx);
                if (r) { out.u0=r->u0; out.v0=r->v0; out.u1=r->u1; out.v1=r->v1; out.w=static_cast<float>(r->width); out.h=static_cast<float>(r->height); return true; }
            }
        }
        if (alt) {
            uint32_t idx = alt->GetIndex(name);
            if (idx != 0xFFFFFFFF) {
                const SpriteRegion* r = alt->GetRegion(idx);
                if (r) { out.u0=r->u0; out.v0=r->v0; out.u1=r->u1; out.v1=r->v1; out.w=static_cast<float>(r->width); out.h=static_cast<float>(r->height); return true; }
            }
        }
        return false;
    };

    loadSprite("icon_mountain", m_iconMountain, iconAtl);
    loadSprite("icon_geologist", m_iconGeologist, iconAtl);
    loadSprite("ornament_1", m_ornament, iconAtl);

    m_atlasLoaded = true;
}

void ConfirmationMenuPass::Execute(
    const RenderFrame& frame, const RenderContext& context,
    RenderCommandBuffer& buffer)
{
    if (!frame.ui.confirmation.visible) return;

    if (!m_atlasLoaded) {
        LoadAtlas();
        if (!m_atlasLoaded) return;
    }

    // Screen-space center and layout.
    float cx = 640.0f;
    float yOff = 200.0f;

    // Menu background panel (centered, matching UIMenu layout).
    if (m_bgPanel.w > 0.0f && m_bgPanel.h > 0.0f) {
        buffer.PushSprite(
            640.0f - m_bgPanel.w * 0.5f, 360.0f - m_bgPanel.h * 0.5f,
            m_bgPanel.w, m_bgPanel.h,
            m_bgPanel.u0, m_bgPanel.v0, m_bgPanel.u1, m_bgPanel.v1,
            m_bgSlot, 100, 0xFFFFFFFF,
            0xFFFF, 0xFF, LAYER_UI);
    }

    // Mountain icon.
    if (m_iconMountain.w > 0.0f) {
        buffer.PushSprite(
            cx - 24.0f, yOff, 48.0f, 48.0f,
            m_iconMountain.u0, m_iconMountain.v0,
            m_iconMountain.u1, m_iconMountain.v1,
            m_iconSlot, 101, 0xC88C6E50,
            0xFFFF, 0xFF, LAYER_UI);
    }

    // Geologist icon.
    if (m_iconGeologist.w > 0.0f) {
        buffer.PushSprite(
            cx - 18.0f, yOff + 90.0f, 36.0f, 36.0f,
            m_iconGeologist.u0, m_iconGeologist.v0,
            m_iconGeologist.u1, m_iconGeologist.v1,
            m_iconSlot, 101, 0xC8FFDC64,
            0xFFFF, 0xFF, LAYER_UI);
    }

    // Ornament.
    if (m_ornament.w > 0.0f) {
        buffer.PushSprite(
            cx - 50.0f, yOff + 132.0f, 100.0f, 14.0f,
            m_ornament.u0, m_ornament.v0,
            m_ornament.u1, m_ornament.v1,
            m_iconSlot, 101, 0xC8B49650,
            0xFFFF, 0xFF, LAYER_UI);
    }

    // Title + body text (pre-resolved in RenderConfirmationMenu DTO).
    if (m_textManager) {
        if (frame.ui.confirmation.titleText[0] != '\0') {
            m_textManager->DrawTextCenteredToScreen(
                frame.ui.confirmation.titleText,
                cx, yOff + 54.0f,
                D3DCOLOR_ARGB(255, 255, 255, 220), 0.095f,
                FONT_MENU, FONT_STYLE_NORMAL, LAYER_UI);
        }
        if (frame.ui.confirmation.bodyText[0] != '\0') {
            m_textManager->DrawTextCenteredToScreen(
                frame.ui.confirmation.bodyText,
                cx, yOff + 162.0f,
                D3DCOLOR_ARGB(255, 200, 200, 200), 0.08f,
                FONT_MENU, FONT_STYLE_NORMAL, LAYER_UI);
        }
    }
}

} // namespace Scene
