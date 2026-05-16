#include "stdafx.h"
#include "SpriteRenderer.h"
#include "ShaderManager.h"
#include "Renderer.h"
#include "../Scene/SceneManager.h"
#include <stdio.h>
#include <string>
#include <vector>
#include <algorithm>
#include <d3d9.h>
#include <d3dx9.h>
#include <math.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

#ifndef DISABLE_RENDER_LOGS
#define OutputDebugStringA(...) do { } while(0)
#endif

using namespace Scene;

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
      m_pGpuBufferA(NULL), m_pGpuBufferB(NULL), m_currentBackBufferIndex(0), m_pVertexBuffer(NULL),
      m_pStaticQuadVB(NULL), m_pStaticQuadIB(NULL), m_pInstancedDecl(NULL), m_pConstantInstancedDecl(NULL)
#ifdef _XBOX
      , m_pAsyncCommandBuffer(NULL)
      , m_pAsyncCall(NULL)
      , m_pGpuFence(NULL)
      , m_isFirstFlush(true)
#endif
{
    // Force 4096 max sprites for performance
    m_maxSprites = 4096;

    // Default rendering mode: use World shader for world-space rendering
    m_currentMode = MODE_INSTANCED_CONST;
    m_currentShaderID = SHADER_WORLD; // Use World shader for world-space objects
    m_currentDepth = 1.0f; // Default: far layer (ground)
    m_currentRenderType = 0; // Default to Single
    m_currentIsUI = false; // Default: world-space (apply camera matrix)
    m_totalVertexCount = 0;
    m_totalIndexCount = 0; // Reset index count at start

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
    char buf[256];
    sprintf(buf, "[SR::Initialize] this=%p, m_pDevice=%p, m_pShaderManager=%p, maxSprites=%d\n", this, m_pDevice, m_pShaderManager, m_maxSprites);
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
        sprintf(buf, "[SR::Initialize] CreateVertexBuffer[%d] hr=0x%08X, pVB=%p\n", i, hr, m_pVB[i]);
        OutputDebugStringA(buf);
        if (FAILED(hr)) {
            OutputDebugStringA("[SR::Initialize] FAILED to create vertex buffer\n");
            return hr;
        }
    }
    m_activeBuffer = 0;

    // Create explicit GPU buffers A and B for ping-pong multi-threaded rendering
    // These are the same as m_pVB[0] and m_pVB[1], but named explicitly for clarity
    m_pGpuBufferA = m_pVB[0];
    m_pGpuBufferB = m_pVB[1];
    m_currentBackBufferIndex = 0;
    sprintf(buf, "[SR::Initialize] Ping-pong buffers: m_pGpuBufferA=%p, m_pGpuBufferB=%p, backIndex=%d\n",
            m_pGpuBufferA, m_pGpuBufferB, m_currentBackBufferIndex);
    OutputDebugStringA(buf);

    // Create SINGLE LARGE GPU buffer for ring buffer with D3DLOCK_NOOVERWRITE
    // Buffer size: 3x max sprites to accommodate ring buffer cycling
    int ringBufferSize = m_maxSprites * 4 * 3 * sizeof(SpriteVertex);
    hr = m_pDevice->CreateVertexBuffer(
        ringBufferSize,
        0,
        0,
        D3DPOOL_DEFAULT,
        &m_pVertexBuffer,
        NULL
    );
    sprintf(buf, "[SR::Initialize] CreateRingBuffer hr=0x%08X, pVertexBuffer=%p, size=%d\n", hr, m_pVertexBuffer, ringBufferSize);
    OutputDebugStringA(buf);
    if (FAILED(hr)) {
        OutputDebugStringA("[SR::Initialize] FAILED to create ring buffer GPU buffer\n");
        return hr;
    }

    // Create index buffer - Xbox 360 compatible
    hr = m_pDevice->CreateIndexBuffer(
        m_indexBufferSize,
        0, // Xbox 360: no usage flags needed
        D3DFMT_INDEX16,
        D3DPOOL_DEFAULT,
        &m_pIndexBuffer,
        NULL
    );
    sprintf(buf, "[SR::Initialize] CreateIndexBuffer hr=0x%08X, pIB=%p, size=%d\n", hr, m_pIndexBuffer, m_indexBufferSize);
    OutputDebugStringA(buf);

    if (FAILED(hr)) {
        OutputDebugStringA("[SR::Initialize] FAILED to create index buffer\n");
        return hr;
    }

    // Pre-fill index buffer with quad indices (never changes)
    WORD* pIndices = NULL;
    hr = m_pIndexBuffer->Lock(0, 0, (void**)&pIndices, 0);
    sprintf(buf, "[SR::Initialize] IndexBuffer Lock hr=0x%08X, pIndices=%p\n", hr, pIndices);
    OutputDebugStringA(buf);
    if (SUCCEEDED(hr)) {
        for (int i = 0; i < m_maxSprites; i++) {
            int baseI = i * 6;
            // LOCAL indices for each sprite (0-3 pattern)
            // baseVertex in DrawIndexedPrimitive will shift them to correct position
            pIndices[baseI + 0] = 0;
            pIndices[baseI + 1] = 1;
            pIndices[baseI + 2] = 3;
            pIndices[baseI + 3] = 1;
            pIndices[baseI + 4] = 3;
            pIndices[baseI + 5] = 2;
        }
        m_pIndexBuffer->Unlock();
        OutputDebugStringA("[SR::Initialize] Index buffer filled with LOCAL indices and unlocked\n");
    }

    // Create vertex declaration (must match SpriteVertex structure - 32 byte aligned)
    // SpriteVertex: float x,y,z (0) | float u,v (12) | DWORD color (20) | float padding[2] (24)
    // FontShader.fx expects: POSITION (0) | TEXCOORD0 (12) | COLOR0 (20) | TEXCOORD1 (24)
    D3DVERTEXELEMENT9 decl[] = {
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },    // x,y,z
        { 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },    // u,v
        { 0, 20, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },     // color
        { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },    // padding[2]
        D3DDECL_END()
    };

    hr = m_pDevice->CreateVertexDeclaration(decl, &m_pVertexDecl);
    sprintf(buf, "[SR::Initialize] CreateVertexDeclaration hr=0x%08X, pDecl=%p\n", hr, m_pVertexDecl);
    OutputDebugStringA(buf);
    if (FAILED(hr)) {
        OutputDebugStringA("[SR::Initialize] FAILED to create vertex declaration\n");
        return hr;
    }

    // Allocate staging buffer in system memory (CPU-side), aligned for VMX (32 bytes for Xbox 360)
    m_pStagingBuffer = (__declspec(align(32)) SpriteVertex*)_aligned_malloc(m_maxSprites * 4 * sizeof(SpriteVertex), 32);
    sprintf(buf, "[SR::Initialize] StagingBuffer=%p, size=%d\n", m_pStagingBuffer, m_maxSprites * 4 * (int)sizeof(SpriteVertex));
    OutputDebugStringA(buf);
    if (!m_pStagingBuffer) {
        OutputDebugStringA("[SR::Initialize] FAILED to allocate staging buffer\n");
        return E_OUTOFMEMORY;
    }
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

    // Create constant buffer instanced vertex declaration (POSITION-only)
    // We'll draw each sprite individually with constant data, no instID needed
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

    // Set projection matrix for sprite rendering (1280x720 orthographic)
    if (m_pShaderManager) {
        D3DXMATRIX ortho;
        D3DXMatrixOrthoOffCenterLH(&ortho, 0, 1280, 720, 0, 0, 1);

        // Set projection matrix for SHADER_SPRITE (ID 0)
        ShaderManager::Shader* pSpriteShader = m_pShaderManager->GetShader(SHADER_SPRITE);
        if (pSpriteShader && pSpriteShader->pEffect) {
            D3DXHANDLE hWVP = pSpriteShader->pEffect->GetParameterByName(NULL, "WVP");
            if (hWVP) {
                pSpriteShader->pEffect->SetMatrix(hWVP, &ortho);
            }
            D3DXHANDLE hMatOrtho = pSpriteShader->pEffect->GetParameterByName(NULL, "matOrtho");
            if (hMatOrtho) {
                pSpriteShader->pEffect->SetMatrix(hMatOrtho, &ortho);
            }
            OutputDebugStringA("[SR] Projection matrix set for SHADER_SPRITE\n");
        }
    }

    // Итоговый лог Initialize
    sprintf(buf, "[SR::Initialize] COMPLETE this=%p, m_pDevice=%p, m_pVB[0]=%p, m_pVB[1]=%p, m_pIndexBuffer=%p, m_pVertexDecl=%p, m_pStagingBuffer=%p\n",
            this, m_pDevice, m_pVB[0], m_pVB[1], m_pIndexBuffer, m_pVertexDecl, m_pStagingBuffer);
    OutputDebugStringA(buf);

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

#ifdef _XBOX
    // Clean up async command buffer objects
    if (m_pAsyncCommandBuffer) {
        m_pAsyncCommandBuffer->Release();
        m_pAsyncCommandBuffer = NULL;
    }
    // Note: m_pAsyncCall is owned by SceneManager, so we don't release it here
    m_pAsyncCall = NULL;
    
    // Release GPU fence
    if (m_pGpuFence) {
        m_pGpuFence->Release();
        m_pGpuFence = NULL;
    }
#endif

    m_vertices.clear();
    m_currentTexture = NULL;
    m_currentShaderID = SHADER_INVALID;
    m_isBatching = false;
    m_pShaderManager = NULL;
    m_pDevice = NULL;
}

#ifdef _XBOX
void SpriteRenderer::SetAsyncCommandBuffer(IDirect3DCommandBuffer9* pBuffer, IDirect3DAsyncCommandBufferCall9* pAsyncCall) {
    m_pAsyncCommandBuffer = pBuffer;
    m_pAsyncCall = pAsyncCall;
    
    // Create GPU fence query for CPU/GPU synchronization
    if (m_pDevice && !m_pGpuFence) {
        HRESULT hr = m_pDevice->CreateQuery(D3DQUERYTYPE_EVENT, &m_pGpuFence);
        if (FAILED(hr) || !m_pGpuFence) {
            OutputDebugStringA("[SpriteRenderer] ERROR: Failed to create D3DQUERYTYPE_EVENT Fence!\n");
            m_pGpuFence = NULL;
        } else {
            OutputDebugStringA("[SpriteRenderer] SUCCESS: D3DQUERYTYPE_EVENT Fence created successfully\n");
        }
    }
    
    OutputDebugStringA("[SpriteRenderer] Async command buffer set\n");
}

void SpriteRenderer::FlushBatchesAsync() {
    if (m_spriteCount == 0) {
        return;
    }

    // [XBOX 360 SYNC] Wait for GPU to finish previous frame using fence
    // Skip on first call - GPU hasn't processed anything yet
    if (m_pGpuFence && !m_isFirstFlush) {
        while (m_pGpuFence->GetData(NULL, 0, D3DGETDATA_FLUSH) == S_FALSE) {
            #ifdef _XBOX
            YieldProcessor();
            #else
            __nop();
            #endif
            Sleep(0);
        }
    }
    m_isFirstFlush = false; // Mark first flush complete

    // GUARD: Check scene graphics ready flag
    if (!::Scene::SceneManager::Instance() || !::Scene::SceneManager::Instance()->IsGraphicsReady()) {
        OutputDebugStringA("[SpriteRenderer::FlushBatchesAsync] Graphics not ready, skipping flush\n");
        return;
    }

    if (!m_pAsyncCommandBuffer || !m_pAsyncCall) {
        OutputDebugStringA("[SpriteRenderer::FlushBatchesAsync] ERROR: Async command buffer not initialized!\n");
        return;
    }

    char dbg[256];
    sprintf(dbg, "[SR::FlushBatchesAsync] spriteCount=%d, vertexCount=%d\n", m_spriteCount, m_spriteCount * 4);
    OutputDebugStringA(dbg);

    // Check all D3D resources before any draw call
    if (!m_pDevice || !m_pVertexBuffer || !m_pIndexBuffer || !m_pVertexDecl) {
        sprintf(dbg, "[SR::FlushBatchesAsync] ERROR: NULL res - pDev=%p, pVB=%p, pIB=%p, pDecl=%p\n",
                m_pDevice, m_pVertexBuffer, m_pIndexBuffer, m_pVertexDecl);
        OutputDebugStringA(dbg);
        return;
    }

    OutputDebugStringA("[Flush] Step 1: BeginCommandBuffer...\n");
    m_pDevice->BeginCommandBuffer(m_pAsyncCommandBuffer, 0, NULL, NULL, NULL, 0);

    // [VERTEX RING BUFFER] Reset offset if buffer would overflow
    DWORD vertexCount = m_spriteCount * 4;
    if (m_totalVertexCount + vertexCount >= MAX_BUFFER_VERTICES) {
        OutputDebugStringA("[SpriteRenderer] Vertex Ring Buffer wrapped around to 0\n");
        m_totalVertexCount = 0;
        m_totalIndexCount = 0;
    }

    DWORD vertexOffset = m_totalVertexCount;
    DWORD indexOffset = m_totalIndexCount;
    DWORD primitiveCount = m_spriteCount * 2;

    // Apply shader and texture (recorded into buffer)
    if (m_pShaderManager) {
        OutputDebugStringA("[Flush] Step 2: BeginShader...\n");
        m_pShaderManager->BeginShader();
        OutputDebugStringA("[Flush] Step 3: BeginPass...\n");
        m_pShaderManager->BeginPass(0);
        OutputDebugStringA("[Flush] Step 4: SetTexture...\n");
        m_pShaderManager->SetTexture("g_texture", m_currentTexture);
        OutputDebugStringA("[Flush] Step 5: Commit...\n");
        m_pShaderManager->Commit();
    }

    // Set vertex and index buffers
    uint32_t stride = sizeof(SpriteVertex);
    OutputDebugStringA("[Flush] Step 6: SetStreamSource...\n");
    m_pDevice->SetStreamSource(0, m_pVertexBuffer, 0, stride);
    OutputDebugStringA("[Flush] Step 7: SetIndices...\n");
    m_pDevice->SetIndices(m_pIndexBuffer);
    OutputDebugStringA("[Flush] Step 8: SetVertexDeclaration...\n");
    m_pDevice->SetVertexDeclaration(m_pVertexDecl);

    // Draw indexed primitives
    OutputDebugStringA("[Flush] Step 9: DrawIndexedPrimitive...\n");
    m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, vertexOffset, 0, vertexCount, indexOffset, primitiveCount);
    OutputDebugStringA("[Flush] Step 10: Draw done!\n");

    // End shader pass
    if (m_pShaderManager) {
        OutputDebugStringA("[Flush] Step 11: EndPass...\n");
        m_pShaderManager->EndPass();
        OutputDebugStringA("[Flush] Step 12: EndShader...\n");
        m_pShaderManager->EndShader();
    }

    // End recording and submit to GPU
    OutputDebugStringA("[Flush] Step 13: EndCommandBuffer...\n");
    m_pDevice->EndCommandBuffer();

    // Issue GPU fence marker for next frame's synchronization
    if (m_pGpuFence) {
        m_pGpuFence->Issue(D3DISSUE_END);
    }

    // Send buffer to Xenon GPU
    HRESULT hr = m_pAsyncCall->FixupAndSignal(m_pAsyncCommandBuffer, 0, 0);
    if (FAILED(hr)) {
        sprintf(dbg, "[SR::FlushBatchesAsync] ERROR: FixupAndSignal failed with HRESULT=0x%08X\n", hr);
        OutputDebugStringA(dbg);
    }

    // Update ring buffer offsets for NEXT batch
    m_totalVertexCount += vertexCount;
    m_totalIndexCount += primitiveCount * 3;

    // Reset batch counter - CPU ready to collect new sprites
    m_spriteCount = 0;
}
#endif

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

    // Pre-fill index buffer with LOCAL quad indices (0,1,2,3,0,2,3 pattern)
    // baseVertex in DrawIndexedPrimitive will shift them to correct position
    WORD* pIndices = NULL;
    hr = m_pIndexBuffer->Lock(0, 0, (void**)&pIndices, 0);
    if (SUCCEEDED(hr)) {
        for (int i = 0; i < m_maxSprites; i++) {
            int baseI = i * 6;
            // LOCAL indices for each sprite (0-3 pattern)
            pIndices[baseI + 0] = 0;
            pIndices[baseI + 1] = 1;
            pIndices[baseI + 2] = 2;
            pIndices[baseI + 3] = 0;
            pIndices[baseI + 4] = 2;
            pIndices[baseI + 5] = 3;
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
    // Must match FontShader.fx: POSITION (0) | TEXCOORD0 (12) | COLOR0 (20) | TEXCOORD1 (24)
    D3DVERTEXELEMENT9 decl[] = {
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },    // x,y,z
        { 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },    // u,v
        { 0, 20, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },     // color
        { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },    // padding[2]
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

static int ShaderNameToID(const char* shaderName) {
    if (!shaderName) return SHADER_INVALID;
    if (strcmp(shaderName, "sprite") == 0) return SHADER_SPRITE;
    if (strcmp(shaderName, "sprite_constant_instanced") == 0) return SHADER_SPRITE_CONSTANT_INSTANCED;
    if (strcmp(shaderName, "RadialMenu") == 0) return SHADER_RADIALMENU;
    return SHADER_SPRITE; // Default fallback
}

// New Begin overloads with shaderID
void SpriteRenderer::Begin(ShaderID shaderID, LPDIRECT3DTEXTURE9 pTexture) {
    OutputDebugStringA("[SR::Begin] ShaderID version CALLED\n");
    fflush(stdout);
    Begin(shaderID, pTexture, 1.0f, 0, false); // Default: far layer, Single, world-space
}

void SpriteRenderer::Begin(ShaderID shaderID, LPDIRECT3DTEXTURE9 pTexture, float depth) {
    Begin(shaderID, pTexture, depth, 0, false); // Default: Single, world-space
}

void SpriteRenderer::Begin(ShaderID shaderID, LPDIRECT3DTEXTURE9 pTexture, float depth, int renderType) {
    Begin(shaderID, pTexture, depth, renderType, false); // Default: world-space
}

void SpriteRenderer::Begin(ShaderID shaderID, LPDIRECT3DTEXTURE9 pTexture, float depth, int renderType, bool isUI) {
    OutputDebugStringA("[SR::Begin] 5-PARAM VERSION ENTERED NOW!!!\n");
    fflush(stdout);

    // === ШАГ 1: БЕЗОПАСНАЯ ИЗОЛЯЦИЯ ПРЕДЫДУЩЕГО БАТЧА ===
    // Если кто-то (например, карта) забыл закрыться перед текстом,
    // или если мы переключаемся с мира на интерфейс
    if (m_isBatching) {
        // Проверяем, изменились ли параметры. Если да — закрываем прошлый батч
        if (m_currentTexture != pTexture || m_currentShaderID != shaderID || 
            m_currentRenderType != renderType || m_currentIsUI != isUI) 
        {
            OutputDebugStringA("[SR::Begin] AUTO-CLOSING previous batch via End() due to state change\n");
            End(); // Отправляет старую геометрию на GPU и сбрасывает m_isBatching в false
        }
    }

    // === ШАГ 2: КРИТИЧЕСКИЕ ПРОВЕРКИ УСТРОЙСТВА ===
    if (!m_pDevice) {
        char buf[256];
        sprintf(buf, "[SR::Begin] CRITICAL ERROR: m_pDevice is NULL! this=%p\n", this);
        OutputDebugStringA(buf);
        return;
    }

    // === ШАГ 3: НАСТРОЙКА СОСТОЯНИЙ DIRECT3D (RENDER STATES) ===
    OutputDebugStringA("[SR::Begin] Setting render states...\n");
    
    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    
    OutputDebugStringA("[SR::Begin] Render states set successfully\n");

    // === ШАГ 4: АКТИВАЦИЯ ШЕЙДЕРА ===
    OutputDebugStringA("[SR::Begin] Setting active shader...\n");
    if (m_pShaderManager) {
        m_pShaderManager->SetActiveShader(shaderID);
        OutputDebugStringA("[SR::Begin] Active shader set\n");
    } else {
        OutputDebugStringA("[SR::Begin] WARNING: m_pShaderManager is NULL\n");
    }

    // Логируем успешный старт нового чистого батча
    char debugMsg[256];
    sprintf(debugMsg, "[SpriteRenderer::Begin] SUCCESS: shaderID=%d, texture=%p, depth=%.2f, renderType=%d, isUI=%d\n",
            shaderID, pTexture, depth, renderType, isUI);
    OutputDebugStringA(debugMsg);
    OutputDebugStringA("[SR::Begin] FINISHED\n");

    // === ШАГ 5: ИНИЦИАЛИЗАЦИЯ ПАРАМЕТРОВ ТЕКУЩЕГО БАТЧА ===
    m_currentShaderID = shaderID;
    m_currentTexture = pTexture;
    m_currentDepth = depth;
    m_currentRenderType = renderType;
    m_currentIsUI = isUI;
    
    m_isBatching = true;  // Включаем батчинг заново для нового компонента
    m_spriteCount = 0;    // Начинаем считать спрайты нового компонента с нуля
}

void SpriteRenderer::Begin(ShaderID shaderID, LPDIRECT3DTEXTURE9 pTexture, int layer, float yPosition) {
    float compositeDepth = (float)(layer * 1000) + yPosition;
    Begin(shaderID, pTexture, compositeDepth, 0, false); // Default: world-space, Single sprite
}

void SpriteRenderer::Begin(ShaderID shaderID, LPDIRECT3DTEXTURE9 pTexture, int layer, float yPosition, int renderType, bool isUI) {
    float compositeDepth = (float)(layer * 1000) + yPosition;
    Begin(shaderID, pTexture, compositeDepth, renderType, isUI);
}

// Y-sorting helper for world objects (buildings, units): depth = layer_base + (y * scale)
void SpriteRenderer::BeginWorldObject(ShaderID shaderID, LPDIRECT3DTEXTURE9 pTexture, float worldY, float layerBase, float yScale, int renderType) {
    float depth = layerBase + (worldY * yScale);
    Begin(shaderID, pTexture, depth, renderType, false); // World-space, not UI
}

// Legacy Begin overloads (map shader names to handles for backward compatibility)
void SpriteRenderer::Begin(const char* shaderName, LPDIRECT3DTEXTURE9 pTexture) {
    OutputDebugStringA("[SR::Begin] char* version CALLED\n");
    fflush(stdout);
    int shaderID = ShaderNameToID(shaderName);
    OutputDebugStringA("[SR::Begin] Calling ShaderID version with ID=");
    char tmp[32];
    sprintf(tmp, "%d\n", shaderID);
    OutputDebugStringA(tmp);
    fflush(stdout);
    Begin(static_cast<ShaderID>(shaderID), pTexture);
    OutputDebugStringA("[SR::Begin] Returned from ShaderID version\n");
    fflush(stdout);
}

void SpriteRenderer::Begin(const char* shaderName, LPDIRECT3DTEXTURE9 pTexture, float depth) {
    int shaderID = ShaderNameToID(shaderName);
    Begin(static_cast<ShaderID>(shaderID), pTexture, depth);
}

void SpriteRenderer::Begin(const char* shaderName, LPDIRECT3DTEXTURE9 pTexture, float depth, int renderType) {
    int shaderID = ShaderNameToID(shaderName);
    Begin(static_cast<ShaderID>(shaderID), pTexture, depth, renderType);
}

void SpriteRenderer::Begin(const char* shaderName, LPDIRECT3DTEXTURE9 pTexture, float depth, int renderType, bool isUI) {
    int shaderID = ShaderNameToID(shaderName);
    Begin(static_cast<ShaderID>(shaderID), pTexture, depth, renderType, isUI);
}

void SpriteRenderer::Draw(float x, float y, float width, float height,
                          float u0, float v0, float u1, float v1,
                          DWORD color) {
    // Check batching state
    if (!m_isBatching) {
        OutputDebugStringA("Draw called without Begin!\n");
        return;
    }

    char buf[256];
    sprintf(buf, "[SR::Draw] Adding quad. x=%.1f, y=%.1f, w=%.1f, h=%.1f, color=0x%08X\n", 
            x, y, width, height, color);
    OutputDebugStringA(buf);

    // Use CreateQuad to write vertices to staging buffer and increment m_spriteCount
    CreateQuad(x, y, width, height, u0, v0, u1, v1, color);
    
    sprintf(buf, "[SR::Draw] Quad added. spriteCount=%d\n", m_spriteCount);
    OutputDebugStringA(buf);
    
    // Check if we need to flush (buffer full)
    if (m_spriteCount >= m_maxSprites) {
        sprintf(buf, "[SR::Draw] Buffer full (%d), forcing flush\n", m_spriteCount);
        OutputDebugStringA(buf);
        Flush();
    }
}

void SpriteRenderer::DrawWithTexture(float x, float y, float width, float height,
                                     float u0, float v0, float u1, float v1,
                                     LPDIRECT3DTEXTURE9 pTexture,
                                     DWORD color) {
    char dbg[256];
    sprintf(dbg, "[SR::DrawWithTexture] ENTRY x=%.1f y=%.1f w=%.1f h=%.1f tex=%p m_isBatching=%d\n",
            x, y, width, height, pTexture, m_isBatching);
    OutputDebugStringA(dbg);

    if (!m_isBatching) {
        OutputDebugStringA("DrawWithTexture called without Begin!\n");
        return;
    }

    // Set texture if changed (enables per-sprite texture switching)
    if (pTexture != m_currentTexture) {
        sprintf(dbg, "[SR::DrawWithTexture] Texture changed: old=%p new=%p\n", m_currentTexture, pTexture);
        OutputDebugStringA(dbg);
        m_currentTexture = pTexture;
        if (m_pShaderManager) {
            m_pShaderManager->SetTexture("g_texture", pTexture);
        }
    }

    // Create quad with texture - pass texture to CreateQuad
    CreateQuadWithTexture(x, y, width, height, u0, v0, u1, v1, color, pTexture);
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
    OutputDebugStringA("[SR::End] ENTRY\n");
    
    if (!m_isBatching) {
        OutputDebugStringA("[SR::End] WARNING: m_isBatching is false!\n");
        return;
    }
    
    m_isBatching = false;
    
    OutputDebugStringA("[SR::End] pendingCommands empty check\n");
    if (!m_pendingCommands.empty() || m_spriteCount > 0) {
        OutputDebugStringA("[SR::End] Calling Flush()...\n");
        Flush();
        OutputDebugStringA("[SR::End] Flush() completed\n");
    }
    
    m_spriteCount = 0;
    OutputDebugStringA("[SR::End] EXIT\n");
}

void SpriteRenderer::ResetVertexCount() {
    m_totalVertexCount = 0;
    m_totalIndexCount = 0;
    m_spriteCount = 0;
}

void SpriteRenderer::Flush(ShaderManager* pShader) {
    char buf[256];
    sprintf(buf, "[SR::Flush] ENTRY - m_spriteCount=%d, m_totalVertexCount=%d\n", m_spriteCount, m_totalVertexCount);
    OutputDebugStringA(buf);
    
    if (m_spriteCount == 0) {
        OutputDebugStringA("[SR::Flush] EXIT: m_spriteCount == 0\n");
        return;
    }

    int numVertices = m_spriteCount * 4;
    int numIndices = m_spriteCount * 6;
    
    OutputDebugStringA("[SR::Flush] Checking vertex buffer...\n");
    if (!m_pVertexBuffer) {
        OutputDebugStringA("[SR::Flush] ERROR: m_pVertexBuffer is NULL!\n");
        return;
    }
    
    OutputDebugStringA("[SR::Flush] Locking vertex buffer...\n");
    void* pGpuVertices = nullptr;
    HRESULT hr = m_pVertexBuffer->Lock(
        m_totalVertexCount * sizeof(SpriteVertex),
        numVertices * sizeof(SpriteVertex),
        &pGpuVertices,
        D3DLOCK_NOOVERWRITE
    );
    
    if (FAILED(hr)) {
        OutputDebugStringA("[SR::Flush] ERROR: Lock failed!\n");
        return;
    }
    
    OutputDebugStringA("[SR::Flush] Copying to GPU...\n");
    
    if (!m_pStagingBuffer) {
        OutputDebugStringA("[SR::Flush] ERROR: m_pStagingBuffer is NULL!\n");
        m_pVertexBuffer->Unlock();
        return;
    }
    
    size_t bytesToWrite = static_cast<size_t>(numVertices * sizeof(SpriteVertex));
    
    // Debug: Print first vertex position
    SpriteVertex* verts = (SpriteVertex*)m_pStagingBuffer;
    char vbuf[256];
    sprintf(vbuf, "[SR::Flush] First vertex: x=%.1f, y=%.1f, z=%.1f, u=%.2f, v=%.2f\n", 
            verts[0].x, verts[0].y, verts[0].z, verts[0].u, verts[0].v);
    OutputDebugStringA(vbuf);
    
    memcpy(pGpuVertices, m_pStagingBuffer, bytesToWrite);
    m_pVertexBuffer->Unlock();
    OutputDebugStringA("[SR::Flush] Unlock done\n");

    // Пакуем команду отложенного рендеринга
    OutputDebugStringA("[SR::Flush] Packing RenderCommand...\n");
    RenderCommand cmd;
    cmd.shaderID = m_currentShaderID;
    cmd.pTexture = m_currentTexture;
    cmd.depth = m_currentDepth;
    cmd.isUI = m_currentIsUI;
    cmd.baseVertex = m_totalVertexCount; 
    cmd.vertexStart = m_totalIndexCount;
    cmd.vertexCount = numVertices;
    cmd.primitiveCount = m_spriteCount * 2;
    cmd.status = 1;
    
    OutputDebugStringA("[SR::Flush] Pushing command to ShaderManager...\n");
    if (pShader) {
        OutputDebugStringA("[SR::Flush] pShader is valid\n");
        pShader->PushXbox360Command(cmd);
        OutputDebugStringA("[SR::Flush] PushXbox360Command done\n");
    } else {
        OutputDebugStringA("[SR::Flush] WARNING: pShader is NULL!\n");
    }

    m_totalVertexCount += numVertices;
    m_totalIndexCount += numIndices;
    m_spriteCount = 0;
    OutputDebugStringA("[SR::Flush] EXIT\n");
}

void SpriteRenderer::SubmitBatch(ShaderManager* pShader) {
    // Manual batch submission - same logic as Flush but without buffer switch
    if (m_spriteCount == 0) return;
    
    char dbg[256];
    sprintf(dbg, "[SR::SubmitBatch] spriteCount=%d, totalVertexCount=%d, totalIndexCount=%d\n", 
            m_spriteCount, m_totalVertexCount, m_totalIndexCount);
    OutputDebugStringA(dbg);
    fflush(stdout);
    
    // Calculate offsets for this batch - ALWAYS start from 0 for FIRST batch of frame
    // The cumulative offsets come from m_totalVertexCount which should be reset at frame start
    DWORD vertexOffset = m_totalVertexCount * sizeof(SpriteVertex);
    int vertexCount = m_spriteCount * 4;
    int indexCount = m_spriteCount * 6; // 6 indices per sprite
    
    // FORCE RESET if counts look wrong (> 1000 suggests accumulation across frames)
    if (m_totalVertexCount > 1000 || m_totalIndexCount > 3000) {
        OutputDebugStringA("[SR::SubmitBatch] WARNING: Counts too high, forcing reset!\n");
        m_totalVertexCount = 0;
        m_totalIndexCount = 0;
        vertexOffset = 0;
    }
    
    // DEBUG: Force start from 0 if this appears to be first batch
    if (m_totalVertexCount == 0 && m_totalIndexCount == 0) {
        vertexOffset = 0;
        sprintf(dbg, "[SR::SubmitBatch] FIRST BATCH - using offset 0!\n");
        OutputDebugStringA(dbg);
    }
    fflush(stdout);
    
    // Copy staging buffer to GPU at correct offset
    LPDIRECT3DVERTEXBUFFER9 currentVB = m_pVB[m_activeBuffer];
    void* pData = NULL;
    HRESULT hr = currentVB->Lock(vertexOffset, vertexCount * sizeof(SpriteVertex), &pData, 0);
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to lock vertex buffer in SubmitBatch\n");
        return;
    }
    memcpy(pData, m_pStagingBuffer, vertexCount * sizeof(SpriteVertex));
    currentVB->Unlock();

    // Create render command with correct INDEX and VERTEX offsets
    if (pShader) {
        ShaderManager::RenderCommand cmd;
        cmd.pTexture = m_currentTexture;
        cmd.shaderID = m_currentShaderID;
        cmd.vertexStart = m_totalIndexCount; // Index buffer offset
        cmd.baseVertex = m_totalVertexCount; // Vertex buffer offset
        cmd.vertexCount = vertexCount;
        cmd.primitiveCount = (DWORD)(m_spriteCount * 2);
        cmd.batchType = m_currentRenderType;
        cmd.depth = m_currentDepth;
        cmd.layer = 0;
        cmd.isUI = m_currentIsUI;
        
        cmd.states.zEnable = D3DZB_FALSE;
        cmd.states.alphaBlendEnable = TRUE;
        cmd.states.srcBlend = D3DBLEND_SRCALPHA;
        cmd.states.destBlend = D3DBLEND_INVSRCALPHA;
        cmd.states.cullMode = D3DCULL_NONE;
        
        sprintf(dbg, "[SR::SubmitBatch] Submitting: baseVert=%d, startIdx=%d, vertexCount=%d, depth=%.2f\n", 
                cmd.baseVertex, cmd.vertexStart, cmd.vertexCount, cmd.depth);
        OutputDebugStringA(dbg);
        
        pShader->Submit(cmd);
    }
    
    // Update total counts for next batch
    m_totalVertexCount += m_spriteCount * 4;  // 4 vertices per sprite
    m_totalIndexCount += m_spriteCount * 6;  // 6 indices per sprite
    m_spriteCount = 0;

    char debugMsg[256];
    sprintf(debugMsg, "[SR::SubmitBatch] After submit: m_totalVertexCount=%d, m_totalIndexCount=%d\n",
            m_totalVertexCount, m_totalIndexCount);
    OutputDebugStringA(debugMsg);
}

void SpriteRenderer::Flush() {
    // Only return if empty
    if (m_spriteCount == 0) return;

    // RING BUFFER: Check for overflow before flushing
    // NOTE: m_totalVertexCount is incremented in SubmitBatch, not here
    if (m_totalVertexCount + m_spriteCount * 4 > MAX_BUFFER_VERTICES) {
        OutputDebugStringA("[SpriteRenderer::Flush] WARNING: Vertex buffer overflow, forcing Execute\n");
        // Force execute would go here if we had access to the Execute method
        m_totalVertexCount = 0;
    }

    // Just call the version with shader manager
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
    cmd.shaderID = sid;
    cmd.pTexture = atlas ? atlas->GetTexture() : NULL;
    cmd.worldX = x - reg->pivotX;
    cmd.worldY = y - reg->pivotY;
    cmd.screenW = (float)reg->width;
    cmd.screenH = (float)reg->height;
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
    if (cmd.isUI) {
        CreateQuad(cmd.screenX, cmd.screenY, cmd.screenW, cmd.screenH, cmd.u0, cmd.v0, cmd.u1, cmd.v1, cmd.color);
    } else {
        CreateQuad(cmd.worldX, cmd.worldY, cmd.screenW, cmd.screenH, cmd.u0, cmd.v0, cmd.u1, cmd.v1, cmd.color);
    }
}

// Public wrapper for atlas sprite (compatibility wrapper in case direct call is private)
void SpriteRenderer::DrawAtlasSpritePublic(SpriteAtlas* atlas, DWORD index, float x, float y, DWORD color) {
    DrawAtlasSprite(atlas, index, x, y, color);
}

void SpriteRenderer::FillStagingAreaPublic(int startIdx, int count, const SpriteData* data) {
    FillStagingArea(startIdx, count, data);
}


void SpriteRenderer::ExecuteBatch() {
    if (m_renderQueue.empty()) return;

    // Sort all commands (std::sort is very fast on Xenon)
    std::sort(m_renderQueue.begin(), m_renderQueue.end());

    // Process sorted list by shaderID -> atlas -> depth
    int currentShaderID = -1;
    LPDIRECT3DTEXTURE9 currentTexture = NULL;
    const char* currentShaderName = NULL;

    for (size_t i = 0; i < m_renderQueue.size(); ++i) {
        const RenderCommand& cmd = m_renderQueue[i];

        if (cmd.shaderID != currentShaderID || cmd.pTexture != currentTexture) {
            // Flush previous batch
            Flush();

            // Update state for new shader/atlas
            currentShaderID = cmd.shaderID;
            currentTexture = cmd.pTexture;
            currentShaderName = (currentShaderID >= 0 && currentShaderID < (int)m_shaderNameRegistry.size())
                                ? m_shaderNameRegistry[currentShaderID].c_str()
                                : NULL;

            if (currentShaderName && m_pShaderManager) {
                // Convert string shader name to ShaderID
                ShaderID shaderID = SHADER_SPRITE; // Default to sprite shader
                if (strcmp(currentShaderName, "sprite") == 0) {
                    shaderID = SHADER_SPRITE;
                } else if (strcmp(currentShaderName, "sprite_constant_instanced") == 0) {
                    shaderID = SHADER_SPRITE_CONSTANT_INSTANCED;
                }
                
                m_pShaderManager->SetActiveShader(shaderID);
                m_pShaderManager->SetTexture("g_texture", currentTexture);
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
    if (m_instanceQueue.empty()) {
        OutputDebugStringA("[FlushConstantInstanced] m_instanceQueue is empty!\n");
        return;
    }

    char debugMsg[256];
    sprintf(debugMsg, "[FlushConstantInstanced] Rendering %d sprites individually\n", (int)m_instanceQueue.size());
    OutputDebugStringA(debugMsg);

    // CRITICAL: Set orthographic projection matrix
    D3DXMATRIX matOrtho;
    D3DXMatrixOrthoOffCenterLH(&matOrtho, 0.0f, 1280.0f, 720.0f, 0.0f, 0.0f, 1.0f);
    m_pDevice->SetVertexShaderConstantF(0, (float*)&matOrtho, 4);

    // CRITICAL: Set texture
    m_pDevice->SetTexture(0, m_currentTexture);

    // Check for NULL buffers
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

    // Set vertex streams and declaration
    m_pDevice->SetStreamSource(0, m_pStaticQuadVB, 0, sizeof(float) * 3);
    m_pDevice->SetIndices(m_pStaticQuadIB);
    m_pDevice->SetVertexDeclaration(m_pConstantInstancedDecl);

    // Draw each sprite individually with its own constant data
    // Shader expects g_SpriteData array with instance ID selecting the index
    for (size_t i = 0; i < m_instanceQueue.size(); ++i) {
        const SpriteInstance& inst = m_instanceQueue[i];

        // Set sprite data in registers c10-c11 (instance ID = 0 for individual draws)
        float spriteData[8]; // 2 float4 = position/size + UV rect
        spriteData[0] = inst.positionSize[0]; // x
        spriteData[1] = inst.positionSize[1]; // y
        spriteData[2] = inst.positionSize[2]; // width
        spriteData[3] = inst.positionSize[3]; // height
        spriteData[4] = inst.uvRect[0]; // u0
        spriteData[5] = inst.uvRect[1]; // v0
        spriteData[6] = inst.uvRect[2]; // u1
        spriteData[7] = inst.uvRect[3]; // v1
        m_pDevice->SetVertexShaderConstantF(10, spriteData, 2);

        // Debug: Log first few sprites
        if (i < 3) {
            sprintf(debugMsg, "[FlushConst] Sprite[%d]: pos=(%.1f,%.1f,%.1f,%.1f), uv=(%.3f,%.3f,%.3f,%.3f)\n",
                    (int)i, spriteData[0], spriteData[1], spriteData[2], spriteData[3],
                    spriteData[4], spriteData[5], spriteData[6], spriteData[7]);
            OutputDebugStringA(debugMsg);
        }

        // Draw the quad (6 vertices = 2 triangles)
        m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2);
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
    if (m_pShaderManager && m_currentShaderID != SHADER_INVALID) {
        char debugMsg[256];
        sprintf(debugMsg, "[SR] FlushStandard: Using shaderID=%d\n", m_currentShaderID);
        OutputDebugStringA(debugMsg);

        // Prepare shader with global uniforms
        D3DXMATRIX matOrtho;
        D3DXMatrixOrthoOffCenterLH(&matOrtho, 0.0f, 1280.0f, 720.0f, 0.0f, 0.0f, 1.0f);
        m_pShaderManager->Prepare(m_currentShaderID, &matOrtho);
        m_pShaderManager->BeginShader();

        // Set texture via UpdateConstants (unified method)
        m_pShaderManager->UpdateConstants(m_currentTexture, NULL);

        sprintf(debugMsg, "[SR] FlushStandard: Setting texture 0x%p\n", m_currentTexture);
        OutputDebugStringA(debugMsg);

        m_pShaderManager->Commit();

        m_pDevice->SetTexture(0, m_currentTexture);

        // Draw for each pass
        UINT numPasses = m_pShaderManager->GetNumPasses();
        for (UINT pass = 0; pass < numPasses; pass++) {
            m_pShaderManager->BeginPass(pass);

            // Xbox 360: CommitChanges() CRITICAL before Draw
            m_pShaderManager->CommitChanges();

            // 6. Draw
            m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, m_spriteCount * 4, 0, m_spriteCount * 2);

            m_pShaderManager->EndPass();
        }

        m_pShaderManager->EndShader();
    } else {
        OutputDebugStringA("[SR] FlushStandard: No shader manager or empty shader name\n");
        m_pDevice->SetTexture(0, m_currentTexture);
        m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, m_spriteCount * 4, 0, m_spriteCount * 2);
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
    CreateQuadWithTexture(x, y, width, height, u0, v0, u1, v1, color, m_currentTexture);
}

void SpriteRenderer::CreateQuadWithTexture(float x, float y, float width, float height,
                                       float u0, float v0, float u1, float v1,
                                       DWORD color, LPDIRECT3DTEXTURE9 pTexture) {
    OutputDebugStringA("[SR::CreateQuad] ENTRY\n");
    
    char texBuf[256];
    sprintf(texBuf, "[SR::CreateQuad] pTexture=%p, m_currentTexture=%p\n", pTexture, m_currentTexture);
    OutputDebugStringA(texBuf);
    
    // Update current texture if different
    if (pTexture != m_currentTexture) {
        m_currentTexture = pTexture;
        if (m_pShaderManager) {
            sprintf(texBuf, "[SR::CreateQuad] Calling SM::SetTexture with texture=%p\n", pTexture);
            OutputDebugStringA(texBuf);
            m_pShaderManager->SetTexture("g_texture", pTexture);
            OutputDebugStringA("[SR::CreateQuad] SM::SetTexture returned\n");
        }
    }

    // XBOX 360 CRITICAL: Check for sprite limit BEFORE any calculations
    if (m_spriteCount >= m_maxSprites) {
        char errorMsg[256];
        sprintf(errorMsg, "[SR::CreateQuad] CRITICAL: Sprite limit exceeded! count=%d, max=%d\n", m_spriteCount, m_maxSprites);
        OutputDebugStringA(errorMsg);
        return;
    }

    // Create render command with vertices (same as Draw() path)
    RenderCommand cmd;
    cmd.pTexture = m_currentTexture;
    cmd.shaderID = m_currentShaderID;
    cmd.depth = m_currentDepth;
    cmd.batchType = m_currentRenderType;
    cmd.isUI = m_currentIsUI;
    cmd.color = color;

    // Store position and UV data
    cmd.screenX = x;
    cmd.screenY = y;
    cmd.screenW = width;
    cmd.screenH = height;
    cmd.u0 = u0; cmd.v0 = v0;
    cmd.u1 = u1; cmd.v1 = v1;

    // Create vertices directly in command
    cmd.vertices[0].x = x; cmd.vertices[0].y = y; cmd.vertices[0].z = 0.0f;
    cmd.vertices[0].u = u0; cmd.vertices[0].v = v0;
    cmd.vertices[0].color = color;
    cmd.vertices[0].padding[0] = 0.0f; cmd.vertices[0].padding[1] = 0.0f;

    cmd.vertices[1].x = x + width; cmd.vertices[1].y = y; cmd.vertices[1].z = 0.0f;
    cmd.vertices[1].u = u1; cmd.vertices[1].v = v0;
    cmd.vertices[1].color = color;
    cmd.vertices[1].padding[0] = 0.0f; cmd.vertices[1].padding[1] = 0.0f;

    cmd.vertices[2].x = x; cmd.vertices[2].y = y + height; cmd.vertices[2].z = 0.0f;
    cmd.vertices[2].u = u0; cmd.vertices[2].v = v1;
    cmd.vertices[2].color = color;
    cmd.vertices[2].padding[0] = 0.0f; cmd.vertices[2].padding[1] = 0.0f;

    cmd.vertices[3].x = x + width; cmd.vertices[3].y = y + height; cmd.vertices[3].z = 0.0f;
    cmd.vertices[3].u = u1; cmd.vertices[3].v = v1;
    cmd.vertices[3].color = color;
    cmd.vertices[3].padding[0] = 0.0f; cmd.vertices[3].padding[1] = 0.0f;

    // Set render states
    cmd.states.zEnable = D3DZB_FALSE;
    cmd.states.alphaBlendEnable = TRUE;
    cmd.states.srcBlend = D3DBLEND_SRCALPHA;
    cmd.states.destBlend = D3DBLEND_INVSRCALPHA;
    cmd.states.cullMode = D3DCULL_NONE;

    // Set counts
    cmd.vertexCount = 4;
    cmd.primitiveCount = 2;
    cmd.vertexStart = 0;

    // Add to pending commands queue (same as Draw() path)
    m_pendingCommands.push_back(cmd);

    m_spriteCount++;

    OutputDebugStringA("[SR::CreateQuad] FINISHED\n");
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
