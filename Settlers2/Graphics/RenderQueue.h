#pragma once
#include <algorithm>
#include "RenderTypes.h"

namespace Graphics {

class RenderQueue {
public:
    static const int MAX_COMMANDS = 16384;

    RenderQueue();
    ~RenderQueue();

    void BeginFrame();

    void Submit(const RenderCommand& cmd);

    void Sort();

    void Clear();

    int GetCommandCount() const { return m_commandCount; }

    const RenderCommand* GetCommands() const { return m_commands; }

    // ─── Convenience methods for common render scenarios ───

    // Submit a sprite with full control over all parameters
    void SubmitSprite(float x, float y, float w, float h,
                      float u0, float v0, float u1, float v1,
                      WORD texID, WORD depth,
                      WORD shader, BYTE layer, BYTE blend = 1,
                      DWORD color = 0xFFFFFFFF) {
        RenderCommand cmd = {};
        cmd.x = x;
        cmd.y = y;
        cmd.width = w;
        cmd.height = h;
        cmd.u0 = u0;
        cmd.v0 = v0;
        cmd.u1 = u1;
        cmd.v1 = v1;
        cmd.textureID = texID;
        cmd.shaderID = shader;
        cmd.blendMode = blend;
        cmd.layer = layer;
        cmd.depth = depth;
        cmd.color = color;
        Submit(cmd);
    }

    // Submit a world sprite (terrain layer)
    void SubmitWorldSprite(float x, float y, float w, float h,
                          float u0, float v0, float u1, float v1,
                          WORD texID, WORD depth) {
        RenderCommand cmd = {};
        cmd.x = x;
        cmd.y = y;
        cmd.width = w;
        cmd.height = h;
        cmd.u0 = u0;
        cmd.v0 = v0;
        cmd.u1 = u1;
        cmd.v1 = v1;
        cmd.textureID = texID;
        cmd.shaderID = SHADER_TERRAIN;
        cmd.blendMode = 1;
        cmd.layer = LAYER_WORLD;
        cmd.depth = depth;
        cmd.color = 0xFFFFFFFF;
        Submit(cmd);
    }

    // Submit a UI element
    void SubmitUIElement(float x, float y, float w, float h,
                        float u0, float v0, float u1, float v1,
                        WORD texID, WORD depth = 100) {
        RenderCommand cmd = {};
        cmd.x = x;
        cmd.y = y;
        cmd.width = w;
        cmd.height = h;
        cmd.u0 = u0;
        cmd.v0 = v0;
        cmd.u1 = u1;
        cmd.v1 = v1;
        cmd.textureID = texID;
        cmd.shaderID = SHADER_UI;
        cmd.blendMode = 1;
        cmd.layer = LAYER_UI;
        cmd.depth = depth;
        cmd.color = 0xFFFFFFFF;
        Submit(cmd);
    }

    // Submit a foreground element (overlay on top of UI)
    void SubmitForegroundElement(float x, float y, float w, float h,
                                float u0, float v0, float u1, float v1,
                                WORD texID, WORD depth = 200) {
        RenderCommand cmd = {};
        cmd.x = x;
        cmd.y = y;
        cmd.width = w;
        cmd.height = h;
        cmd.u0 = u0;
        cmd.v0 = v0;
        cmd.u1 = u1;
        cmd.v1 = v1;
        cmd.textureID = texID;
        cmd.shaderID = SHADER_UI;
        cmd.blendMode = 1;
        cmd.layer = LAYER_FOREGROUND;
        cmd.depth = depth;
        cmd.color = 0xFFFFFFFF;
        Submit(cmd);
    }

private:
    RenderCommand m_commands[MAX_COMMANDS];
    int m_commandCount;
};

}
