#pragma once
#include <algorithm>
#include "RenderTypes.h"

namespace Graphics {

class RenderQueue {
public:
    static const int MAX_COMMANDS = 16384;

    RenderQueue();
    ~RenderQueue();

    void BeginFrame();

    void Submit(const RenderCommand& cmd);

    void Sort();

    void Clear();

    int GetCommandCount() const { return m_commandCount; }

    const RenderCommand* GetCommands() const { return m_commands; }

private:
    RenderCommand m_commands[MAX_COMMANDS];
    int m_commandCount;
};

}
