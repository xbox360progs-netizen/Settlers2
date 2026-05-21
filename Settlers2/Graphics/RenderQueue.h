#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include <algorithm>
#include "RenderTypes.h"
#include "BatchBuilder.h"

namespace Graphics {

struct SpriteCommand {
    WORD shaderID;
    float x, y;
    float width, height;
    float u0, v0, u1, v1;
    DWORD color;
    WORD depth;
    BYTE layer;
    BYTE blendMode;
    WORD textureID;

    SpriteCommand()
        : shaderID(0), x(0), y(0), width(0), height(0),
          u0(0), v0(0), u1(1), v1(1), color(0xFFFFFFFF),
          depth(0), layer(0), blendMode(0), textureID(0) {}
};

class RenderQueue {
public:
    RenderQueue();
    ~RenderQueue();

    void Initialize(LPDIRECT3DDEVICE9 pDevice);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void Submit(const SpriteCommand& cmd);

    int GetCommandCount() const;

    void Sort();
    void Batch();

    void Clear();

    int GetBatchCount() const { return m_batchBuilder.GetBatchCount(); }
    int GetDrawCallCount() const { return m_drawCallCount; }
    int GetSpriteCount() const { return (int)m_commands.size(); }

    const std::vector<RenderCommand>& GetCommands() const { return m_commands; }

    LPDIRECT3DVERTEXBUFFER9 GetVertexBuffer() const { return m_batchBuilder.GetVertexBuffer(); }
    int GetBuiltBatchCount() const { return m_batchBuilder.GetBatchCount(); }
    const RenderBatch* GetBuiltBatches() const { return m_batchBuilder.GetBatches(); }

private:
    LPDIRECT3DDEVICE9 m_pDevice;

    std::vector<RenderCommand> m_commands;

    BatchBuilder m_batchBuilder;

    int m_batchCount;
    int m_drawCallCount;
    int m_maxCommands;
};

}
