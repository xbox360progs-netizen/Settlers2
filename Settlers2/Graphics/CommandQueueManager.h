#pragma once
#include <d3d9.h>
#include <vector>
#include "RenderTypes.h"

namespace Graphics {

enum CommandQueueType {
    QUEUE_GEOMETRY,
    QUEUE_TRANSPARENT,
    QUEUE_UI,
    QUEUE_POSTFX,
    QUEUE_COUNT
};

class FrameAllocator {
public:
    FrameAllocator();
    ~FrameAllocator();
    
    void Initialize(size_t capacity);
    void Shutdown();
    
    void* Allocate(size_t size, size_t alignment = 8);
    void Reset();
    
    size_t GetUsedSize() const { return m_usedSize; }
    size_t GetCapacity() const { return m_capacity; }
    
private:
    char* m_buffer;
    size_t m_capacity;
    size_t m_usedSize;
    bool m_initialized;
};

class CommandQueue {
public:
    CommandQueue(CommandQueueType type);
    ~CommandQueue();
    
    void Initialize(LPDIRECT3DDEVICE9 device);
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    
    void Add(const RenderCommand& cmd);
    void AddImmutable(const RenderCommand& cmd);
    
    void SortByMaterial();
    void SortByDepth();
    void SortDeterministic();
    
    void Execute(LPDIRECT3DDEVICE9 device);
    void Clear();
    
    int GetCommandCount() const { return m_readOnly ? m_readOnly->size() : m_commands.size(); }
    bool IsImmutable() const { return m_isImmutable; }
    
    void MakeImmutable();
    
private:
    CommandQueueType m_type;
    LPDIRECT3DDEVICE9 m_device;
    std::vector<RenderCommand> m_commands;
    const std::vector<RenderCommand>* m_readOnly;
    bool m_isImmutable;
    int m_executedCount;
    
    void ExecuteInternal(LPDIRECT3DDEVICE9 device, const std::vector<RenderCommand>& cmds);
};

class CommandQueueManager {
public:
    CommandQueueManager();
    ~CommandQueueManager();
    
    void Initialize(LPDIRECT3DDEVICE9 device);
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    
    CommandQueue* GetQueue(CommandQueueType type);
    
    void ExecuteAll();
    void ClearAll();
    
    void ValidateNoLeaks();
    
    FrameAllocator* GetFrameAllocator() { return &m_frameAllocator; }
    
private:
    LPDIRECT3DDEVICE9 m_device;
    CommandQueue* m_queues[QUEUE_COUNT];
    FrameAllocator m_frameAllocator;
    bool m_frameActive;
};

CommandQueueManager* GetGlobalQueueManager();
void SetGlobalQueueManager(CommandQueueManager* mgr);

}