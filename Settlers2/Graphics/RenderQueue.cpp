#include "stdafx.h"
#include "RenderQueue.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

RenderQueue::RenderQueue()
    : m_pDevice(NULL)
    , m_batchCount(0)
    , m_drawCallCount(0)
    , m_maxCommands(16384)
{
}

RenderQueue::~RenderQueue() {
    Shutdown();
}

void RenderQueue::Initialize(LPDIRECT3DDEVICE9 pDevice) {
    m_pDevice = pDevice;

    m_commands.reserve(m_maxCommands);

    m_batchBuilder.Initialize(pDevice);

    char buf[256];
    sprintf(buf, "[RenderQueue] Initialized with max %d commands\n", m_maxCommands);
    OutputDebugStringA(buf);
}

void RenderQueue::Shutdown() {
    Clear();
    m_batchBuilder.Shutdown();
    m_pDevice = NULL;
}

void RenderQueue::BeginFrame() {
    m_commands.clear();
    m_batchCount = 0;
    m_drawCallCount = 0;
    m_batchBuilder.BeginFrame();
}

void RenderQueue::EndFrame() {
    char buf[256];
    sprintf(buf, "[RenderQueue] Frame: commands=%d, batches=%d\n",
            (int)m_commands.size(), m_batchCount);
    OutputDebugStringA(buf);
}

void RenderQueue::Submit(const SpriteCommand& cmd) {
    RenderCommand renderCmd;
    renderCmd.x = cmd.x;
    renderCmd.y = cmd.y;
    renderCmd.width = cmd.width;
    renderCmd.height = cmd.height;
    renderCmd.u0 = cmd.u0;
    renderCmd.v0 = cmd.v0;
    renderCmd.u1 = cmd.u1;
    renderCmd.v1 = cmd.v1;
    renderCmd.color = cmd.color;
    renderCmd.textureID = cmd.textureID;
    renderCmd.shaderID = cmd.shaderID;
    renderCmd.blendMode = cmd.blendMode;
    renderCmd.layer = cmd.layer;
    renderCmd.depth = cmd.depth;
    renderCmd.sortKey = BuildSortKey(cmd.layer, cmd.blendMode, cmd.shaderID, cmd.textureID, cmd.depth);

    if ((int)m_commands.size() < m_maxCommands) {
        m_commands.push_back(renderCmd);
    }
}

int RenderQueue::GetCommandCount() const {
    return (int)m_commands.size();
}

void RenderQueue::Sort() {
    std::sort(m_commands.begin(), m_commands.end(),
              [](const RenderCommand& a, const RenderCommand& b) {
                  return a.sortKey < b.sortKey;
              });
}

void RenderQueue::Batch() {
    m_batchCount = 0;
    m_drawCallCount = 0;

    m_batchBuilder.BuildBatches(m_commands);

    m_batchCount = m_batchBuilder.GetBatchCount();
    m_drawCallCount = m_batchCount;
}

void RenderQueue::Clear() {
    m_commands.clear();
}

}
