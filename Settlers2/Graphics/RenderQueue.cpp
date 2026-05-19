#include "stdafx.h"
#include "RenderQueue.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

RenderQueue::RenderQueue()
    : m_pDevice(NULL), m_maxCommands(8192), m_batchingEnabled(true),
      m_debugDraw(false), m_batchCount(0), m_drawCallCount(0) {
}

RenderQueue::~RenderQueue() {
    Shutdown();
}

void RenderQueue::Initialize(LPDIRECT3DDEVICE9 pDevice) {
    m_pDevice = pDevice;
    m_commands.reserve(m_maxCommands);

    char buf[256];
    sprintf(buf, "[RenderQueue] Initialized with max %d commands\n", m_maxCommands);
    OutputDebugStringA(buf);
}

void RenderQueue::Shutdown() {
    Clear();
    m_pDevice = NULL;
}

void RenderQueue::BeginFrame() {
    m_commands.clear();
    m_batchedGroups.clear();
    m_batchCount = 0;
    m_drawCallCount = 0;
}

void RenderQueue::EndFrame() {
    if (m_debugDraw) {
        char buf[256];
        sprintf(buf, "[RenderQueue] Frame stats: %d commands, %d batches, %d draw calls\n",
                (int)m_commands.size(), m_batchCount, m_drawCallCount);
        OutputDebugStringA(buf);
    }
}

void RenderQueue::AddCommand(const RenderCommand& cmd) {
    if ((int)m_commands.size() >= m_maxCommands) {
        OutputDebugStringA("[RenderQueue] WARNING: Max commands reached, dropping command\n");
        return;
    }
    m_commands.push_back(cmd);
}

void RenderQueue::AddCommands(const RenderCommand* cmds, int count) {
    for (int i = 0; i < count; i++) {
        AddCommand(cmds[i]);
    }
}

void RenderQueue::Sort(RenderQueueSortMode mode) {
    switch (mode) {
    case SORT_BY_DEPTH:
        SortByDepth(m_commands);
        break;
    case SORT_BY_SHADER:
        SortByShader(m_commands);
        break;
    case SORT_BY_TEXTURE:
        SortByTexture(m_commands);
        break;
    case SORT_BY_DEPTH_THEN_SHADER_THEN_TEXTURE:
        std::sort(m_commands.begin(), m_commands.end(),
                 [](const RenderCommand& a, const RenderCommand& b) {
                     if (a.depth != b.depth) return a.depth > b.depth;
                     if (a.shaderID != b.shaderID) return a.shaderID < b.shaderID;
                     return a.pTexture < b.pTexture;
                 });
        break;
    default:
        break;
    }
}

void RenderQueue::SortByDepth(std::vector<RenderCommand>& cmds) {
    std::sort(cmds.begin(), cmds.end(),
              [](const RenderCommand& a, const RenderCommand& b) {
                  return a.depth > b.depth;
              });
}

void RenderQueue::SortByShader(std::vector<RenderCommand>& cmds) {
    std::sort(cmds.begin(), cmds.end(),
              [](const RenderCommand& a, const RenderCommand& b) {
                  return a.shaderID < b.shaderID;
              });
}

void RenderQueue::SortByTexture(std::vector<RenderCommand>& cmds) {
    std::sort(cmds.begin(), cmds.end(),
              [](const RenderCommand& a, const RenderCommand& b) {
                  return a.pTexture < b.pTexture;
              });
}

void RenderQueue::Clear() {
    m_commands.clear();
    m_batchedGroups.clear();
}

void RenderQueue::Execute(LPDIRECT3DDEVICE9 pDevice) {
    if (!pDevice || m_commands.empty()) return;

    char buf[256];
    sprintf(buf, "[RenderQueue] Executing %d commands\n", (int)m_commands.size());
    OutputDebugStringA(buf);

    const RenderCommand* lastCmd = NULL;

    for (size_t i = 0; i < m_commands.size(); i++) {
        const RenderCommand& cmd = m_commands[i];

        bool needShaderSwitch = !lastCmd || lastCmd->shaderID != cmd.shaderID;
        bool needTextureSwitch = !lastCmd || lastCmd->pTexture != cmd.pTexture;

        if (needShaderSwitch) {
            sprintf(buf, "[RenderQueue] Shader switch to %d\n", cmd.shaderID);
            OutputDebugStringA(buf);
        }

        if (needTextureSwitch && cmd.pTexture) {
            sprintf(buf, "[RenderQueue] Texture switch to %p\n", cmd.pTexture);
            OutputDebugStringA(buf);
        }

        sprintf(buf, "[RenderQueue] Draw: verts=%d, prims=%d\n", cmd.vertexCount, cmd.primitiveCount);
        OutputDebugStringA(buf);

        m_drawCallCount++;
        lastCmd = &cmd;
    }

    if (m_debugDraw) {
        sprintf(buf, "[RenderQueue] Execute complete: %d draw calls\n", m_drawCallCount);
        OutputDebugStringA(buf);
    }
}

void RenderQueue::BatchByShader() {
    if (!m_batchingEnabled || m_commands.empty()) return;

    m_batchedGroups.clear();

    int start = 0;
    int currentShader = m_commands[0].shaderID;

    for (size_t i = 1; i <= m_commands.size(); i++) {
        bool endOfGroup = (i == m_commands.size()) || (m_commands[i].shaderID != currentShader);

        if (endOfGroup) {
            BatchedGroup group;
            group.shaderID = currentShader;
            group.texture = m_commands[start].pTexture;
            group.startIndex = start;
            group.commandCount = (int)(i - start);
            group.minDepth = m_commands[start].depth;
            group.maxDepth = m_commands[start].depth;

            for (int j = start; j < (int)i; j++) {
                if (m_commands[j].depth < group.minDepth) group.minDepth = m_commands[j].depth;
                if (m_commands[j].depth > group.maxDepth) group.maxDepth = m_commands[j].depth;
            }

            m_batchedGroups.push_back(group);
            m_batchCount++;

            if (i < m_commands.size()) {
                start = (int)i;
                currentShader = m_commands[i].shaderID;
            }
        }
    }
}

void RenderQueue::BatchByTexture() {
    if (!m_batchingEnabled || m_commands.empty()) return;

    m_batchedGroups.clear();

    int start = 0;
    void* currentTex = m_commands[0].pTexture;

    for (size_t i = 1; i <= m_commands.size(); i++) {
        bool endOfGroup = (i == m_commands.size()) || (m_commands[i].pTexture != currentTex);

        if (endOfGroup) {
            BatchedGroup group;
            group.shaderID = m_commands[start].shaderID;
            group.texture = currentTex;
            group.startIndex = start;
            group.commandCount = (int)(i - start);
            group.minDepth = m_commands[start].depth;
            group.maxDepth = m_commands[start].depth;

            m_batchedGroups.push_back(group);
            m_batchCount++;

            if (i < m_commands.size()) {
                start = (int)i;
                currentTex = m_commands[i].pTexture;
            }
        }
    }
}

void RenderQueue::BatchByDepthAndShader() {
    if (!m_batchingEnabled || m_commands.empty()) return;

    Sort(SORT_BY_DEPTH_THEN_SHADER_THEN_TEXTURE);
    BatchByShader();
}

void RenderQueue::OptimizeBatches() {
}

bool RenderQueue::CanMergeCommands(const RenderCommand& a, const RenderCommand& b) {
    if (a.shaderID != b.shaderID) return false;
    if (a.pTexture != b.pTexture) return false;
    if (a.batchType != b.batchType) return false;
    if (abs(a.depth - b.depth) > 0.001f) return false;

    return true;
}

void RenderQueue::MergeCommands(RenderCommand& out, const RenderCommand& a, const RenderCommand& b) {
    out = a;
    out.vertexCount += b.vertexCount;
    out.primitiveCount += b.primitiveCount;
    out.baseVertex = min(a.baseVertex, b.baseVertex);
    out.vertexStart = min(a.vertexStart, b.vertexStart);
}

}