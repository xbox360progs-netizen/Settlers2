#include "stdafx.h"
#include "SpriteRenderer.h"
#include "ShaderManager.h"

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

SpriteRenderer::SpriteRenderer()
    : m_pDevice(NULL)
    , m_pShaderManager(NULL)
    , m_vertexBuffer(NULL)
    , m_indexBuffer(NULL)
    , m_vertexDecl(NULL)
    , m_maxSprites(4096)
    , m_drawCalls(0)
    , m_textureSwitches(0)
    , m_shaderSwitches(0)
    , m_stateChanges(0)
{
}

SpriteRenderer::~SpriteRenderer() {
    Shutdown();
}

HRESULT SpriteRenderer::Initialize(LPDIRECT3DDEVICE9 device, ShaderManager* shaderManager, int maxSprites) {
    m_pDevice = device;
    m_pShaderManager = shaderManager;
    m_maxSprites = maxSprites;

    int vertexBufferSize = m_maxSprites * 4 * sizeof(SpriteVertex);
    int indexBufferSize = m_maxSprites * 6 * sizeof(WORD);

    HRESULT hr = device->CreateVertexBuffer(
        vertexBufferSize,
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        0,
        D3DPOOL_DEFAULT,
        &m_vertexBuffer,
        NULL);
    if (FAILED(hr)) return hr;

    hr = device->CreateIndexBuffer(
        indexBufferSize,
        D3DUSAGE_WRITEONLY,
        D3DFMT_INDEX16,
        D3DPOOL_DEFAULT,
        &m_indexBuffer,
        NULL);
    if (FAILED(hr)) return hr;

    WORD* pIndices = NULL;
    hr = m_indexBuffer->Lock(0, 0, (void**)&pIndices, 0);
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
        m_indexBuffer->Unlock();
    }

    D3DVERTEXELEMENT9 decl[] = {
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        { 0, 20, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
        { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
        D3DDECL_END()
    };
    hr = device->CreateVertexDeclaration(decl, &m_vertexDecl);
    if (FAILED(hr)) return hr;

    OutputDebugStringA("[SpriteRenderer] Initialized\n");
    return S_OK;
}

void SpriteRenderer::Shutdown() {
    if (m_vertexDecl) { m_vertexDecl->Release(); m_vertexDecl = NULL; }
    if (m_indexBuffer) { m_indexBuffer->Release(); m_indexBuffer = NULL; }
    if (m_vertexBuffer) { m_vertexBuffer->Release(); m_vertexBuffer = NULL; }

    m_pDevice = NULL;
    m_pShaderManager = NULL;
}

void SpriteRenderer::OnLostDevice() {
    if (m_vertexDecl) { m_vertexDecl->Release(); m_vertexDecl = NULL; }
    if (m_indexBuffer) { m_indexBuffer->Release(); m_indexBuffer = NULL; }
}

void SpriteRenderer::OnResetDevice() {
    Initialize(m_pDevice, m_pShaderManager, m_maxSprites);
}

void SpriteRenderer::BeginFrame() {
    m_drawCalls = 0;
    m_textureSwitches = 0;
    m_shaderSwitches = 0;
    m_stateChanges = 0;
    m_stateCache = RenderStateCache();
}

void SpriteRenderer::EndFrame() {
}

void SpriteRenderer::Execute(const RenderBatch* batches, int batchCount) {
    if (!m_pDevice || batchCount == 0 || !batches) return;

    m_pDevice->SetVertexDeclaration(m_vertexDecl);
    m_pDevice->SetStreamSource(0, m_vertexBuffer, 0, sizeof(SpriteVertex));
    m_pDevice->SetIndices(m_indexBuffer);

    for (int i = 0; i < batchCount; i++) {
        const RenderBatch& batch = batches[i];

        if (m_stateCache.BlendChanged(batch.blendMode)) {
            if (batch.blendMode == 0) {
                m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            } else {
                m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
                m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
                m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            }
            m_stateCache.currentBlend = batch.blendMode;
            m_stateChanges++;
        }

        if (m_stateCache.ShaderChanged(batch.shaderID)) {
            SetShader(batch.shaderID);
            m_stateCache.currentShader = batch.shaderID;
            m_shaderSwitches++;
        }

        if (m_stateCache.TextureChanged(batch.textureID)) {
            SetTexture(batch.textureID);
            m_stateCache.currentTexture = batch.textureID;
            m_textureSwitches++;
        }

        DWORD spriteCount = batch.vertexCount / 4;
        DWORD indexStart = (batch.vertexOffset / 4) * 6;

        m_pDevice->DrawIndexedPrimitive(
            D3DPT_TRIANGLELIST,
            0,
            batch.vertexOffset,
            batch.vertexCount,
            indexStart,
            spriteCount * 2);

        m_drawCalls++;
    }

    m_pDevice->SetTexture(0, NULL);
}

void SpriteRenderer::SetTexture(WORD textureID) {
    if (!m_pDevice) return;
    (void)textureID;
}

void SpriteRenderer::SetShader(WORD shaderID) {
    if (!m_pShaderManager) return;
    m_pShaderManager->SetActiveShader((ShaderID)shaderID);
    m_pShaderManager->BeginShader();
    m_pShaderManager->BeginPass(0);
    m_pShaderManager->Commit();
}
