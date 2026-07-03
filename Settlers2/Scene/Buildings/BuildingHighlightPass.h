#pragma once
#include "../Rendering/RenderPass.h"
#include <stdint.h>
#include <vector>
#include <string>
#include <map>

namespace Scene {

class RenderCommandBuffer;
struct RenderFrame;
struct RenderContext;

class BuildingHighlightPass : public RenderPass {
public:
    BuildingHighlightPass();
    virtual void Execute(const RenderFrame& frame, const RenderContext& context,
                         RenderCommandBuffer& buffer);

    void SetTextureSlot(uint16_t slot) { m_textureSlot = slot; }

private:
    struct SpriteInfo { float u0, v0, u1, v1; float w, h; float pivotX, pivotY; };

    bool        m_atlasLoaded;
    uint16_t    m_textureSlot;
    std::map<int, SpriteInfo> m_spriteCache;  // BuildingType → sprite data

    void LoadAtlas();
    const SpriteInfo* GetSpriteInfo(int buildingType, bool depleted);

    BuildingHighlightPass(const BuildingHighlightPass&);
    BuildingHighlightPass& operator=(const BuildingHighlightPass&);
};

} // namespace Scene
