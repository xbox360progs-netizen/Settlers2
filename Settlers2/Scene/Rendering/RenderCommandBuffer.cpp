#include "stdafx.h"
#include "RenderCommandBuffer.h"
#include "../../Graphics/RenderQueue.h"
#include "../../Graphics/RenderTypes.h"
#include "../../Graphics/RenderLayers.h"
#include "../../Graphics/ShaderManager.h"

namespace Scene {

// Sentinel values for PushSprite defaults
static const uint16_t kDefaultShader = 0xFFFF;
static const uint8_t  kDefaultBlend  = 0xFF;
static const uint8_t  kDefaultLayer  = 0xFF;

RenderCommandBuffer::RenderCommandBuffer()
{
}

RenderCommandBuffer::~RenderCommandBuffer()
{
    m_commands.clear();
}

void RenderCommandBuffer::PushSprite(int x, int y, float w, float h,
                                     float u0, float v0, float u1, float v1,
                                     uint16_t textureSlot, uint16_t depth,
                                     uint32_t color,
                                     uint16_t shaderId,
                                     uint8_t  blendMode,
                                     uint8_t  layer)
{
    RenderCommand cmd;
    cmd.x = static_cast<float>(x);
    cmd.y = static_cast<float>(y);
    cmd.width = w;
    cmd.height = h;
    cmd.u0 = u0; cmd.v0 = v0;
    cmd.u1 = u1; cmd.v1 = v1;
    cmd.textureSlot = textureSlot;
    cmd.depth = depth;
    cmd.shaderId = (shaderId == kDefaultShader) ? SHADER_WORLD_SCREEN : shaderId;
    cmd.blendMode = (blendMode == kDefaultBlend) ? 1 : blendMode;
    cmd.layer = (layer == kDefaultLayer) ? LAYER_WORLD : layer;
    cmd.color = color;
    m_commands.push_back(cmd);
}

void RenderCommandBuffer::Clear()
{
    m_commands.clear();
}

void RenderCommandBuffer::SubmitToQueue(Graphics::RenderQueue* queue)
{
    if (!queue) return;

    for (size_t i = 0; i < m_commands.size(); ++i) {
        const RenderCommand& src = m_commands[i];
        Graphics::RenderCommand dst;
        dst.x = src.x;
        dst.y = src.y;
        dst.width = src.width;
        dst.height = src.height;
        dst.u0 = src.u0; dst.v0 = src.v0;
        dst.u1 = src.u1; dst.v1 = src.v1;
        dst.color = src.color;
        dst.textureID = src.textureSlot;
        dst.shaderID = src.shaderId;
        dst.blendMode = src.blendMode;
        dst.layer = src.layer;
        dst.depth = src.depth;
        dst.sortKey = 0;
        queue->Submit(dst);
    }
}

}
