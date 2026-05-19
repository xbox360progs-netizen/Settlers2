#pragma once
#include <d3d9.h>
#include <vector>

namespace Graphics {

enum AllocatorPolicy {
    ALLOCATOR_LINEAR,
    ALLOCATOR_RING,
    ALLOCATOR_STACK
};

class FrameLinearAllocator {
public:
    FrameLinearAllocator();
    ~FrameLinearAllocator();

    void Initialize(IDirect3DDevice9* pDevice, size_t capacity);
    void Shutdown();

    void* Allocate(size_t size, size_t alignment = 16);
    void Reset();
    void* GetBase() const { return m_pBase; }
    size_t GetUsedSize() const { return m_usedSize; }
    size_t GetCapacity() const { return m_capacity; }

private:
    IDirect3DDevice9* m_pDevice;
    void* m_pBase;
    size_t m_capacity;
    size_t m_usedSize;
};

class DoubleBufferedCommandQueue {
public:
    DoubleBufferedCommandQueue();
    ~DoubleBufferedCommandQueue();

    void Initialize(IDirect3DDevice9* pDevice, size_t bufferSize);
    void Shutdown();

    void* GetCurrentBuffer();
    void* GetPreviousBuffer();
    void Flip();

    bool IsCurrentReady() const;
    void WaitForGpu();

    size_t GetBufferSize() const { return m_bufferSize; }

private:
    IDirect3DDevice9* m_pDevice;
    void* m_pBufferA;
    void* m_pBufferB;
    void* m_pCurrent;
    size_t m_bufferSize;
    int m_currentFrame;
};

class Xbox360CommandBuffer {
public:
    Xbox360CommandBuffer();
    ~Xbox360CommandBuffer();

    void Initialize(IDirect3DDevice9* pDevice);
    void Shutdown();

    void BeginFrame();
    void EndFrame();
    void Execute();

    template<typename T>
    T* AllocCommand() {
        size_t align = alignof(T);
        void* ptr = m_allocator.Allocate(sizeof(T), align);
        return new (ptr) T();
    }

    FrameLinearAllocator* GetAllocator() { return &m_allocator; }

private:
    IDirect3DDevice9* m_pDevice;
    FrameLinearAllocator m_allocator;
    DoubleBufferedCommandQueue m_queue;
    int m_frameCount;
};

}