#include "stdafx.h"
#include "RenderQueue.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

RenderQueue::RenderQueue()
    : m_commandCount(0)
{
}

RenderQueue::~RenderQueue() {
    Clear();
}

void RenderQueue::BeginFrame() {
    m_commandCount = 0;
}

void RenderQueue::Submit(const RenderCommand& cmd) {
    if (m_commandCount >= MAX_COMMANDS) return;

    RenderCommand* dst = &m_commands[m_commandCount++];
    *dst = cmd;
    dst->sortKey = BuildSortKey(cmd.layer, cmd.blendMode, cmd.shaderID, cmd.textureID, cmd.depth);
}

int RenderQueue::GetCommandCount() const {
    return m_commandCount;
}

void RenderQueue::Sort() {
    std::sort(m_commands, m_commands + m_commandCount,
              [](const RenderCommand& a, const RenderCommand& b) {
                  return a.sortKey < b.sortKey;
              });
}

void RenderQueue::Clear() {
    m_commandCount = 0;
}

}
