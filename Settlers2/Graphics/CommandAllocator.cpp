#include "stdafx.h"
#include "CommandAllocator.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

FrameLinearAllocator::FrameLinearAllocator()
    : m_pDevice(NULL)
    , m_pBase(NULL)
    , m_capacity(0)
    , m_usedSize(0)
{
}

FrameLinearAllocator::~FrameLinearAllocator() {
    Shutdown();
}

void FrameLinearAllocator::Initialize(IDirect3DDevice9* pDevice, size_t capacity) {
    m_pDevice = pDevice;
    m_capacity = capacity;
    m_usedSize = 0;

    if (capacity > 0) {
        m_pBase = new char[capacity];
        memset(m_pBase, 0, capacity);
        OutputDebugStringA("[FrameLinearAllocator] Initialized\n");
    }
}

void FrameLinearAllocator::Shutdown() {
    if (m_pBase) {
        delete[] (char*)m_pBase;
        m_pBase = NULL;
    }
    m_capacity = 0;
    m_usedSize = 0;
}

void* FrameLinearAllocator::Allocate(size_t size, size_t alignment) {
    if (!m_pBase) return NULL;

    size_t alignedSize = (size + alignment - 1) & ~(alignment - 1);
    
    if (m_usedSize + alignedSize > m_capacity) {
        OutputDebugStringA("[FrameLinearAllocator] WARNING: Out of memory!\n");
        return NULL;
    }

    void* ptr = (char*)m_pBase + m_usedSize;
    m_usedSize += alignedSize;

    return ptr;
}

void FrameLinearAllocator::Reset() {
    m_usedSize = 0;
}

DoubleBufferedCommandQueue::DoubleBufferedCommandQueue()
    : m_pDevice(NULL)
    , m_pBufferA(NULL)
    , m_pBufferB(NULL)
    , m_pCurrent(NULL)
    , m_bufferSize(0)
    , m_currentFrame(0)
{
}

DoubleBufferedCommandQueue::~DoubleBufferedCommandQueue() {
    Shutdown();
}

void DoubleBufferedCommandQueue::Initialize(IDirect3DDevice9* pDevice, size_t bufferSize) {
    m_pDevice = pDevice;
    m_bufferSize = bufferSize;

    m_pBufferA = new char[bufferSize];
    m_pBufferB = new char[bufferSize];
    m_pCurrent = m_pBufferA;

    OutputDebugStringA("[DoubleBufferedCommandQueue] Initialized\n");
}

void DoubleBufferedCommandQueue::Shutdown() {
    if (m_pBufferA) { delete[] (char*)m_pBufferA; m_pBufferA = NULL; }
    if (m_pBufferB) { delete[] (char*)m_pBufferB; m_pBufferB = NULL; }
    m_pCurrent = NULL;
    m_bufferSize = 0;
}

void* DoubleBufferedCommandQueue::GetCurrentBuffer() {
    return m_pCurrent;
}

void* DoubleBufferedCommandQueue::GetPreviousBuffer() {
    return m_pCurrent == m_pBufferA ? m_pBufferB : m_pBufferA;
}

void DoubleBufferedCommandQueue::Flip() {
    m_pCurrent = (m_pCurrent == m_pBufferA) ? m_pBufferB : m_pBufferA;
    m_currentFrame++;
}

bool DoubleBufferedCommandQueue::IsCurrentReady() const {
    return true;
}

void DoubleBufferedCommandQueue::WaitForGpu() {
}

Xbox360CommandBuffer::Xbox360CommandBuffer()
    : m_pDevice(NULL)
    , m_frameCount(0)
{
}

Xbox360CommandBuffer::~Xbox360CommandBuffer() {
    Shutdown();
}

void Xbox360CommandBuffer::Initialize(IDirect3DDevice9* pDevice) {
    m_pDevice = pDevice;
    m_allocator.Initialize(pDevice, 64 * 1024);
    m_queue.Initialize(pDevice, 64 * 1024);
    OutputDebugStringA("[Xbox360CommandBuffer] Initialized\n");
}

void Xbox360CommandBuffer::Shutdown() {
    m_allocator.Shutdown();
    m_queue.Shutdown();
    m_pDevice = NULL;
}

void Xbox360CommandBuffer::BeginFrame() {
    m_allocator.Reset();
    m_queue.Flip();
}

void Xbox360CommandBuffer::EndFrame() {
    m_frameCount++;
}

void Xbox360CommandBuffer::Execute() {
    OutputDebugStringA("[Xbox360CommandBuffer] Execute\n");
}

}