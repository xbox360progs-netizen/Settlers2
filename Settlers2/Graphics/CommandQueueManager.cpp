#include "stdafx.h"
#include "CommandQueueManager.h"
#include "RenderQueue.h"
#include <algorithm>
#include <cstring>

#ifdef _DEBUG
#define CMD_LOG(msg, ...) \
    do { \
        char _buf[256]; \
        sprintf(_buf, "[CmdQueue] " msg "\n", __VA_ARGS__); \
        ::OutputDebugStringA(_buf); \
    } while(0)
#else
#define CMD_LOG(...) ((void)0)
#endif

namespace Graphics {

static CommandQueueManager* g_queueManager = NULL;

FrameAllocator::FrameAllocator()
    : m_buffer(NULL)
    , m_capacity(0)
    , m_usedSize(0)
    , m_initialized(false)
{
}

FrameAllocator::~FrameAllocator() {
    Shutdown();
}

void FrameAllocator::Initialize(size_t capacity) {
    if (m_initialized) return;
    
    m_buffer = new char[capacity];
    m_capacity = capacity;
    m_usedSize = 0;
    m_initialized = true;
    
    CMD_LOG("Initialized with %llu bytes", capacity);
}

void FrameAllocator::Shutdown() {
    if (m_buffer) {
        delete[] m_buffer;
        m_buffer = NULL;
    }
    m_capacity = 0;
    m_usedSize = 0;
    m_initialized = false;
}

void* FrameAllocator::Allocate(size_t size, size_t alignment) {
    if (!m_initialized || !m_buffer) {
        return NULL;
    }
    
    size_t alignedSize = (size + alignment - 1) & ~(alignment - 1);
    
    if (m_usedSize + alignedSize > m_capacity) {
        CMD_LOG("ERROR: FrameAllocator overflow! Used: %llu, requested: %llu, capacity: %llu", 
                m_usedSize, alignedSize, m_capacity);
        return NULL;
    }
    
    void* ptr = m_buffer + m_usedSize;
    m_usedSize += alignedSize;
    
    memset(ptr, 0, alignedSize);
    
    return ptr;
}

void FrameAllocator::Reset() {
    m_usedSize = 0;
}

CommandQueue::CommandQueue(CommandQueueType type)
    : m_type(type)
    , m_device(NULL)
    , m_readOnly(NULL)
    , m_isImmutable(false)
    , m_executedCount(0)
{
}

CommandQueue::~CommandQueue() {
    Shutdown();
}

void CommandQueue::Initialize(LPDIRECT3DDEVICE9 device) {
    m_device = device;
    CMD_LOG("Initialized queue type %d", type);
}

void CommandQueue::Shutdown() {
    m_device = NULL;
    m_commands.clear();
    m_readOnly = NULL;
    m_isImmutable = false;
}

void CommandQueue::BeginFrame() {
    m_commands.clear();
    m_isImmutable = false;
    m_readOnly = NULL;
    m_executedCount = 0;
}

void CommandQueue::EndFrame() {
    m_isImmutable = false;
    m_readOnly = NULL;
}

void CommandQueue::Add(const RenderCommand& cmd) {
    if (m_isImmutable) {
        CMD_LOG("ERROR: Cannot add to immutable queue!");
        return;
    }
    m_commands.push_back(cmd);
}

void CommandQueue::AddImmutable(const RenderCommand& cmd) {
    m_commands.push_back(cmd);
}

void CommandQueue::SortByMaterial() {
    if (m_isImmutable) return;
    
    std::sort(m_commands.begin(), m_commands.end(), 
        [](const RenderCommand& a, const RenderCommand& b) -> bool {
            if (a.materialID != b.materialID) return a.materialID < b.materialID;
            return a.shaderID < b.shaderID;
        });
}

void CommandQueue::SortByDepth() {
    if (m_isImmutable) return;
    
    std::sort(m_commands.begin(), m_commands.end(),
        [](const RenderCommand& a, const RenderCommand& b) {
            return a.depth < b.depth;
        });
}

void CommandQueue::SortDeterministic() {
    if (m_isImmutable) return;
    
    std::sort(m_commands.begin(), m_commands.end(),
        [](const RenderCommand& a, const RenderCommand& b) -> bool {
            if (a.materialID != b.materialID) return a.materialID < b.materialID;
            if (a.shaderID != b.shaderID) return a.shaderID < b.shaderID;
            if (a.depth != b.depth) return a.depth < b.depth;
            return a.sortKey < b.sortKey;
        });
}

void CommandQueue::Execute(LPDIRECT3DDEVICE9 device) {
    if (!device) device = m_device;
    if (!device) return;
    
    if (m_readOnly) {
        ExecuteInternal(device, *m_readOnly);
    } else {
        ExecuteInternal(device, m_commands);
    }
    
    m_executedCount = (int)m_commands.size();
}

void CommandQueue::ExecuteInternal(LPDIRECT3DDEVICE9 device, const std::vector<RenderCommand>& cmds) {
    for (size_t i = 0; i < cmds.size(); i++) {
        const RenderCommand& cmd = cmds[i];
        
        if (cmd.pVertexBuffer) {
            device->SetStreamSource(0, cmd.pVertexBuffer, 0, SPRITE_VERTEX_STRIDE);
        }
        
        if (cmd.shaderID >= 0) {
        }
        
        // For now, just use DrawPrimitive with vertex count
        // TODO: Add index buffer support to RenderCommand if needed
        device->DrawPrimitive((D3DPRIMITIVETYPE)cmd.primType, cmd.vertexStart, cmd.primitiveCount);
    }
}

void CommandQueue::Clear() {
    if (!m_isImmutable) {
        m_commands.clear();
    }
}

void CommandQueue::MakeImmutable() {
    m_readOnly = &m_commands;
    m_isImmutable = true;
}

CommandQueueManager::CommandQueueManager()
    : m_device(NULL)
    , m_frameActive(false)
{
    for (int i = 0; i < QUEUE_COUNT; i++) {
        m_queues[i] = NULL;
    }
}

CommandQueueManager::~CommandQueueManager() {
    Shutdown();
}

void CommandQueueManager::Initialize(LPDIRECT3DDEVICE9 device) {
    m_device = device;
    
    for (int i = 0; i < QUEUE_COUNT; i++) {
        m_queues[i] = new CommandQueue((CommandQueueType)i);
        m_queues[i]->Initialize(device);
    }
    
    m_frameAllocator.Initialize(1024 * 1024);
    
    g_queueManager = this;
    CMD_LOG("Initialized");
}

void CommandQueueManager::Shutdown() {
    for (int i = 0; i < QUEUE_COUNT; i++) {
        if (m_queues[i]) {
            delete m_queues[i];
            m_queues[i] = NULL;
        }
    }
    
    m_frameAllocator.Shutdown();
    m_device = NULL;
    g_queueManager = NULL;
}

void CommandQueueManager::BeginFrame() {
    m_frameActive = true;
    
    for (int i = 0; i < QUEUE_COUNT; i++) {
        if (m_queues[i]) {
            m_queues[i]->BeginFrame();
        }
    }
    
    m_frameAllocator.Reset();
}

void CommandQueueManager::EndFrame() {
    m_frameActive = false;
    
    for (int i = 0; i < QUEUE_COUNT; i++) {
        if (m_queues[i]) {
            m_queues[i]->EndFrame();
        }
    }
    
    ValidateNoLeaks();
}

CommandQueue* CommandQueueManager::GetQueue(CommandQueueType type) {
    if (type >= 0 && type < QUEUE_COUNT) {
        return m_queues[type];
    }
    return NULL;
}

void CommandQueueManager::ExecuteAll() {
    for (int i = 0; i < QUEUE_COUNT; i++) {
        if (m_queues[i] && m_queues[i]->GetCommandCount() > 0) {
            m_queues[i]->Execute(m_device);
        }
    }
}

void CommandQueueManager::ClearAll() {
    for (int i = 0; i < QUEUE_COUNT; i++) {
        if (m_queues[i]) {
            m_queues[i]->Clear();
        }
    }
}

void CommandQueueManager::ValidateNoLeaks() {
#ifdef _DEBUG
    size_t used = m_frameAllocator.GetUsedSize();
    if (used > 0) {
        CMD_LOG("WARNING: FrameAllocator has %llu bytes used after frame", used);
    }
#endif
}

CommandQueueManager* GetGlobalQueueManager() {
    return g_queueManager;
}

void SetGlobalQueueManager(CommandQueueManager* mgr) {
    g_queueManager = mgr;
}

}