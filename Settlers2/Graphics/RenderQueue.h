#pragma once
#include <d3d9.h>
#include <vector>
#include <algorithm>
#include "RenderTypes.h"

namespace Graphics {

enum RenderQueueSortMode {
    SORT_NONE,
    SORT_BY_DEPTH,
    SORT_BY_SHADER,
    SORT_BY_TEXTURE,
    SORT_BY_DEPTH_THEN_SHADER_THEN_TEXTURE
};

class RenderQueue {
public:
    RenderQueue();
    ~RenderQueue();

    void Initialize(LPDIRECT3DDEVICE9 pDevice);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void AddCommand(const RenderCommand& cmd);
    void AddCommands(const RenderCommand* cmds, int count);

    void Sort(RenderQueueSortMode mode = SORT_BY_DEPTH_THEN_SHADER_THEN_TEXTURE);

    void Clear();

    int GetCommandCount() const { return (int)m_commands.size(); }
    const RenderCommand* GetCommands() const { return m_commands.data(); }

    void Execute(LPDIRECT3DDEVICE9 pDevice);

    void SetMaxCommands(int max) { m_maxCommands = max; }
    int GetMaxCommands() const { return m_maxCommands; }

    void EnableBatching(bool enable) { m_batchingEnabled = enable; }
    bool IsBatchingEnabled() const { return m_batchingEnabled; }

    void SetDebugDraw(bool debug) { m_debugDraw = debug; }
    bool IsDebugDrawEnabled() const { return m_debugDraw; }

    int GetBatchCount() const { return m_batchCount; }
    int GetDrawCallCount() const { return m_drawCallCount; }

private:
    LPDIRECT3DDEVICE9 m_pDevice;
    std::vector<RenderCommand> m_commands;
    int m_maxCommands;
    bool m_batchingEnabled;
    bool m_debugDraw;

    int m_batchCount;
    int m_drawCallCount;

    struct BatchedGroup {
        int shaderID;
        void* texture;
        int startIndex;
        int commandCount;
        float minDepth;
        float maxDepth;
    };

    std::vector<BatchedGroup> m_batchedGroups;

    void BatchByShader();
    void BatchByTexture();
    void BatchByDepthAndShader();
    void OptimizeBatches();

    void SortByDepth(std::vector<RenderCommand>& cmds);
    void SortByShader(std::vector<RenderCommand>& cmds);
    void SortByTexture(std::vector<RenderCommand>& cmds);

    bool CanMergeCommands(const RenderCommand& a, const RenderCommand& b);
    void MergeCommands(RenderCommand& out, const RenderCommand& a, const RenderCommand& b);
};

}