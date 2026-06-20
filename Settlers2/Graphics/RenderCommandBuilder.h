#pragma once
#include "RenderTypes.h"
#include "RenderLayers.h"

namespace Graphics {

//-------------------------------------------------------------------------------------
// RenderCommandBuilder - Fluent API for building render commands
// Reduces boilerplate code and improves readability
//-------------------------------------------------------------------------------------
class RenderCommandBuilder {
private:
    RenderCommand m_cmd;

public:
    RenderCommandBuilder() {
        memset(&m_cmd, 0, sizeof(m_cmd));
        m_cmd.color = 0xFFFFFFFF;  // Default: white, fully opaque
    }

    // ─── Individual property setters (fluent) ───
    RenderCommandBuilder& Position(float x, float y) {
        m_cmd.x = x;
        m_cmd.y = y;
        return *this;
    }

    RenderCommandBuilder& Size(float w, float h) {
        m_cmd.width = w;
        m_cmd.height = h;
        return *this;
    }

    RenderCommandBuilder& UV(float u0, float v0, float u1, float v1) {
        m_cmd.u0 = u0;
        m_cmd.v0 = v0;
        m_cmd.u1 = u1;
        m_cmd.v1 = v1;
        return *this;
    }

    RenderCommandBuilder& Texture(WORD texID) {
        m_cmd.textureID = texID;
        return *this;
    }

    RenderCommandBuilder& Shader(WORD shaderID) {
        m_cmd.shaderID = shaderID;
        return *this;
    }

    RenderCommandBuilder& Blend(BYTE blend) {
        m_cmd.blendMode = blend;
        return *this;
    }

    RenderCommandBuilder& Layer(BYTE layer) {
        m_cmd.layer = layer;
        return *this;
    }

    RenderCommandBuilder& Depth(WORD depth) {
        m_cmd.depth = depth;
        return *this;
    }

    RenderCommandBuilder& Color(DWORD color) {
        m_cmd.color = color;
        return *this;
    }

    // ─── Convenience methods for common scenarios ───

    // World sprite: terrain, units, buildings, etc.
    // Default: SHADER_TERRAIN, LAYER_WORLD, blend=1
    RenderCommandBuilder& WorldSprite(float x, float y, float w, float h,
                                      float u0, float v0, float u1, float v1,
                                      WORD textureID, WORD depth) {
        m_cmd.x = x;
        m_cmd.y = y;
        m_cmd.width = w;
        m_cmd.height = h;
        m_cmd.u0 = u0;
        m_cmd.v0 = v0;
        m_cmd.u1 = u1;
        m_cmd.v1 = v1;
        m_cmd.textureID = textureID;
        m_cmd.shaderID = SHADER_TERRAIN;
        m_cmd.blendMode = 1;
        m_cmd.layer = LAYER_WORLD;
        m_cmd.depth = depth;
        m_cmd.color = 0xFFFFFFFF;
        return *this;
    }

    // UI element: menu, UI layer sprites
    // Default: SHADER_UI, LAYER_UI, blend=1, depth=100
    RenderCommandBuilder& UIElement(float x, float y, float w, float h,
                                    float u0, float v0, float u1, float v1,
                                    WORD textureID, WORD depth = 100) {
        m_cmd.x = x;
        m_cmd.y = y;
        m_cmd.width = w;
        m_cmd.height = h;
        m_cmd.u0 = u0;
        m_cmd.v0 = v0;
        m_cmd.u1 = u1;
        m_cmd.v1 = v1;
        m_cmd.textureID = textureID;
        m_cmd.shaderID = SHADER_UI;
        m_cmd.blendMode = 1;
        m_cmd.layer = LAYER_UI;
        m_cmd.depth = depth;
        m_cmd.color = 0xFFFFFFFF;
        return *this;
    }

    // Foreground element: overlays on top of UI
    // Default: SHADER_UI, LAYER_FOREGROUND, blend=1, depth=200
    RenderCommandBuilder& ForegroundElement(float x, float y, float w, float h,
                                           float u0, float v0, float u1, float v1,
                                           WORD textureID, WORD depth = 200) {
        m_cmd.x = x;
        m_cmd.y = y;
        m_cmd.width = w;
        m_cmd.height = h;
        m_cmd.u0 = u0;
        m_cmd.v0 = v0;
        m_cmd.u1 = u1;
        m_cmd.v1 = v1;
        m_cmd.textureID = textureID;
        m_cmd.shaderID = SHADER_UI;
        m_cmd.blendMode = 1;
        m_cmd.layer = LAYER_FOREGROUND;
        m_cmd.depth = depth;
        m_cmd.color = 0xFFFFFFFF;
        return *this;
    }

    // ─── Output methods ───

    const RenderCommand& Build() const {
        return m_cmd;
    }

    void Submit(RenderQueue* queue) const;
};

}
