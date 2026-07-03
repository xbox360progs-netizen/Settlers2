#include "stdafx.h"
#include "BannerPass.h"
#include "RenderBanner.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/TextManager.h"
#include "../../Graphics/RenderLayers.h"

namespace Scene {

BannerPass::BannerPass(TextManager* textManager)
    : m_textManager(textManager)
    , m_textureSlot(0)
{
}

void BannerPass::Execute(const RenderFrame& frame, const RenderContext& context,
                          RenderCommandBuffer& buffer)
{
    const RenderBanner& banner = frame.ui.banner;
    if (!banner.loaded) {
        // Even if banner is not loaded, draw status text if present.
        if (m_textManager && banner.statusText[0] != '\0') {
            float screenW = 1280.0f;
            float screenH = 720.0f;
            float textY = screenH - 40.0f;
            m_textManager->DrawString(banner.statusText, 40.0f, textY, 0xFFFFFFFF, 0.096f);
        }
        return;
    }

    float screenW = 1280.0f;
    float screenH = 720.0f;
    float textY = screenH - 40.0f;

    // Banner sprite
    if (banner.slideX < screenW) {
        buffer.PushSprite(
            static_cast<int>(banner.slideX),
            static_cast<int>(textY - banner.h),
            banner.w, banner.h,
            banner.u0, banner.v0, banner.u1, banner.v1,
            m_textureSlot, 0, 0xFFFFFFFF,
            0xFFFF, 0xFF, LAYER_EFFECTS
        );
    }

    // Status text
    if (m_textManager && banner.statusText[0] != '\0') {
        float textX = banner.slideX + 40.0f;
        m_textManager->DrawString(
            banner.statusText,
            textX,
            textY - banner.h + 4.0f + 25.0f,
            0xFFFFFFFF, 0.096f);
    }
}

} // namespace Scene
