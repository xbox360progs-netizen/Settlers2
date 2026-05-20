#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include <map>
#include "RenderTypes.h"

namespace Graphics {

enum RenderQueueSortMode {
    SORT_NONE,
    SORT_BY_DEPTH,
    SORT_BY_SHADER,
    SORT_BY_TEXTURE,
    SORT_BY_DEPTH_THEN_SHADER_THEN_TEXTURE
};

enum RenderLayer {
    LAYER_OPAQUE,
    LAYER_TRANSPARENT,
    LAYER_UI,
    LAYER_COUNT
};

struct SpriteCommand {
    int shaderID;
    float x, y;
    float width, height;
    float u0, v0, u1, v1;
    DWORD color;
    float depth;
    RenderLayer layer;
    bool isUI;

    SpriteCommand()
        : shaderID(0), x(0), y(0), width(0), height(0),
          u0(0), v0(0), u1(1), v1(1), color(0xFFFFFFFF),
          depth(1.0f), layer(LAYER_OPAQUE), isUI(false) {}
};

struct BatchKey {
    int shaderID;
    void* texture;

    bool operator<(const BatchKey& other) const {
        if (shaderID != other.shaderID)
            return shaderID < other.shaderID;
        return texture < other.texture;
    }
};

struct DrawBatch {
    BatchKey key;
    int startIndex;
    int commandCount;
    float minDepth;
    float maxDepth;

    DrawBatch() : startIndex(0), commandCount(0), minDepth(0), maxDepth(0) {}
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
    void SubmitBatch(const RenderCommand& cmd);

    int GetCommandCount(RenderLayer layer) const;
    int GetTotalCommandCount() const;

    void Sort(RenderQueueSortMode mode = SORT_BY_DEPTH_THEN_SHADER_THEN_TEXTURE);
    void Batch();

    void Clear();

    void DispatchOpaque(void* backend);
    void DispatchTransparent(void* backend);
    void DispatchUI(void* backend);
    void DispatchAll(void* backend);

    int GetBatchCount() const { return m_batchCount; }
    int GetDrawCallCount() const { return m_drawCallCount; }

    const std::vector<RenderCommand>& GetOpaqueQueue() const { return m_opaqueQueue; }
    const std::vector<RenderCommand>& GetTransparentQueue() const { return m_transparentQueue; }
    const std::vector<RenderCommand>& GetUIQueue() const { return m_uiQueue; }

private:
    void SortQueue(std::vector<RenderCommand>& queue, RenderQueueSortMode mode);
    void CreateBatches(std::vector<RenderCommand>& queue);

    LPDIRECT3DDEVICE9 m_pDevice;

    std::vector<RenderCommand> m_opaqueQueue;
    std::vector<RenderCommand> m_transparentQueue;
    std::vector<RenderCommand> m_uiQueue;

    std::vector<DrawBatch> m_batches;

    int m_batchCount;
    int m_drawCallCount;
    int m_maxCommands;
};

}