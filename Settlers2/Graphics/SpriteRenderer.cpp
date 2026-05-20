#include "stdafx.h"
#include "SpriteRenderer_slim.h"
#include "ShaderManager.h"

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

SpriteRenderer::SpriteRenderer()
    : m_pDevice(NULL)
    , m_pShaderManager(NULL)
    , m_pIndexBuffer(NULL)
    , m_pVertexDecl(NULL)
    , m_pStagingBuffer(NULL)
    , m_activeBuffer(0)
    , m_pVertexBuffer(NULL)
    , m_pGpuBufferA(NULL)
    , m_pGpuBufferB(NULL)
    , m_pVB{NULL, NULL}
    , m_totalVertexCount(0)
    , m_totalIndexCount(0)
    , m_maxSprites(4096)
    , m_spriteCount(0)
#ifdef _XBOX
    , m_pAsyncCommandBuffer(NULL)
    , m_pAsyncCall(NULL)
    , m_pGpuFence(NULL)
    , m_isFirstFlush(true)
#endif
{
}

SpriteRenderer::~SpriteRenderer() {
    Shutdown();
}

HRESULT SpriteRenderer::Initialize(LPDIRECT3DDEVICE9 device, ShaderManager* shaderManager, int maxSprites) {
    m_pDevice = device;
    m_pShaderManager = shaderManager;
    m_maxSprites = 4096;

    int vertexBufferSize = m_maxSprites * 4 * sizeof(SpriteVertex);
    int indexBufferSize = m_maxSprites * 6 * sizeof(WORD);

    for (int i = 0; i < 2; i++) {
        HRESULT hr = m_pDevice->CreateVertexBuffer(
            vertexBufferSize, 0, 0, D3DPOOL_DEFAULT, &m_pVB[i], NULL);
        if (FAILED(hr)) return hr;
    }
    m_activeBuffer = 0;
    m_pGpuBufferA = m_pVB[0];
    m_pGpuBufferB = m_pVB[1];

    int ringBufferSize = m_maxSprites * 4 * 3 * sizeof(SpriteVertex);
    HRESULT hr = m_pDevice->CreateVertexBuffer(
        ringBufferSize, 0, 0, D3DPOOL_DEFAULT, &m_pVertexBuffer, NULL);
    if (FAILED(hr)) return hr;

    hr = m_pDevice->CreateIndexBuffer(
        indexBufferSize, 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_pIndexBuffer, NULL);
    if (FAILED(hr)) return hr;

    WORD* pIndices = NULL;
    hr = m_pIndexBuffer->Lock(0, 0, (void**)&pIndices, 0);
    if (SUCCEEDED(hr)) {
        for (int i = 0; i < m_maxSprites; i++) {
            int v = (i % 1024) * 4;
            pIndices[i * 6 + 0] = v + 0;
            pIndices[i * 6 + 1] = v + 1;
            pIndices[i * 6 + 2] = v + 2;
            pIndices[i * 6 + 3] = v + 2;
            pIndices[i * 6 + 4] = v + 1;
            pIndices[i * 6 + 5] = v + 3;
        }
        m_pIndexBuffer->Unlock();
    }

    D3DVERTEXELEMENT9 decl[] = {
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        { 0, 20, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
        { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
        D3DDECL_END()
    };
    hr = m_pDevice->CreateVertexDeclaration(decl, &m_pVertexDecl);
    if (FAILED(hr)) return hr;

    m_pStagingBuffer = (__declspec(align(32)) SpriteVertex*)_aligned_malloc(
        m_maxSprites * 4 * sizeof(SpriteVertex), 32);

    OutputDebugStringA("[SpriteRenderer] Slim initialized\n");
    return S_OK;
}

void SpriteRenderer::Shutdown() {
    if (m_pStagingBuffer) {
        _aligned_free(m_pStagingBuffer);
        m_pStagingBuffer = NULL;
    }

    for (int i = 0; i < 2; i++) {
        if (m_pVB[i]) { m_pVB[i]->Release(); m_pVB[i] = NULL; }
    }

    if (m_pVertexDecl) { m_pVertexDecl->Release(); m_pVertexDecl = NULL; }
    if (m_pIndexBuffer) { m_pIndexBuffer->Release(); m_pIndexBuffer = NULL; }

#ifdef _XBOX
    if (m_pAsyncCommandBuffer) { m_pAsyncCommandBuffer->Release(); m_pAsyncCommandBuffer = NULL; }
    if (m_pGpuFence) { m_pGpuFence->Release(); m_pGpuFence = NULL; }
#endif

    m_commands.clear();
    m_pDevice = NULL;
    m_pShaderManager = NULL;
}

void SpriteRenderer::OnLostDevice() {
    for (int i = 0; i < 2; i++) {
        if (m_pVB[i]) { m_pVB[i]->Release(); m_pVB[i] = NULL; }
    }
    if (m_pIndexBuffer) { m_pIndexBuffer->Release(); m_pIndexBuffer = NULL; }
    if (m_pVertexDecl) { m_pVertexDecl->Release(); m_pVertexDecl = NULL; }
}

void SpriteRenderer::OnResetDevice() {
    Initialize(m_pDevice, m_pShaderManager, m_maxSprites);
}

void SpriteRenderer::BeginFrame() {
    m_totalVertexCount = 0;
    m_totalIndexCount = 0;
    m_spriteCount = 0;
    m_commands.clear();
}

void SpriteRenderer::EndFrame() {
}

void SpriteRenderer::ResetBatchState() {
    m_spriteCount = 0;
}

void SpriteRenderer::SubmitSprite(const RenderCommand& cmd) {
    if (m_spriteCount >= m_maxSprites) return;
    m_commands.push_back(cmd);
    CreateQuad(cmd.worldX, cmd.worldY, cmd.width, cmd.height,
               cmd.u0, cmd.v0, cmd.u1, cmd.v1, cmd.color);
    m_spriteCount++;
}

void SpriteRenderer::CreateQuad(float x, float y, float width, float height,
                                 float u0, float v0, float u1, float v1,
                                 DWORD color) {
    if (!m_pStagingBuffer || m_spriteCount >= m_maxSprites) return;

    SpriteVertex* verts = &m_pStagingBuffer[m_spriteCount * 4];

    verts[0].x = x;          verts[0].y = y;           verts[0].z = 0.0f;
    verts[0].u = u0;         verts[0].v = v0;
    verts[0].color = color;

    verts[1].x = x + width;  verts[1].y = y;           verts[1].z = 0.0f;
    verts[1].u = u1;         verts[1].v = v0;
    verts[1].color = color;

    verts[2].x = x;          verts[2].y = y + height; verts[2].z = 0.0f;
    verts[2].u = u0;         verts[2].v = v1;
    verts[2].color = color;

    verts[3].x = x + width;  verts[3].y = y + height; verts[3].z = 0.0f;
    verts[3].u = u1;         verts[3].v = v1;
    verts[3].color = color;
}

void SpriteRenderer::Flush() {
    if (m_spriteCount == 0) return;
    if (m_pShaderManager) {
        Flush(m_pShaderManager);
    }
}

void SpriteRenderer::Flush(ShaderManager* pShader) {
    if (m_spriteCount == 0 || !m_pDevice) return;

    int numVertices = m_spriteCount * 4;

    if (m_totalVertexCount > 48000 || numVertices > 16000) {
        m_totalVertexCount = 0;
    }

    void* pGpuVertices = NULL;
    HRESULT hr = m_pVertexBuffer->Lock(
        m_totalVertexCount * sizeof(SpriteVertex),
        numVertices * sizeof(SpriteVertex),
        &pGpuVertices, D3DLOCK_NOOVERWRITE);

    if (FAILED(hr)) return;

    void* pSrc = (void*)((SpriteVertex*)m_pStagingBuffer);
    memcpy(pGpuVertices, pSrc, numVertices * sizeof(SpriteVertex));
    m_pVertexBuffer->Unlock();

    RenderCommand cmd;
    cmd.baseVertex = m_totalVertexCount;
    cmd.vertexStart = m_totalIndexCount;
    cmd.vertexCount = numVertices;
    cmd.primitiveCount = m_spriteCount * 2;
    cmd.status = 1;

    if (pShader) {
        pShader->PushXbox360Command(cmd);
    }

    m_totalVertexCount += numVertices;
    m_totalIndexCount += m_spriteCount * 6;
    m_spriteCount = 0;
}

void SpriteRenderer::InternalDraw(const RenderCommand& cmd) {
}

void SpriteRenderer::PushCommand(const RenderCommand& cmd) {
    m_commands.push_back(cmd);
}

#ifdef _XBOX
void SpriteRenderer::SetAsyncCommandBuffer(IDirect3DCommandBuffer9* pBuffer, IDirect3DAsyncCommandBufferCall9* pAsyncCall) {
    m_pAsyncCommandBuffer = pBuffer;
    m_pAsyncCall = pAsyncCall;
}

void SpriteRenderer::FlushBatchesAsync() {
    if (m_spriteCount == 0) return;

    if (m_pGpuFence && !m_isFirstFlush) {
        HRESULT hr = S_OK;
        while ((hr = m_pGpuFence->GetData(NULL, 0, D3DGETDATA_FLUSH)) == S_FALSE) {
            YieldProcessor();
            Sleep(0);
        }
    }
    m_isFirstFlush = false;

    m_pDevice->BeginCommandBuffer(m_pAsyncCommandBuffer, 0, NULL, NULL, NULL, 0);

    if (m_pShaderManager) {
        m_pShaderManager->BeginShader();
        m_pShaderManager->BeginPass(0);
        m_pShaderManager->Commit();
    }

    m_pDevice->SetStreamSource(0, m_pVertexBuffer, 0, sizeof(SpriteVertex));
    m_pDevice->SetIndices(m_pIndexBuffer);
    m_pDevice->SetVertexDeclaration(m_pVertexDecl);

    m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, m_totalVertexCount, 0,
                                    m_spriteCount * 4, 0, m_spriteCount * 2);

    if (m_pShaderManager) {
        m_pShaderManager->EndPass();
        m_pShaderManager->EndShader();
    }

    m_pDevice->EndCommandBuffer();

    if (m_pGpuFence) {
        m_pGpuFence->Issue(D3DISSUE_END);
    }

    HRESULT hr = m_pAsyncCall->FixupAndSignal(m_pAsyncCommandBuffer, 0, 0);

    m_totalVertexCount += m_spriteCount * 4;
    m_totalIndexCount += m_spriteCount * 6;
    m_spriteCount = 0;
}
#endif