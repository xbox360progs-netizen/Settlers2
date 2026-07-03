#pragma once
#include "../Rendering/RenderPass.h"
#include <stdint.h>

class TextManager;

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

class MenuPass : public RenderPass {
public:
    explicit MenuPass(TextManager* textManager);

    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

private:
    TextManager* m_textManager;

    MenuPass(const MenuPass&);
    MenuPass& operator=(const MenuPass&);
};

} // namespace Scene
