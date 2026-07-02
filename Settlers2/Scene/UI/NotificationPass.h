#pragma once
#include "../Rendering/RenderPass.h"
#include <stdint.h>

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

class NotificationPass : public RenderPass {
public:
    NotificationPass();
    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

    void SetTextureSlot(uint16_t slot) { m_slot = slot; }

private:
    NotificationPass(const NotificationPass&);
    NotificationPass& operator=(const NotificationPass&);

    uint16_t m_slot;
};

} // namespace Scene
