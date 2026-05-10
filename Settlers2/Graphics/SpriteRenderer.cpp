#include "stdafx.h"
#include "SpriteAtlas.h"
#include "SpriteRenderer.h"
#include "Renderer.h"
#include "Texture.h"
#include <stdio.h>
#include <string>
#include <algorithm>
#include <d3d9.h>
#include <d3dx9.h>
#include <math.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

// Silence debug logs in release build
#ifndef DISABLE_RENDER_LOGS
#define OutputDebugStringA(...) do { } while(0)
#endif

// Global worker thread data (accessible from worker threads)
ThreadData g_WorkerData[2];
HANDLE g_hStartWorkEvent = NULL;
HANDLE g_hThreadDoneEvents[2] = {NULL, NULL};
volatile bool g_KeepRunning = true;

// Worker thread function for Xbox 360 multi-threading
DWORD WINAPI SpriteWorkerThread(LPVOID lpParam) {
    int threadIdx = (int)lpParam;

    // Pin thread to specific core (Core 1 or Core 2)
    // Core 0 is reserved for main/render thread
    XSetThreadProcessor(GetCurrentThread(), threadIdx + 1);

    while (g_KeepRunning) {
        // Wait for command from main thread
        WaitForSingleObject(g_hStartWorkEvent, INFINITE);

        // Check if we should exit
        if (!g_KeepRunning) {
            break;
        }

        // Perform VMX calculations in Staging Buffer
        g_WorkerData[threadIdx].pRenderer->FillStagingAreaPublic(
            g_WorkerData[threadIdx].startIdx,
            g_WorkerData[threadIdx].count,
            g_WorkerData[threadIdx].source
        );

        // Signal completion of this thread's work
        SetEvent(g_hThreadDoneEvents[threadIdx]);
    }
    return 0;
}

// Configure vertex streams for hardware instancing (Stream 0: geometry, Stream 1: instance data)
void SpriteRenderer::SetupInstancingStates(int spriteCount) {
    // Stream 0: Geometry (Static Quad)
    m_pDevice->SetStreamSource(0, m_pStaticQuadVB, 0, 12);
    // Xbox 360: instancing parameters (no SetStreamSourceFreq on XDK)
    #ifdef _XBOX
    // On some Xbox SDKs, SetInstancingParameters is not available; skip if not defined
    #else
    m_pDevice->SetInstancingParameters(0, spriteCount);
    #endif
    // Stream 1: Per-instance data
    m_pDevice->SetStreamSource(1, m_pInstanceVB[m_activeBuffer], 0, sizeof(SpriteInstance));
}

SpriteRenderer::SpriteRenderer()
    : m_pDevice(NULL), m_pShaderManager(NULL), m_pIndexBuffer(NULL), m_pVertexDecl(NULL),
      m_pStagingBuffer(NULL), m_currentTexture(NULL), m_isBatching(false), m_maxSprites(4096), m_vertexBufferSize(0), m_indexBufferSize(0), m_spriteCount(0),
      m_hStartWorkEvent(NULL), m_threadsInitialized(false), m_activeBuffer(0),
      m_pStaticQuadVB(NULL), m_pStaticQuadIB(NULL), m_pInstancedDecl(NULL), m_pConstantInstancedDecl(NULL) 
{
    // Force 4096 max sprites for performance
    m_maxSprites = 4096;
    
    // Default rendering mode: constant instanced (unified shader approach)
    m_currentMode = MODE_INSTANCED_CONST;
    m_currentShaderName = "sprite_constant_instanced";
    
    // Initialize arrays inside constructor body
    m_hThreadDoneEvents[0] = NULL;
    m_hThreadDoneEvents[1] = NULL;
    m_hWorkerThreads[0] = NULL;
    m_hWorkerThreads[1] = NULL;
    m_pVB[0] = NULL;
    m_pVB[1] = NULL;
    m_pInstanceVB[0] = NULL;
    m_pInstanceVB[1] = NULL;
}

SpriteRenderer::~SpriteRenderer() {
    Shutdown();
}

HRESULT SpriteRenderer::Initialize(LPDIRECT3DDEVICE9 device, ShaderManager* shaderManager, int maxSprites) {
    HRESULT hr = S_OK;
    m_pDevice = device;
    m_pShaderManager = shaderManager;
    // Force 4096 for performance - ignore any passed value
    m_maxSprites = 4096;
    char buf[128];
    sprintf(buf, "[SR] Initialized with maxSprites=%d\n", m_maxSprites);
    OutputDebugStringA(buf);

    // Calculate vertex buffer size (4 vertices per sprite * sizeof(SpriteVertex))
    // FORCE 4096 sprites for buffer size calculation
    m_maxSprites = 4096;
    m_vertexBufferSize = m_maxSprites * 4 * sizeof(SpriteVertex);

    // Calculate index buffer size (6 indices per sprite * sizeof(WORD))
    m_indexBufferSize = m_maxSprites * 6 * sizeof(WORD);

    // Create DOUBLE vertex buffers for double-buffering (performance x2)
    for (int i = 0; i < 2; i++) {
        hr = m_pDevice->CreateVertexBuffer(
            m_vertexBufferSize,
            0,
            0,
            D3DPOOL_DEFAULT,
            &m_pVB[i],
            NULL
        );
        if (FAILED(hr)) {
            OutputDebugStringA("Failed to create vertex buffer\n");
            return hr;
        }
    }
    m_activeBuffer = 0;

    // Create index buffer - Xbox 360 compatible
    hr = m_pDevice->CreateIndexBuffer(
        m_indexBufferSize,
        0, // Xbox 360: no usage flags needed
        D3DFMT_INDEX16,
        D3DPOOL_DEFAULT,
        &m_pIndexBuffer,
        NULL
    );

    if (FAILED(hr)) {
        OutputDebugStringA("Failed to create index buffer\n");
        return hr;
    }

    // Pre-fill index buffer with quad indices (never changes)
    WORD* pIndices = NULL;
    hr = m_pIndexBuffer->Lock(0, 0, (void**)&pIndices, 0);
    if (SUCCEEDED(hr)) {
        for (int i = 0; i < m_maxSprites; i++) {
            int baseV = i * 4;
            int baseI = i * 6;
            // Triangle 1: 0-1-2
            pIndices[baseI + 0] = baseV + 0;
            pIndices[baseI + 1] = baseV + 1;
            pIndices[baseI + 2] = baseV + 2;
            // Triangle 2: 0-2-3
            pIndices[baseI + 3] = baseV + 0;
            pIndices[baseI + 4] = baseV + 2;
            pIndices[baseI + 5] = baseV + 3;
        }
        m_pIndexBuffer->Unlock();
    }

    // Create vertex declaration (must match SpriteVertex structure - 32 byte aligned)
    D3DVERTEXELEMENT9 decl[] = {
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
        { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 }, // Padding for 32-byte alignment
        D3DDECL_END()
    };

    hr = m_pDevice->CreateVertexDeclaration(decl, &m_pVertexDecl);
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to create vertex declaration\n");
        return hr;
    }

    // Allocate staging buffer in system memory (CPU-side), aligned for VMX (32 bytes for Xbox 360)
    // Used for multi-threaded rendering on Xbox 360
    // Use m_maxSprites to ensure enough space for 4096 sprites
    m_pStagingBuffer = (__declspec(align(32)) SpriteVertex*)_aligned_malloc(m_maxSprites * 4 * sizeof(SpriteVertex), 32);
    if (!m_pStagingBuffer) {
        OutputDebugStringA("Failed to allocate staging buffer\n");
        return E_OUTOFMEMORY;
    }
    // Zero the buffer to prevent garbage data causing BSOD
    memset(m_pStagingBuffer, 0, m_maxSprites * 4 * sizeof(SpriteVertex));

    // Reserve space in CPU mirror (4 vertices per sprite now)
    m_vertices.reserve(m_maxSprites * 4);

    // === Hardware Instancing Setup for Xbox 360 ===

    // Create static quad vertex buffer (4 vertices: 0,0 to 1,1)
    float quadVertices[] = {
        0.0f, 0.0f, 0.0f,  // Top-left
        1.0f, 0.0f, 0.0f,  // Top-right
        0.0f, 1.0f, 0.0f,  // Bottom-left
        1.0f, 1.0f, 0.0f   // Bottom-right
    };
    hr = m_pDevice->CreateVertexBuffer(sizeof(quadVertices), 0, 0, D3DPOOL_DEFAULT, &m_pStaticQuadVB, NULL);
    if (FAILED(hr)) {
        return hr;
    }
    void* pQuadData = NULL;
    m_pStaticQuadVB->Lock(0, sizeof(quadVertices), &pQuadData, 0);
    memcpy(pQuadData, quadVertices, sizeof(quadVertices));
    m_pStaticQuadVB->Unlock();

    // Create static quad index buffer (6 indices for 2 triangles)
    WORD quadIndices[] = { 0, 1, 2, 0, 2, 3 };
    hr = m_pDevice->CreateIndexBuffer(sizeof(quadIndices), 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_pStaticQuadIB, NULL);
    if (FAILED(hr)) {
        return hr;
    }
    void* pIndexData = NULL;
    m_pStaticQuadIB->Lock(0, sizeof(quadIndices), &pIndexData, 0);
    memcpy(pIndexData, quadIndices, sizeof(quadIndices));
    m_pStaticQuadIB->Unlock();

    // Create instance vertex buffers with double buffering
    int instanceBufferSize = m_maxSprites * sizeof(SpriteInstance);
    for (int i = 0; i < 2; i++) {
        hr = m_pDevice->CreateVertexBuffer(instanceBufferSize, 0, 0, D3DPOOL_DEFAULT, &m_pInstanceVB[i], NULL);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Create instanced vertex declaration (two streams)
    D3DVERTEXELEMENT9 instancingDecl[] = {
        // Stream 0: Base quad position (shared by all instances)
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        // Stream 1: Instance data (per-instance)
        { 1,  0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 }, // Pos/Size
        { 1, 16, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2 }, // UVs
        { 1, 32, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,  0 }, // Color
        D3DDECL_END()
    };
    hr = m_pDevice->CreateVertexDeclaration(instancingDecl, &m_pInstancedDecl);
    if (FAILED(hr)) { return hr; }

    // Create constant buffer instanced vertex declaration (POSITION-only on Stream 0)
    // This matches m_pStaticQuadVB which only has float3 positions (12 bytes per vertex)
    // Using m_pVertexDecl here would cause BSOD because it expects COLOR+TEXCOORD at offsets 12+16
    // which don't exist in the static quad VB
    D3DVERTEXELEMENT9 constantInstDecl[] = {
        { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        D3DDECL_END()
    };
    hr = m_pDevice->CreateVertexDeclaration(constantInstDecl, &m_pConstantInstancedDecl);
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to create constant instanced vertex declaration\n");
        return hr;
    }
    OutputDebugStringA("Created m_pConstantInstancedDecl successfully\n");

    // Initialization log removed

    return S_OK;
}

HRESULT SpriteRenderer::InitializeWorkerThreads() {
    if (m_threadsInitialized) {
        return S_OK;
    }

    // Create synchronization events
    m_hStartWorkEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hThreadDoneEvents[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hThreadDoneEvents[1] = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!m_hStartWorkEvent || !m_hThreadDoneEvents[0] || !m_hThreadDoneEvents[1]) {
        OutputDebugStringA("Failed to create synchronization events\n");
        return E_FAIL;
    }

    // Set global event handles for worker threads
    g_hStartWorkEvent = m_hStartWorkEvent;
    g_hThreadDoneEvents[0] = m_hThreadDoneEvents[0];
    g_hThreadDoneEvents[1] = m_hThreadDoneEvents[1];

    // Create worker threads (Core 1 and Core 2)
    DWORD threadId;
    m_hWorkerThreads[0] = CreateThread(NULL, 0, SpriteWorkerThread, (LPVOID)0, 0, &threadId);
    m_hWorkerThreads[1] = CreateThread(NULL, 0, SpriteWorkerThread, (LPVOID)1, 0, &threadId);

    if (!m_hWorkerThreads[0] || !m_hWorkerThreads[1]) {
        OutputDebugStringA("Failed to create worker threads\n");
        return E_FAIL;
    }

    m_threadsInitialized = true;
    OutputDebugStringA("Worker threads initialized (Core 1, Core 2)\n");

    return S_OK;
}

void SpriteRenderer::ShutdownWorkerThreads() {
    if (!m_threadsInitialized) {
        return;
    }

    // Signal worker threads to exit gracefully
    g_KeepRunning = false;
    SetEvent(m_hStartWorkEvent); // Use SetEvent instead of PulseEvent for Xbox 360 safety

    // Wait for worker threads to exit
    if (m_hWorkerThreads[0]) {
        WaitForSingleObject(m_hWorkerThreads[0], 500);
        CloseHandle(m_hWorkerThreads[0]);
        m_hWorkerThreads[0] = NULL;
    }
    if (m_hWorkerThreads[1]) {
        WaitForSingleObject(m_hWorkerThreads[1], 500);
        CloseHandle(m_hWorkerThreads[1]);
        m_hWorkerThreads[1] = NULL;
    }

    // Close event handles
    if (m_hStartWorkEvent) {
        CloseHandle(m_hStartWorkEvent);
        m_hStartWorkEvent = NULL;
    }
    if (m_hThreadDoneEvents[0]) {
        CloseHandle(m_hThreadDoneEvents[0]);
        m_hThreadDoneEvents[0] = NULL;
    }
    if (m_hThreadDoneEvents[1]) {
        CloseHandle(m_hThreadDoneEvents[1]);
        m_hThreadDoneEvents[1] = NULL;
    }

    g_hStartWorkEvent = NULL;
    g_hThreadDoneEvents[0] = NULL;
    g_hThreadDoneEvents[1] = NULL;

    m_threadsInitialized = false;
    OutputDebugStringA("Worker threads shutdown\n");
}

void SpriteRenderer::Shutdown() {
    ShutdownWorkerThreads();

    if (m_pStagingBuffer) {
        _aligned_free(m_pStagingBuffer);
        m_pStagingBuffer = NULL;
    }

    // Release both vertex buffers for double buffering
    for (int i = 0; i < 2; i++) {
        if (m_pVB[i]) {
            m_pVB[i]->Release();
            m_pVB[i] = NULL;
        }
    }

    if (m_pVertexDecl) {
        m_pVertexDecl->Release();
        m_pVertexDecl = NULL;
    }

    if (m_pIndexBuffer) {
        m_pIndexBuffer->Release();
        m_pIndexBuffer = NULL;
    }

    // Release instancing buffers
    if (m_pStaticQuadVB) {
        m_pStaticQuadVB->Release();
        m_pStaticQuadVB = NULL;
    }
    if (m_pStaticQuadIB) {
        m_pStaticQuadIB->Release();
        m_pStaticQuadIB = NULL;
    }
    for (int i = 0; i < 2; i++) {
        if (m_pInstanceVB[i]) {
            m_pInstanceVB[i]->Release();
            m_pInstanceVB[i] = NULL;
        }
    }
    if (m_pInstancedDecl) {
        m_pInstancedDecl->Release();
        m_pInstancedDecl = NULL;
    }
    if (m_pConstantInstancedDecl) {
        m_pConstantInstancedDecl->Release();
        m_pConstantInstancedDecl = NULL;
    }

    m_vertices.clear();
    m_currentTexture = NULL;
    m_currentShaderName.clear();
    m_isBatching = false;
    m_pShaderManager = NULL;
    m_pDevice = NULL;
}

void SpriteRenderer::OnLostDevice() {
    // Release vertex buffer
    if (m_pVB[0]) {
        m_pVB[0]->Release();
        m_pVB[0] = NULL;
    }
    if (m_pVB[1]) {
        m_pVB[1]->Release();
        m_pVB[1] = NULL;
    }

    // Release instancing buffers
    if (m_pStaticQuadVB) {
        m_pStaticQuadVB->Release();
        m_pStaticQuadVB = NULL;
    }
    if (m_pStaticQuadIB) {
        m_pStaticQuadIB->Release();
        m_pStaticQuadIB = NULL;
    }
    for (int i = 0; i < 2; i++) {
        if (m_pInstanceVB[i]) {
            m_pInstanceVB[i]->Release();
            m_pInstanceVB[i] = NULL;
        }
    }

    // Staging buffer is in system memory, doesn't need to be released
    // Index buffer is also D3DPOOL_DEFAULT, release it too
    if (m_pIndexBuffer) {
        m_pIndexBuffer->Release();
        m_pIndexBuffer = NULL;
    }

    // Vertex declarations must be released before Reset on Xbox 360
    if (m_pVertexDecl) {
        m_pVertexDecl->Release();
        m_pVertexDecl = NULL;
    }
    if (m_pInstancedDecl) {
        m_pInstancedDecl->Release();
        m_pInstancedDecl = NULL;
    }
    if (m_pConstantInstancedDecl) {
        m_pConstantInstancedDecl->Release();
        m_pConstantInstancedDecl = NULL;
    }
}

void SpriteRenderer::OnResetDevice() {

    HRESULT hr = S_OK;

    // Release old resources first to prevent memory leaks
    for (int i = 0; i < 2; i++) {
        if (m_pVB[i]) { m_pVB[i]->Release(); m_pVB[i] = NULL; }
    }
    if (m_pIndexBuffer) { m_pIndexBuffer->Release(); m_pIndexBuffer = NULL; }
    if (m_pStaticQuadVB) { m_pStaticQuadVB->Release(); m_pStaticQuadVB = NULL; }
    if (m_pStaticQuadIB) { m_pStaticQuadIB->Release(); m_pStaticQuadIB = NULL; }
    for (int i = 0; i < 2; i++) {
        if (m_pInstanceVB[i]) { m_pInstanceVB[i]->Release(); m_pInstanceVB[i] = NULL; }
    }
    if (m_pVertexDecl) { m_pVertexDecl->Release(); m_pVertexDecl = NULL; }
    if (m_pInstancedDecl) { m_pInstancedDecl->Release(); m_pInstancedDecl = NULL; }
    if (m_pConstantInstancedDecl) { m_pConstantInstancedDecl->Release(); m_pConstantInstancedDecl = NULL; }

    // Recreate DOUBLE vertex buffers for double-buffering
    for (int i = 0; i < 2; i++) {
        hr = m_pDevice->CreateVertexBuffer(
            m_vertexBufferSize,
            0,
            0,
            D3DPOOL_DEFAULT,
            &m_pVB[i],
            NULL
        );
        if (FAILED(hr)) {
            return;
        }
    }

    // Recreate index buffer
    hr = m_pDevice->CreateIndexBuffer(
        m_indexBufferSize,
        0,
        D3DFMT_INDEX16,
        D3DPOOL_DEFAULT,
        &m_pIndexBuffer,
        NULL
    );

    if (FAILED(hr)) { return; }

    // Pre-fill index buffer with quad indices
    WORD* pIndices = NULL;
    hr = m_pIndexBuffer->Lock(0, 0, (void**)&pIndices, 0);
    if (SUCCEEDED(hr)) {
        for (int i = 0; i < m_maxSprites; i++) {
            int baseV = i * 4;
            int baseI = i * 6;
            pIndices[baseI + 0] = baseV + 0;
            pIndices[baseI + 1] = baseV + 1;
            pIndices[baseI + 2] = baseV + 2;
            pIndices[baseI + 3] = baseV + 0;
            pIndices[baseI + 4] = baseV + 2;
            pIndices[baseI + 5] = baseV + 3;
        }
        m_pIndexBuffer->Unlock();
    }

    // Reset active buffer
    m_activeBuffer = 0;

    // Recreate instancing buffers
    float quadVertices[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f
    };
    hr = m_pDevice->CreateVertexBuffer(sizeof(quadVertices), 0, 0, D3DPOOL_DEFAULT, &m_pStaticQuadVB, NULL);
    if (FAILED(hr)) {
        OutputDebugStringA("[SpriteRenderer] Failed to recreate static quad VB after reset\n");
        return;
    }
    void* pQuadData = NULL;
    m_pStaticQuadVB->Lock(0, sizeof(quadVertices), &pQuadData, 0);
    memcpy(pQuadData, quadVertices, sizeof(quadVertices));
    m_pStaticQuadVB->Unlock();

    WORD quadIndices[] = { 0, 1, 2, 0, 2, 3 };
    hr = m_pDevice->CreateIndexBuffer(sizeof(quadIndices), 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_pStaticQuadIB, NULL);
    if (FAILED(hr)) {
        OutputDebugStringA("[SpriteRenderer] Failed to recreate static quad IB after reset\n");
        return;
    }
    void* pIndexData = NULL;
    m_pStaticQuadIB->Lock(0, sizeof(quadIndices), &pIndexData, 0);
    memcpy(pIndexData, quadIndices, sizeof(quadIndices));
    m_pStaticQuadIB->Unlock();

    int instanceBufferSize = m_maxSprites * sizeof(SpriteInstance);
    for (int i = 0; i < 2; i++) {
        hr = m_pDevice->CreateVertexBuffer(instanceBufferSize, 0, 0, D3DPOOL_DEFAULT, &m_pInstanceVB[i], NULL);
        if (FAILED(hr)) {
            OutputDebugStringA("[SpriteRenderer] Failed to recreate instance VB after reset\n");
            return;
        }
    }

    // Recreate standard vertex declaration (32-byte aligned)
    D3DVERTEXELEMENT9 decl[] = {
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
        { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 }, // Padding for 32-byte alignment
        D3DDECL_END()
    };
    hr = m_pDevice->CreateVertexDeclaration(decl, &m_pVertexDecl);
    if (FAILED(hr)) {
        OutputDebugStringA("[SpriteRenderer] Failed to recreate standard declaration after reset\n");
        return;
    }

    D3DVERTEXELEMENT9 instancingDecl[] = {
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 1,  0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
        { 1, 16, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2 },
        { 1, 32, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,  0 },
        D3DDECL_END()
    };
    hr = m_pDevice->CreateVertexDeclaration(instancingDecl, &m_pInstancedDecl);
    if (FAILED(hr)) {
        OutputDebugStringA("[SpriteRenderer] Failed to recreate instanced declaration after reset\n");
        return;
    }

    // Recreate constant buffer instanced vertex declaration (POSITION-only)
    D3DVERTEXELEMENT9 constantInstDecl[] = {
        { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        D3DDECL_END()
    };
    hr = m_pDevice->CreateVertexDeclaration(constantInstDecl, &m_pConstantInstancedDecl);
    if (FAILED(hr)) {
        OutputDebugStringA("[SpriteRenderer] Failed to recreate constant instanced declaration after reset\n");
        return;
    }

    
}

void SpriteRenderer::Begin(const char* shaderName, LPDIRECT3DTEXTURE9 pTexture) {

    // If state changed, flush previous batch
    if (m_isBatching) {
        if (m_currentShaderName != shaderName || m_currentTexture != pTexture) {
            Flush();
        }
    }

    m_currentShaderName = shaderName;
    m_currentTexture = pTexture;
    m_isBatching = true;
    m_spriteCount = 0;

}

void SpriteRenderer::Draw(float x, float y, float width, float height,
                          float u0, float v0, float u1, float v1,
                          DWORD color) {
    if (!m_isBatching) {
        OutputDebugStringA("Draw called without Begin!\n");
        return;
    }

    // Check if buffer is full (do this FIRST to prevent overflow)
    if (m_spriteCount >= m_maxSprites) {
        char warnMsg[128];
        sprintf(warnMsg, "WARNING: Sprite limit reached (%d sprites)! Flushing...\n", m_maxSprites);
        OutputDebugStringA(warnMsg);
        Flush();
    }

    // Check for buffer overflow
    if ((size_t)m_spriteCount * 4 * sizeof(SpriteVertex) >= (size_t)m_vertexBufferSize) {
        char errBuf[128];
        sprintf(errBuf, "BSOD PREVENTED: Staging buffer overflow! spriteCount=%d, max=%d\n", m_spriteCount, m_maxSprites);
        OutputDebugStringA(errBuf);
        return;
    }

    CreateQuad(x, y, width, height, u0, v0, u1, v1, color);
}

void SpriteRenderer::DrawWithTexture(float x, float y, float width, float height,
                                     float u0, float v0, float u1, float v1,
                                     LPDIRECT3DTEXTURE9 pTexture,
                                     DWORD color) {
    if (!m_isBatching) {
        OutputDebugStringA("DrawWithTexture called without Begin!\n");
        return;
    }

    // Flush batch if texture changed (enables per-sprite texture switching)
    if (pTexture != m_currentTexture) {
        if (m_spriteCount > 0) {
            Flush();
        }
        m_currentTexture = pTexture;
        m_pDevice->SetTexture(0, pTexture);
    }

    Draw(x, y, width, height, u0, v0, u1, v1, color);
}

void SpriteRenderer::DrawRotated(float x, float y, float width, float height, float angle,
                                 float u0, float v0, float u1, float v1,
                                 DWORD color) {
    if (!m_isBatching) {
        OutputDebugStringA("DrawRotated called without Begin!\n");
        return;
    }

    // Check for buffer overflow
    if ((size_t)m_spriteCount * 4 * sizeof(SpriteVertex) >= (size_t)m_vertexBufferSize) {
        char errBuf[128];
        sprintf(errBuf, "BSOD PREVENTED: Staging buffer overflow! spriteCount=%d, max=%d\n", m_spriteCount, m_maxSprites);
        OutputDebugStringA(errBuf);
        return;
    }

    // Check if buffer is full
    if (m_spriteCount >= m_maxSprites) {
        char warnMsg[128];
        sprintf(warnMsg, "WARNING: Sprite limit reached (%d sprites)! Flushing...\n", m_maxSprites);
        OutputDebugStringA(warnMsg);
        Flush();
    }

    CreateQuadRotated(x, y, width, height, angle, u0, v0, u1, v1, color);
}

void SpriteRenderer::End() {
    if (m_isBatching) {
        Flush();
        m_isBatching = false;
    }
    if (m_pDevice) {
        m_pDevice->SetTexture(0, NULL);
    }
}

void SpriteRenderer::Flush(ShaderManager* pShader) {
    if (m_spriteCount == 0) return;

    // DEBUG LOG: Monitor vertex count to detect buffer overflow
    int vertexCount = m_spriteCount * 4;
    char debugMsg[256];
    sprintf(debugMsg, "[SpriteRenderer::Flush] spriteCount=%d, vertexCount=%d, maxSprites=%d\n", 
            m_spriteCount, vertexCount, m_maxSprites);
    OutputDebugStringA(debugMsg);

    // Use strategy pattern: select rendering path based on current mode
    switch (m_currentMode) {
        case MODE_STANDARD:
            // Standard path: CPU prepares all vertices, single DrawIndexedPrimitive
            FlushStandard();
            break;
        case MODE_INSTANCED_STREAM:
            // Two-stream instancing: geometry + instance data
            if (m_pShaderManager) {
                m_pShaderManager->SetActiveShader("sprite_instanced");
                m_pShaderManager->BeginShader();
                m_pShaderManager->BeginPass(0);
                m_pShaderManager->SetTexture("g_texture", m_currentTexture);
                m_pShaderManager->Commit();
            }
            FlushInstanced();
            if (m_pShaderManager) {
                m_pShaderManager->EndPass();
                m_pShaderManager->EndShader();
            }
            break;
        case MODE_INSTANCED_CONST:
            // Constant buffer instancing: uses g_InstanceData[200] registers
            if (m_pShaderManager) {
                m_pShaderManager->SetActiveShader("sprite_constant_instanced");
                m_pShaderManager->BeginShader();
                m_pShaderManager->BeginPass(0);
                m_pShaderManager->SetTexture("g_texture", m_currentTexture);
                m_pShaderManager->Commit();
            }
            FlushConstantInstanced();
            if (m_pShaderManager) {
                m_pShaderManager->EndPass();
                m_pShaderManager->EndShader();
            }
            break;
    }

    // Switch buffer for next frame (double buffering)
    m_activeBuffer = (m_activeBuffer + 1) % 2;
    m_spriteCount = 0;
}

void SpriteRenderer::Flush() {
    if (m_spriteCount == 0) return;

    // Just call the version with shader manager - it handles all modes including MODE_STANDARD
    Flush(m_pShaderManager);
}



void SpriteRenderer::UpdateAndFlush(const SpriteData* allSprites, int totalCount) {
    if (!m_threadsInitialized) {
        OutputDebugStringA("Worker threads not initialized, falling back to single-threaded\n");
        // Fallback to single-threaded rendering
        for (int i = 0; i < totalCount; i++) {
            DrawRotated(allSprites[i].x, allSprites[i].y, allSprites[i].width, allSprites[i].height,
                       allSprites[i].angle, allSprites[i].u0, allSprites[i].v0, allSprites[i].u1, allSprites[i].v1,
                       allSprites[i].color);
        }
        Flush();
        return;
    }

    int half = totalCount / 2;

    // 1. Distribute data to worker threads
    m_workerData[0].startIdx = 0;
    m_workerData[0].count = half;
    m_workerData[0].source = allSprites;
    m_workerData[0].pRenderer = this;

    m_workerData[1].startIdx = half;
    m_workerData[1].count = totalCount - half;
    m_workerData[1].source = allSprites;
    m_workerData[1].pRenderer = this;

    // Copy to global data for worker threads
    g_WorkerData[0] = m_workerData[0];
    g_WorkerData[1] = m_workerData[1];

    // 2. Reset completion events and signal start
    ResetEvent(m_hThreadDoneEvents[0]);
    ResetEvent(m_hThreadDoneEvents[1]);
    PulseEvent(m_hStartWorkEvent); // Wake both threads simultaneously

    // 3. Wait for BOTH threads to complete (Core 0 can do other work here)
    WaitForMultipleObjects(2, m_hThreadDoneEvents, TRUE, INFINITE);

    // 4. Now that Staging Buffer is 100% filled, copy to GPU and flush
    m_spriteCount = totalCount;

    // Use buffer 0 (single buffer mode)
    LPDIRECT3DVERTEXBUFFER9 currentVB = m_pVB[0];

    // Lock vertex buffer and copy from staging buffer
    void* pData = NULL;
    HRESULT hr = currentVB->Lock(0, m_spriteCount * 4 * sizeof(SpriteVertex), &pData, 0);
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to lock vertex buffer\n");
        m_spriteCount = 0;
        return;
    }

    // Copy from staging buffer to GPU
    memcpy(pData, m_pStagingBuffer, m_spriteCount * 4 * sizeof(SpriteVertex));
    currentVB->Unlock();

    // Set vertex declaration
    m_pDevice->SetVertexDeclaration(m_pVertexDecl);
    m_pDevice->SetStreamSource(0, currentVB, 0, 32); // Hardcoded 32-byte stride for Xbox 360 alignment
    m_pDevice->SetIndices(m_pIndexBuffer);

    // Set active shader
    if (m_pShaderManager) {
        m_pShaderManager->BeginShader();
        m_pShaderManager->BeginPass(0);

        // Set texture and matrix (no auto-commit)
        m_pShaderManager->SetTexture("g_texture", m_currentTexture);
        m_pShaderManager->Commit(); // Explicit commit for Xbox 360
    }

    // Draw indexed primitives
    m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, m_spriteCount * 4, 0, m_spriteCount * 2);

    // End shader pass
    if (m_pShaderManager) {
        m_pShaderManager->EndPass();
        m_pShaderManager->EndShader();
    }

    // Keep single buffer, just reset count
    m_spriteCount = 0;
}

void SpriteRenderer::RenderBatch(const SpriteData* pData, int count) {
    if (count <= 0 || !m_threadsInitialized) return;

    // Divide work between 2 worker threads
    int half = count / 2;

    g_WorkerData[0].startIdx = 0;
    g_WorkerData[0].count = half;
    g_WorkerData[0].source = pData;
    g_WorkerData[0].pRenderer = this;

    g_WorkerData[1].startIdx = half;
    g_WorkerData[1].count = count - half;
    g_WorkerData[1].source = pData;
    g_WorkerData[1].pRenderer = this;

    // Trigger workers
    SetEvent(m_hStartWorkEvent);

    // Wait for Core 1 and Core 2 to finish
    WaitForMultipleObjects(2, m_hThreadDoneEvents, TRUE, INFINITE);

    // Now the staging buffer is full, move to GPU and Draw
    m_spriteCount = count;
    Flush();
}

void SpriteRenderer::DrawSprite(SpriteAtlas* atlas, const char* spriteName, float x, float y, DWORD color) {
    if (!atlas) return;

    // Fast lookup of sprite index by name
    uint32_t spriteIdx = atlas->GetIndex(spriteName);
    if (spriteIdx == (uint32_t)-1) {
        char buf[256];
        sprintf(buf, "Sprite not found in atlas: %s\n", spriteName);
        OutputDebugStringA(buf);
        return;
    }

    // Call the optimized indexed version
    DrawIndexedSprite(atlas, spriteIdx, x, y, color);
}

void SpriteRenderer::DrawIndexedSprite(SpriteAtlas* atlas, uint32_t spriteIndex, float x, float y, DWORD color) {
    if (!atlas) return;

    // Fast vector access by index (no map lookup)
    const SpriteRegion* reg = atlas->GetRegion(spriteIndex);
    if (!reg) return;

    // Handle flipX/flipY on CPU for Xbox 360 optimization
    float finalU0 = reg->u0;
    float finalU1 = reg->u1;
    float finalV0 = reg->v0;
    float finalV1 = reg->v1;

    if (reg->flipX) {
        finalU0 = reg->u1;
        finalU1 = reg->u0;
    }
    if (reg->flipY) {
        finalV0 = reg->v1;
        finalV1 = reg->v0;
    }

    // Call the batched draw with UV coordinates from atlas (with flip applied)
    Draw(x, y, (float)reg->width, (float)reg->height,
         finalU0, finalV0, finalU1, finalV1, color);
}

void SpriteRenderer::DrawAtlasSprite(SpriteAtlas* atlas, uint32_t regionIndex, float x, float y, DWORD color) {
    if (!atlas) return;

    const SpriteRegion* reg = atlas->GetRegion(regionIndex);
    if (!reg) {
        // Region not found - skip this sprite to prevent exception
        return;
    }

    Draw(x, y, (float)reg->width, (float)reg->height, reg->u0, reg->v0, reg->u1, reg->v1, color);
}

void SpriteRenderer::Submit(const char* shader, SpriteAtlas* atlas, uint32_t spriteIdx, float x, float y, float depth, DWORD color) {
    if (!atlas) return;

    const SpriteRegion* reg = atlas->GetRegion(spriteIdx);
    if (!reg) return;

    int sid = GetShaderId(shader);
    RenderCommand cmd;
    cmd.shaderId = sid;
    cmd.pAtlas = atlas;
    cmd.x = x - reg->pivotX;
    cmd.y = y - reg->pivotY;
    cmd.w = (float)reg->width;
    cmd.h = (float)reg->height;
    cmd.u0 = reg->u0; cmd.v0 = reg->v0;
    cmd.u1 = reg->u1; cmd.v1 = reg->v1;
    cmd.color = color;
    cmd.depth = depth;

    m_renderQueue.push_back(cmd);
}

int SpriteRenderer::GetShaderId(const char* name) {
    if (!name) return -1;
    for (size_t i = 0; i < m_shaderNameRegistry.size(); ++i) {
        if (m_shaderNameRegistry[i] == std::string(name)) {
            return (int)i;
        }
    }
    m_shaderNameRegistry.push_back(std::string(name));
    return (int)m_shaderNameRegistry.size() - 1;
}
void SpriteRenderer::InternalDraw(const RenderCommand& cmd) {
    CreateQuad(cmd.x, cmd.y, cmd.w, cmd.h, cmd.u0, cmd.v0, cmd.u1, cmd.v1, cmd.color);
}

// Public wrapper for atlas sprite (compatibility wrapper in case direct call is private)
void SpriteRenderer::DrawAtlasSpritePublic(SpriteAtlas* atlas, uint32_t index, float x, float y, DWORD color) {
    DrawAtlasSprite(atlas, index, x, y, color);
}

void SpriteRenderer::FillStagingAreaPublic(int startIdx, int count, const SpriteData* data) {
    FillStagingArea(startIdx, count, data);
}

// (removed duplicate wrappers to keep single source of truth)

void SpriteRenderer::ExecuteBatch() {
    if (m_renderQueue.empty()) return;

    // Sort all commands (std::sort is very fast on Xenon)
    std::sort(m_renderQueue.begin(), m_renderQueue.end());

    // Process sorted list by shaderId -> atlas -> depth
    int currentShaderId = -1;
    SpriteAtlas* currentAtlas = NULL;
    const char* currentShaderName = NULL;

    for (size_t i = 0; i < m_renderQueue.size(); ++i) {
        const RenderCommand& cmd = m_renderQueue[i];

        if (cmd.shaderId != currentShaderId || cmd.pAtlas != currentAtlas) {
            // Flush previous batch
            Flush();

            // Update state for new shader/atlas
            currentShaderId = cmd.shaderId;
            currentAtlas = cmd.pAtlas;
            currentShaderName = (currentShaderId >= 0 && currentShaderId < (int)m_shaderNameRegistry.size())
                                ? m_shaderNameRegistry[currentShaderId].c_str()
                                : NULL;

            if (currentShaderName && m_pShaderManager) {
                m_pShaderManager->SetActiveShader(currentShaderName);
                m_pShaderManager->SetTexture("g_texture", currentAtlas->GetTexture());
                m_pShaderManager->BeginShader();
                m_pShaderManager->BeginPass(0);
                m_pShaderManager->Commit();
            }
        }

        InternalDraw(cmd);
        if (m_spriteCount >= m_maxSprites) Flush();
    }

    Flush(); // Final flush
    m_renderQueue.clear();
}

void SpriteRenderer::SubmitInstanced(const char* shader, SpriteAtlas* atlas, uint32_t spriteIdx, float x, float y, float depth, DWORD color) {
    if (!atlas) return;

    const SpriteRegion* reg = atlas->GetRegion(spriteIdx);
    if (!reg) return;

    SpriteInstance inst;
    inst.positionSize[0] = x - reg->pivotX;
    inst.positionSize[1] = y - reg->pivotY;
    inst.positionSize[2] = (float)reg->width;
    inst.positionSize[3] = (float)reg->height;
    inst.uvRect[0] = reg->u0;
    inst.uvRect[1] = reg->v0;
    inst.uvRect[2] = reg->u1;
    inst.uvRect[3] = reg->v1;
    inst.color = color;

    m_instanceQueue.push_back(inst);
}

void SpriteRenderer::FlushInstanced() {
    if (m_instanceQueue.empty()) return;

    // Check for NULL buffers to prevent BSOD
    if (m_pStaticQuadVB == NULL) {
        OutputDebugStringA("BSOD PREVENTED: m_pStaticQuadVB is NULL in FlushInstanced!\n");
        return;
    }
    if (m_pStaticQuadIB == NULL) {
        OutputDebugStringA("BSOD PREVENTED: m_pStaticQuadIB is NULL in FlushInstanced!\n");
        return;
    }
    if (m_pVertexDecl == NULL) {
        OutputDebugStringA("BSOD PREVENTED: m_pVertexDecl is NULL in FlushInstanced!\n");
        return;
    }

    // Xbox 360 doesn't use SetStreamSourceFreq.
    // We use c10+ registers to pass instance data (Constant Buffer Instancing).
    static D3DVECTOR4 constantBuffer[256];

    int totalSprites = (int)m_instanceQueue.size();
    int processed = 0;

    // Set the static quad (Stream 0)
    m_pDevice->SetStreamSource(0, m_pStaticQuadVB, 0, sizeof(float) * 3);
    m_pDevice->SetIndices(m_pStaticQuadIB);
    m_pDevice->SetVertexDeclaration(m_pVertexDecl);

    // Process in chunks (registers c10 - c250 = 240 registers = 120 sprites)
    while (processed < totalSprites) {
        int batchSize = totalSprites - processed;
        if (batchSize > 120) batchSize = 120;

        // Pack instance data from m_instanceQueue into D3DVECTOR4 format
        for (int i = 0; i < batchSize; ++i) {
            int spriteIdx = processed + i;

            // Register 1: Position and Size (x, y, w, h)
            constantBuffer[i * 2].x = m_instanceQueue[spriteIdx].positionSize[0];
            constantBuffer[i * 2].y = m_instanceQueue[spriteIdx].positionSize[1];
            constantBuffer[i * 2].z = m_instanceQueue[spriteIdx].positionSize[2];
            constantBuffer[i * 2].w = m_instanceQueue[spriteIdx].positionSize[3];

            // Register 2: UVs (u0, v0, u1, v1)
            constantBuffer[i * 2 + 1].x = m_instanceQueue[spriteIdx].uvRect[0];
            constantBuffer[i * 2 + 1].y = m_instanceQueue[spriteIdx].uvRect[1];
            constantBuffer[i * 2 + 1].z = m_instanceQueue[spriteIdx].uvRect[2];
            constantBuffer[i * 2 + 1].w = m_instanceQueue[spriteIdx].uvRect[3];
        }

        // Upload to VS registers starting at c10
        m_pDevice->SetVertexShaderConstantF(10, (float*)constantBuffer, batchSize * 2);

        // Standard 6-argument Draw call for Xbox 360
        m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2);

        processed += batchSize;
    }

    m_instanceQueue.clear();
}

void SpriteRenderer::FlushConstantInstanced() {
    if (m_instanceQueue.empty()) return;

    // Check for NULL buffers to prevent BSOD
    if (m_pStaticQuadVB == NULL) {
        OutputDebugStringA("BSOD PREVENTED: m_pStaticQuadVB is NULL!\n");
        return;
    }
    if (m_pStaticQuadIB == NULL) {
        OutputDebugStringA("BSOD PREVENTED: m_pStaticQuadIB is NULL!\n");
        return;
    }
    if (m_pConstantInstancedDecl == NULL) {
        OutputDebugStringA("BSOD PREVENTED: m_pConstantInstancedDecl is NULL!\n");
        return;
    }

    // Use D3DVECTOR4 (Xbox 360 standard 128-bit vector type)
    // 2 registers per sprite: [x, y, w, h] and [u0, v0, u1, v1]
    static D3DVECTOR4 constantBuffer[256];

    int totalSprites = (int)m_instanceQueue.size();
    int processed = 0;

    // CRITICAL: Use m_pConstantInstancedDecl (POSITION-only) instead of m_pVertexDecl
    // m_pVertexDecl expects POSITION+COLOR+TEXCOORD (24 bytes) but m_pStaticQuadVB
    // only has POSITION (12 bytes). Mismatch = GPU reads garbage = BSOD on Xbox 360!
    m_pDevice->SetStreamSource(0, m_pStaticQuadVB, 0, sizeof(float) * 3);
    m_pDevice->SetIndices(m_pStaticQuadIB);
    m_pDevice->SetVertexDeclaration(m_pConstantInstancedDecl);

    // Process in chunks (shader has g_InstanceData[200] = 100 sprites max, 2 registers per sprite)
    while (processed < totalSprites) {
        int batchSize = totalSprites - processed;
        if (batchSize > 100) batchSize = 100;

        // Pack data from m_instanceQueue into D3DVECTOR4 format
        for (int i = 0; i < batchSize; ++i) {
            int spriteIdx = processed + i;

            // Register 1: Position and Size (x, y, w, h)
            constantBuffer[i * 2].x = m_instanceQueue[spriteIdx].positionSize[0];
            constantBuffer[i * 2].y = m_instanceQueue[spriteIdx].positionSize[1];
            constantBuffer[i * 2].z = m_instanceQueue[spriteIdx].positionSize[2];
            constantBuffer[i * 2].w = m_instanceQueue[spriteIdx].positionSize[3];

            // Register 2: UVs (u0, v0, u1, v1)
            constantBuffer[i * 2 + 1].x = m_instanceQueue[spriteIdx].uvRect[0];
            constantBuffer[i * 2 + 1].y = m_instanceQueue[spriteIdx].uvRect[1];
            constantBuffer[i * 2 + 1].z = m_instanceQueue[spriteIdx].uvRect[2];
            constantBuffer[i * 2 + 1].w = m_instanceQueue[spriteIdx].uvRect[3];
        }

        // Upload to Vertex Shader Constant Registers starting at c10
        m_pDevice->SetVertexShaderConstantF(10, (float*)constantBuffer, batchSize * 2);

        // Draw the batch using standard Xbox 360 DrawIndexedPrimitive (6 arguments)
        m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2);

        processed += batchSize;
    }

    m_instanceQueue.clear();
}

void SpriteRenderer::FlushStandard() {
    if (m_spriteCount == 0) return;

    // 1. Check for NULL buffers
    if (m_pStagingBuffer == NULL || m_pVB[m_activeBuffer] == NULL || 
        m_pIndexBuffer == NULL || m_pVertexDecl == NULL) {
        OutputDebugStringA("BSOD PREVENTED: Buffer/NULL in FlushStandard!\n");
        return;
    }

    // 2. Wait for worker threads to finish (CRITICAL!)
    // Must wait for all sprite data to be ready in staging buffer before copying to GPU
    if (m_threadsInitialized && m_hThreadDoneEvents[0] && m_hThreadDoneEvents[1]) {
        WaitForMultipleObjects(2, m_hThreadDoneEvents, TRUE, INFINITE);
    }

    // 3. Lock VB and copy from staging buffer
    void* pData = NULL;
    if (SUCCEEDED(m_pVB[m_activeBuffer]->Lock(0, m_spriteCount * 4 * sizeof(SpriteVertex), &pData, D3DLOCK_NOOVERWRITE))) {
        memcpy(pData, m_pStagingBuffer, m_spriteCount * 4 * sizeof(SpriteVertex));
        m_pVB[m_activeBuffer]->Unlock();
    } else {
        return;
    }

    // 4. Set states
    m_pDevice->SetVertexDeclaration(m_pVertexDecl);
    m_pDevice->SetStreamSource(0, m_pVB[m_activeBuffer], 0, 32); // Hardcoded 32-byte stride for Xbox 360 alignment
    m_pDevice->SetIndices(m_pIndexBuffer);

    // 5. Shader management (if needed)
    if (m_pShaderManager && !m_currentShaderName.empty()) {
        char debugMsg[256];
        sprintf(debugMsg, "[SR] FlushStandard: Using shader '%s'\n", m_currentShaderName.c_str());
        OutputDebugStringA(debugMsg);

        m_pShaderManager->SetActiveShader(m_currentShaderName.c_str());
        m_pShaderManager->BeginShader();
        m_pShaderManager->BeginPass(0);

        // Set texture with correct parameter name based on shader
        if (m_currentShaderName == "Basic2D") {
            m_pShaderManager->SetTexture("tex", m_currentTexture);
        } else {
            m_pShaderManager->SetTexture("g_texture", m_currentTexture);
        }

        // Set orthographic projection matrix for 2D sprites
        D3DXMATRIX matOrtho;
        D3DXMatrixOrthoOffCenterLH(&matOrtho, 0.0f, 1280.0f, 720.0f, 0.0f, 0.0f, 1.0f);
        m_pShaderManager->SetMatrix("matOrtho", (const float*)&matOrtho);

        sprintf(debugMsg, "[SR] FlushStandard: Setting texture 0x%p\n", m_currentTexture);
        OutputDebugStringA(debugMsg);

        m_pShaderManager->Commit();
    } else {
        OutputDebugStringA("[SR] FlushStandard: No shader manager or empty shader name\n");
    }
    m_pDevice->SetTexture(0, m_currentTexture);

    // 6. Draw
    m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, m_spriteCount * 4, 0, m_spriteCount * 2);

    // 7. End shader pass
    if (m_pShaderManager && !m_currentShaderName.empty()) {
        m_pShaderManager->EndPass();
        m_pShaderManager->EndShader();
    }
}

void SpriteRenderer::FlushConstantInstancedHybrid() {
    if (m_spriteCount == 0) return;

    // Check for NULL buffers to prevent BSOD
    if (m_pStaticQuadVB == NULL) {
        OutputDebugStringA("BSOD PREVENTED: m_pStaticQuadVB is NULL in Hybrid!\n");
        return;
    }
    if (m_pStaticQuadIB == NULL) {
        OutputDebugStringA("BSOD PREVENTED: m_pStaticQuadIB is NULL in Hybrid!\n");
        return;
    }
    if (m_pConstantInstancedDecl == NULL) {
        OutputDebugStringA("BSOD PREVENTED: m_pConstantInstancedDecl is NULL in Hybrid!\n");
        return;
    }
    if (m_pStagingBuffer == NULL) {
        OutputDebugStringA("BSOD PREVENTED: m_pStagingBuffer is NULL in Hybrid!\n");
        return;
    }

    // CRITICAL: Use m_pConstantInstancedDecl (POSITION-only) instead of m_pVertexDecl
    // m_pVertexDecl expects POSITION+COLOR+TEXCOORD but m_pStaticQuadVB only has POSITION
    m_pDevice->SetVertexDeclaration(m_pConstantInstancedDecl);
    m_pDevice->SetStreamSource(0, m_pStaticQuadVB, 0, sizeof(float) * 3);
    m_pDevice->SetIndices(m_pStaticQuadIB);

    m_pShaderManager->BeginShader();
    m_pShaderManager->BeginPass(0);  // Only Pass 0 exists now
    m_pShaderManager->SetTexture("g_texture", m_currentTexture);
    m_pShaderManager->Commit();

    // Draw each sprite individually with its own constant data
    // This avoids the vIdx/instID problem: on Xbox 360, vIdx gives vertex index (0-3),
    // NOT instance index. Drawing per-sprite with constant at c10[0] is safe.
    for (int i = 0; i < m_spriteCount; i++) {
        int idx = i * 4;
        D3DVECTOR4 spriteData[2];

        // Register 0: [x, y, w, h]
        spriteData[0].x = m_pStagingBuffer[idx].x;
        spriteData[0].y = m_pStagingBuffer[idx].y;
        spriteData[0].z = m_pStagingBuffer[idx + 1].x - m_pStagingBuffer[idx].x; // width
        spriteData[0].w = m_pStagingBuffer[idx + 2].y - m_pStagingBuffer[idx].y; // height

        // Register 1: [u0, v0, u1, v1]
        spriteData[1].x = m_pStagingBuffer[idx].u;
        spriteData[1].y = m_pStagingBuffer[idx].v;
        spriteData[1].z = m_pStagingBuffer[idx + 2].u;
        spriteData[1].w = m_pStagingBuffer[idx + 2].v;

        m_pDevice->SetVertexShaderConstantF(10, (float*)spriteData, 2);
        m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2);
    }

    m_pShaderManager->EndPass();
    m_pShaderManager->EndShader();
}

void SpriteRenderer::FillStagingArea(int startIdx, int count, const SpriteData* sourceData) {
    // This function is called from worker threads (Core 1, Core 2)
    // Each thread writes to its part of m_stagingBuffer
    for (int i = 0; i < count; ++i) {
        int spriteIdx = startIdx + i;
        const SpriteData& sprite = sourceData[spriteIdx];
        SpriteVertex* v = &m_pStagingBuffer[spriteIdx * 4];

        // If angle is 0, use fast path, otherwise use rotation
        if (sprite.angle == 0.0f) {
            float x0 = sprite.x - 0.5f;
            float x1 = (sprite.x + sprite.width) - 0.5f;
            float y0 = sprite.y - 0.5f;
            float y1 = (sprite.y + sprite.height) - 0.5f;

            v[0].x = x0; v[0].y = y0; v[0].z = 0.0f; v[0].color = sprite.color; v[0].u = sprite.u0; v[0].v = sprite.v0;
            v[1].x = x1; v[1].y = y0; v[1].z = 0.0f; v[1].color = sprite.color; v[1].u = sprite.u1; v[1].v = sprite.v0;
            v[2].x = x1; v[2].y = y1; v[2].z = 0.0f; v[2].color = sprite.color; v[2].u = sprite.u1; v[2].v = sprite.v1;
            v[3].x = x0; v[3].y = y1; v[3].z = 0.0f; v[3].color = sprite.color; v[3].u = sprite.u0; v[3].v = sprite.v1;
        } else {
            // Call the VMX optimized rotation here
            ComputeRotatedVerticesVMX(sprite, v);
        }
    }
}

void SpriteRenderer::CreateQuad(float x, float y, float width, float height,
                                float u0, float v0, float u1, float v1,
                                DWORD color) {
    // Check for overflow before writing to staging buffer
    if (m_spriteCount >= m_maxSprites) {
        char errBuf[128];
        sprintf(errBuf, "BSOD PREVENTED: CreateQuad overflow! spriteCount=%d, max=%d\n", m_spriteCount, m_maxSprites);
        OutputDebugStringA(errBuf);
        return;
    }

    // Calculate quad corners with half-pixel offset for pixel-perfect sprites
    float x0 = x - 0.5f;
    float x1 = (x + width) - 0.5f;
    float y0 = y - 0.5f;
    float y1 = (y + height) - 0.5f;
    float z = 0.0f;

    // Write DIRECTLY to staging buffer (not m_vertices!)
    // FlushStandard reads from m_pStagingBuffer, not m_vertices
    SpriteVertex* v = &m_pStagingBuffer[m_spriteCount * 4];

    v[0].x = x0; v[0].y = y0; v[0].z = z; v[0].color = color; v[0].u = u0; v[0].v = v0;
    v[1].x = x1; v[1].y = y0; v[1].z = z; v[1].color = color; v[1].u = u1; v[1].v = v0;
    v[2].x = x1; v[2].y = y1; v[2].z = z; v[2].color = color; v[2].u = u1; v[2].v = v1;
    v[3].x = x0; v[3].y = y1; v[3].z = z; v[3].color = color; v[3].u = u0; v[3].v = v1;

    m_spriteCount++;
}

void SpriteRenderer::CreateQuadRotated(float x, float y, float width, float height, float angle,
                                       float u0, float v0, float u1, float v1,
                                       DWORD color) {
    // Check for overflow before writing to staging buffer
    if (m_spriteCount >= m_maxSprites) {
        return;
    }

    // Calculate center
    float centerX = x + width * 0.5f;
    float centerY = y + height * 0.5f;
    float z = 0.0f;

    // Calculate sin/cos
    float sinA = sinf(angle);
    float cosA = cosf(angle);

    // Half dimensions
    float halfW = width * 0.5f;
    float halfH = height * 0.5f;

    // Local corners relative to center
    float localX[4] = {-halfW,  halfW,  halfW, -halfW};
    float localY[4] = {-halfH, -halfH,  halfH,  halfH};

    // Rotate and translate
    float finalX[4], finalY[4];
    for (int i = 0; i < 4; i++) {
        finalX[i] = localX[i] * cosA - localY[i] * sinA + centerX - 0.5f;
        finalY[i] = localX[i] * sinA + localY[i] * cosA + centerY - 0.5f;
    }

    // UV coordinates
    float u[4] = {u0, u1, u1, u0};
    float v[4] = {v0, v0, v1, v1};

    // Write DIRECTLY to staging buffer (not m_vertices!)
    SpriteVertex* vOut = &m_pStagingBuffer[m_spriteCount * 4];
    for (int i = 0; i < 4; i++) {
        vOut[i].x = finalX[i];
        vOut[i].y = finalY[i];
        vOut[i].z = z;
        vOut[i].color = color;
        vOut[i].u = u[i];
        vOut[i].v = v[i];
    }
    m_spriteCount++;
}

void SpriteRenderer::ComputeRotatedVerticesVMX(const SpriteData& data, SpriteVertex* pOutVertices) {
    // Calculate center
    float centerX = data.x + data.width * 0.5f;
    float centerY = data.y + data.height * 0.5f;
    float z = 0.0f;

    // Calculate sin/cos
    float sinA = sinf(data.angle);
    float cosA = cosf(data.angle);

    // Half dimensions
    float halfW = data.width * 0.5f;
    float halfH = data.height * 0.5f;

    // Local corners relative to center
    float localX[4] = {-halfW,  halfW,  halfW, -halfW};
    float localY[4] = {-halfH, -halfH,  halfH,  halfH};

    // Rotate and translate
    float finalX[4], finalY[4];
    for (int i = 0; i < 4; i++) {
        finalX[i] = localX[i] * cosA - localY[i] * sinA + centerX - 0.5f;
        finalY[i] = localX[i] * sinA + localY[i] * cosA + centerY - 0.5f;
    }

    // UV coordinates
    float u[4] = {data.u0, data.u1, data.u1, data.u0};
    float v[4] = {data.v0, data.v0, data.v1, data.v1};

    // Create 4 vertices directly to output (no vector copy)
    for (int i = 0; i < 4; i++) {
        pOutVertices[i].x = finalX[i];
        pOutVertices[i].y = finalY[i];
        pOutVertices[i].z = z;
        pOutVertices[i].color = data.color;
        pOutVertices[i].u = u[i];
        pOutVertices[i].v = v[i];
    }
}
