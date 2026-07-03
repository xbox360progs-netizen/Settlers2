#pragma once
#include "../Rendering/RenderPass.h"
#include <stdint.h>

class TextManager;

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

class LogisticsDebugPass : public RenderPass {
public:
    explicit LogisticsDebugPass(TextManager* textManager);
    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

private:
    TextManager* m_textManager;

    LogisticsDebugPass(const LogisticsDebugPass&);
    LogisticsDebugPass& operator=(const LogisticsDebugPass&);
};

} // namespace Scene
