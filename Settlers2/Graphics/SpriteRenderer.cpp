#include "stdafx.h"
#include "SpriteRenderer.h"
#include "ShaderManager.h"

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
    D3DXMatrixIdentity((D3DXMATRIX*)m_projMatrix);
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

    for (std::map<WORD, LPDIRECT3DTEXTURE9>::iterator it = m_textureMap.begin(); it != m_textureMap.end(); ++it) {
        if (it->second) it->second->Release();
    }
    m_textureMap.clear();

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

int SpriteRenderer::Execute(const BatchBuilder& builder) {
    ::OutputDebugStringA("A\n");

    uint32_t vertexCount = builder.GetVertexCount();
    uint32_t indexCount = builder.GetIndexCount();

    char buf[256];
    sprintf(buf, "[SR] SpriteVertex sizeof=%d pos=%d color=%d uv=%d stride=%d\n",
        (int)sizeof(SpriteVertex),
        (int)offsetof(SpriteVertex, x),
        (int)offsetof(SpriteVertex, color),
        (int)offsetof(SpriteVertex, u),
        (int)sizeof(SpriteVertex));
    ::OutputDebugStringA(buf);

    ::OutputDebugStringA("B\n");
    void* pData = NULL;
    HRESULT hr = m_vertexBuffer->Lock(0, vertexCount * sizeof(SpriteVertex), &pData, 0);
    sprintf(buf, "[SR] VB Lock hr=%08X pData=%p\n", hr, pData);
    ::OutputDebugStringA(buf);
    if (SUCCEEDED(hr) && pData) {
        memcpy(pData, builder.GetVertices(), vertexCount * sizeof(SpriteVertex));
        m_vertexBuffer->Unlock();
        ::OutputDebugStringA("[SR] VB unlocked\n");
    }

    ::OutputDebugStringA("C\n");
    hr = m_indexBuffer->Lock(0, indexCount * sizeof(uint32_t), &pData, 0);
    sprintf(buf, "[SR] IB Lock hr=%08X pData=%p\n", hr, pData);
    ::OutputDebugStringA(buf);
    if (SUCCEEDED(hr) && pData) {
        memcpy(pData, builder.GetIndices(), indexCount * sizeof(uint32_t));
        m_indexBuffer->Unlock();
        ::OutputDebugStringA("[SR] IB unlocked\n");
    }

    ::OutputDebugStringA("D\n");
    hr = m_pDevice->SetVertexDeclaration(m_vertexDecl);
    sprintf(buf, "[SR] SetVertexDecl hr=%08X\n", hr);
    ::OutputDebugStringA(buf);

    ::OutputDebugStringA("E\n");
    hr = m_pDevice->SetStreamSource(0, m_vertexBuffer, 0, sizeof(SpriteVertex));
    sprintf(buf, "[SR] SetStreamSource hr=%08X stride=%d\n", hr, (int)sizeof(SpriteVertex));
    ::OutputDebugStringA(buf);

    ::OutputDebugStringA("F\n");
    hr = m_pDevice->SetIndices(m_indexBuffer);
    sprintf(buf, "[SR] SetIndices hr=%08X\n", hr);
    ::OutputDebugStringA(buf);

    ::OutputDebugStringA("G\n");
    for (uint32_t i = 0; i < builder.GetBatchCount(); i++) {
        const RenderBatch& batch = builder.GetBatches()[i];

        if (m_stateCache.BlendChanged(batch.blendMode)) {
            ::OutputDebugStringA("[SR] Blending\n");
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
            ::OutputDebugStringA("[SR] Shader\n");
            SetShader(batch.shaderID);
            m_stateCache.currentShader = batch.shaderID;
            m_shaderSwitches++;
        }

        if (m_stateCache.TextureChanged(batch.textureID)) {
            ::OutputDebugStringA("[SR] Texture\n");
            SetTexture(batch.textureID);
            m_pShaderManager->CommitChanges();
            m_stateCache.currentTexture = batch.textureID;
            m_textureSwitches++;
        }

        uint32_t primitiveCount = batch.indexCount / 3;

        sprintf(buf, "[SR] DIP batch=%d startIdx=%d idx=%d prim=%d tex=%d shader=%d\n",
                i, batch.startIndex, batch.indexCount, primitiveCount, batch.textureID, batch.shaderID);
        ::OutputDebugStringA(buf);

        ::OutputDebugStringA("H\n");
        hr = m_pDevice->DrawIndexedPrimitive(
            D3DPT_TRIANGLELIST,
            0,
            0,
            vertexCount,
            batch.startIndex,
            primitiveCount);
        sprintf(buf, "[SR] DIP hr=%08X\n", hr);
        ::OutputDebugStringA(buf);

        m_drawCalls++;
    }

    ::OutputDebugStringA("I\n");
    m_pDevice->SetTexture(0, NULL);

    ::OutputDebugStringA("J\n");
    return 0;
}

void SpriteRenderer::SetTextureSlot(WORD id, LPDIRECT3DTEXTURE9 tex) {
    std::map<WORD, LPDIRECT3DTEXTURE9>::iterator it = m_textureMap.find(id);
    if (it != m_textureMap.end()) {
        if (it->second) it->second->Release();
    }
    m_textureMap[id] = tex;
    if (tex) tex->AddRef();
}

void SpriteRenderer::SetTexture(WORD textureID) {
    if (!m_pDevice) return;

    std::map<WORD, LPDIRECT3DTEXTURE9>::iterator it = m_textureMap.find(textureID);
    if (it != m_textureMap.end() && it->second) {
        m_pDevice->SetTexture(0, it->second);
        if (m_pShaderManager) {
            m_pShaderManager->SetLocalUniforms(it->second, 0.0f);
        }
    }
}

void SpriteRenderer::SetShader(WORD shaderID) {
    if (!m_pShaderManager) return;
    if (m_pShaderManager->GetCurrentShaderID() != SHADER_INVALID) {
        m_pShaderManager->EndPass();
        m_pShaderManager->EndShader();
    }
    if (!m_pShaderManager->SetActiveShader((ShaderID)shaderID)) return;
    m_pShaderManager->SetMatrix("WVP", m_projMatrix);
    m_pShaderManager->BeginShader();
    m_pShaderManager->BeginPass(0);
    m_pShaderManager->Commit();
}

}
