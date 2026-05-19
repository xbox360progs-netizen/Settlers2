#pragma once
#include <d3d9.h>
#include <vector>

namespace Graphics {

struct SpriteInstanceData {
    float posX, posY;
    float width, height;
    float u0, v0, u1, v1;
    DWORD color;
    float depth;
};

class GPUDrivenRenderer {
public:
    GPUDrivenRenderer();
    ~GPUDrivenRenderer();

    void Initialize(LPDIRECT3DDEVICE9 pDevice);
    void Shutdown();

    void BeginInstancedBatch(int maxInstances);
    void AddInstance(const SpriteInstanceData& instance);
    void EndInstancedBatch();

    void BeginTextureAtlasBatch(int maxSprites);
    void AddAtlasSprite(int atlasIndex, int spriteIndex, float x, float y, float depth, DWORD color);
    void EndTextureAtlasBatch();

    void SetMaxInstancesPerBatch(int max) { m_maxInstancesPerBatch = max; }
    int GetMaxInstancesPerBatch() const { return m_maxInstancesPerBatch; }

    void EnableInstancing(bool enable) { m_instancingEnabled = enable; }
    bool IsInstancingEnabled() const { return m_instancingEnabled; }

    void EnableTextureAtlases(bool enable) { m_textureAtlasesEnabled = enable; }
    bool IsTextureAtlasesEnabled() const { return m_textureAtlasesEnabled; }

    void SetInstanceBuffer(LPDIRECT3DVERTEXBUFFER9 buffer) { m_pInstanceBuffer = buffer; }
    LPDIRECT3DVERTEXBUFFER9 GetInstanceBuffer() const { return m_pInstanceBuffer; }

    int GetInstanceCount() const { return m_currentInstanceCount; }
    int GetDrawCallCount() const { return m_drawCallCount; }

    void ResetCounters();

private:
    LPDIRECT3DDEVICE9 m_pDevice;
    LPDIRECT3DVERTEXBUFFER9 m_pInstanceBuffer;
    LPDIRECT3DINDEXBUFFER9 m_pQuadIndexBuffer;
    LPDIRECT3DVERTEXBUFFER9 m_pQuadVertexBuffer;
    LPDIRECT3DVERTEXDECLARATION9 m_pInstancedDecl;

    bool m_instancingEnabled;
    bool m_textureAtlasesEnabled;
    int m_maxInstancesPerBatch;

    std::vector<SpriteInstanceData> m_instanceBuffer;
    int m_currentInstanceCount;
    int m_drawCallCount;

    int m_atlasBatchSize;
    std::vector<int> m_atlasSpriteIndices;

    struct AtlasBatch {
        int atlasIndex;
        int spriteCount;
        std::vector<SpriteInstanceData> sprites;
    };

    std::vector<AtlasBatch> m_atlasBatches;

    void CreateQuadGeometry();
    void CreateInstancedDeclaration();
    void UploadInstanceData();
};

}