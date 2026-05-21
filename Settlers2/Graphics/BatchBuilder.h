#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include "RenderTypes.h"

namespace Graphics {

struct RenderBatch {
    WORD textureID;
    WORD shaderID;
    BYTE blendMode;

    DWORD vertexOffset;
    DWORD vertexCount;

    RenderBatch()
        : textureID(0), shaderID(0), blendMode(0),
          vertexOffset(0), vertexCount(0) {}
};

class BatchBuilder {
public:
    BatchBuilder();
    ~BatchBuilder();

    void Initialize(LPDIRECT3DDEVICE9 device);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void BuildBatches(const std::vector<RenderCommand>& commands);

    int GetBatchCount() const { return (int)m_batches.size(); }
    const RenderBatch* GetBatches() const { return m_batches.data(); }

    LPDIRECT3DVERTEXBUFFER9 GetVertexBuffer() const { return m_vertexBuffer; }

    void Clear();

private:
    void FlushVertexStream();

    LPDIRECT3DDEVICE9 m_device;

    std::vector<RenderBatch> m_batches;
    std::vector<SpriteVertex> m_vertices;

    LPDIRECT3DVERTEXBUFFER9 m_vertexBuffer;
    int m_maxVertices;

    WORD m_currentTexture;
    WORD m_currentShader;
    BYTE m_currentBlend;
};

}
