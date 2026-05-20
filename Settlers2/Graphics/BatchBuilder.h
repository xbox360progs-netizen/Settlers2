#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include "RenderTypes.h"
#include "ShaderManager.h"

namespace Graphics {

struct RenderBatch {
    ShaderID shaderID;
    LPDIRECT3DTEXTURE9 texture;
    int startVertex;
    int vertexCount;
    int primitiveCount;
    float minDepth;
    float maxDepth;

    RenderBatch() 
        : shaderID(SHADER_INVALID)
        , texture(NULL)
        , startVertex(0)
        , vertexCount(0)
        , primitiveCount(0)
        , minDepth(1.0f)
        , maxDepth(0.0f) 
    {}
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
    void CreateBatch(const RenderCommand& cmd);
    void FlushCurrentBatch();
    void BuildVertexData();

    LPDIRECT3DDEVICE9 m_device;

    std::vector<RenderBatch> m_batches;
    std::vector<SpriteVertex> m_vertices;

    LPDIRECT3DVERTEXBUFFER9 m_vertexBuffer;
    int m_maxVertices;

    ShaderID m_currentShader;
    LPDIRECT3DTEXTURE9 m_currentTexture;
    int m_batchStartVertex;
};

}