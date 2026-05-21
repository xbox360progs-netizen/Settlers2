#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <stdint.h>
#include <type_traits>
#include "RenderTypes.h"

namespace Graphics {

struct RenderBatch {
    uint32_t startIndex;
    uint32_t indexCount;

    uint16_t textureID;
    uint16_t shaderID;

    uint8_t blendMode;
};

static_assert(std::is_pod<RenderBatch>::value,
    "RenderBatch must be POD");

class BatchBuilder {
public:
    static const int MAX_VERTICES = 65536;
    static const int MAX_BATCHES = 4096;

    BatchBuilder();
    ~BatchBuilder();

    void BeginFrame();
    void EndFrame();

    void BuildBatches(const RenderCommand* commands, uint32_t commandCount);

    uint32_t GetBatchCount() const { return m_batchCount; }
    const RenderBatch* GetBatches() const { return m_batches; }

    const SpriteVertex* GetVertices() const { return m_vertexPool; }
    uint32_t GetVertexCount() const { return m_vertexWritePos; }

    const uint32_t* GetIndices() const { return m_indexPool; }
    uint32_t GetIndexCount() const { return m_indexWritePos; }

    void Clear();

private:
    RenderBatch m_batches[MAX_BATCHES];
    uint32_t m_batchCount;

    SpriteVertex m_vertexPool[MAX_VERTICES];
    uint32_t m_vertexWritePos;

    uint32_t m_indexPool[MAX_VERTICES * 3 / 2];
    uint32_t m_indexWritePos;

    uint16_t m_currentTexture;
    uint16_t m_currentShader;
    uint8_t m_currentBlend;
};

}
