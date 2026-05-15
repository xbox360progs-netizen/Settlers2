#pragma once
#ifdef _XBOX
#include <xtl.h>
#else
#include <windows.h>
#endif
#include <algorithm>

struct LogicRenderCmd {
    int textureId;
    float x, y, w, h;
    float srcX, srcY, srcW, srcH;
    unsigned int color;
    float depth;
};

struct LogicFrameBuffer {
    static const int MAX_COMMANDS = 4096;
    LogicRenderCmd commands[MAX_COMMANDS];
    int commandCount;
    volatile bool isReady;
};

class ThreadSync {
public:
    CRITICAL_SECTION criticalSection;
    LogicFrameBuffer frameBuffers[2];
    volatile int writeBufferIndex;
    volatile int readBufferIndex;
    volatile bool swapRequested;

    void Initialize() {
        InitializeCriticalSection(&criticalSection);
        for (int i = 0; i < 2; i++) {
            frameBuffers[i].commandCount = 0;
            frameBuffers[i].isReady = false;
        }
        writeBufferIndex = 0;
        readBufferIndex = 1;
        swapRequested = false;
    }

    void SwapBuffers() {
        EnterCriticalSection(&criticalSection);
        if (frameBuffers[writeBufferIndex].isReady && !frameBuffers[readBufferIndex].isReady) {
            int temp = writeBufferIndex;
            writeBufferIndex = readBufferIndex;
            readBufferIndex = temp;
            swapRequested = true;
        }
        LeaveCriticalSection(&criticalSection);
    }

    LogicFrameBuffer& GetWriteBuffer() {
        return frameBuffers[writeBufferIndex];
    }

    LogicFrameBuffer& GetReadBuffer() {
        return frameBuffers[readBufferIndex];
    }

    void MarkReadBufferConsumed() {
        EnterCriticalSection(&criticalSection);
        frameBuffers[readBufferIndex].isReady = false;
        swapRequested = false;
        LeaveCriticalSection(&criticalSection);
    }

    void SortReadBufferByDepth() {
        EnterCriticalSection(&criticalSection);
        LogicFrameBuffer& buffer = frameBuffers[readBufferIndex];
        if (buffer.commandCount > 1) {
            std::sort(buffer.commands, buffer.commands + buffer.commandCount,
                [](const LogicRenderCmd& a, const LogicRenderCmd& b) {
                    return a.depth > b.depth;
                });
        }
        LeaveCriticalSection(&criticalSection);
    }
};

extern ThreadSync g_ThreadSync;