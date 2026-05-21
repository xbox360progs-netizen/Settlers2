#include "stdafx.h"
#include "SpriteRenderer.h"
#include "ShaderManager.h"

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

namespace Graphics {

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
        D3DUSAGE_WRITEONLY,
        0,
        D3DPOOL_DEFAULT,
        &m_vertexBuffer,
        NULL);
    if (FAILED(hr)) return hr;

    hr = device->CreateIndexBuffer(
        indexBufferSize,
        D3DUSAGE_WRITEONLY,
        D3DFMT_INDEX32,
        D3DPOOL_DEFAULT,
        &m_indexBuffer,
        NULL);
    if (FAILED(hr)) return hr;

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

void SpriteRenderer::Execute(const BatchBuilder& builder) {
    if (!m_pDevice || builder.GetBatchCount() == 0) return;

    uint32_t vertexCount = builder.GetVertexCount();
    uint32_t indexCount = builder.GetIndexCount();

    void* pData = NULL;
    HRESULT hr = m_vertexBuffer->Lock(0, vertexCount * sizeof(SpriteVertex), &pData, 0);
    if (SUCCEEDED(hr) && pData) {
        memcpy(pData, builder.GetVertices(), vertexCount * sizeof(SpriteVertex));
        m_vertexBuffer->Unlock();
    }

    hr = m_indexBuffer->Lock(0, indexCount * sizeof(uint32_t), &pData, 0);
    if (SUCCEEDED(hr) && pData) {
        memcpy(pData, builder.GetIndices(), indexCount * sizeof(uint32_t));
        m_indexBuffer->Unlock();
    }

    m_pDevice->SetVertexDeclaration(m_vertexDecl);
    m_pDevice->SetStreamSource(0, m_vertexBuffer, 0, sizeof(SpriteVertex));
    m_pDevice->SetIndices(m_indexBuffer);

    for (uint32_t i = 0; i < builder.GetBatchCount(); i++) {
        const RenderBatch& batch = builder.GetBatches()[i];

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

        uint32_t primitiveCount = batch.indexCount / 3;

        m_pDevice->DrawIndexedPrimitive(
            D3DPT_TRIANGLELIST,
            0,
            0,
            vertexCount,
            batch.startIndex,
            primitiveCount);

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

}
