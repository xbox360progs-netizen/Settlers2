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
        D3DFMT_INDEX16,
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

    // Сбрасываем шейдер в INVALID — чтобы следующий Execute()
    // гарантированно вызвал BeginShader() / BeginPass() заново
    if (m_pShaderManager) {
        m_pShaderManager->SetActiveShader(SHADER_INVALID);
    }
}

void SpriteRenderer::EndFrame() {
}

int SpriteRenderer::Execute(const BatchBuilder& builder) {
    char dbg[512];
    OutputDebugStringA("[SpriteRenderer] Execute ENTRY\n");

    uint32_t vertexCount = builder.GetVertexCount();
    uint32_t indexCount = builder.GetIndexCount();
    uint32_t batchCount = builder.GetBatchCount();
    sprintf(dbg, "[SpriteRenderer] Execute: vtx=%u idx=%u batch=%u\n", vertexCount, indexCount, batchCount);
    OutputDebugStringA(dbg);

    void* pData = NULL;
    HRESULT hr = m_vertexBuffer->Lock(0, vertexCount * sizeof(SpriteVertex), &pData, 0);
    if (SUCCEEDED(hr) && pData) {
        memcpy(pData, builder.GetVertices(), vertexCount * sizeof(SpriteVertex));
        m_vertexBuffer->Unlock();
        OutputDebugStringA("[SpriteRenderer] VB locked+copy OK\n");
    } else {
        sprintf(dbg, "[SpriteRenderer] VB LOCK FAILED: hr=0x%08x pData=%p\n", hr, pData);
        OutputDebugStringA(dbg);
    }

    hr = m_indexBuffer->Lock(0, indexCount * sizeof(uint16_t), &pData, 0);
    if (SUCCEEDED(hr) && pData) {
        memcpy(pData, builder.GetIndices(), indexCount * sizeof(uint16_t));
        m_indexBuffer->Unlock();
        OutputDebugStringA("[SpriteRenderer] IB locked+copy OK\n");
    } else {
        sprintf(dbg, "[SpriteRenderer] IB LOCK FAILED: hr=0x%08x pData=%p\n", hr, pData);
        OutputDebugStringA(dbg);
    }

    hr = m_pDevice->SetVertexDeclaration(m_vertexDecl);
    hr = m_pDevice->SetStreamSource(0, m_vertexBuffer, 0, sizeof(SpriteVertex));
    hr = m_pDevice->SetIndices(m_indexBuffer);

    // Force Z-test OFF at start of each frame
    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    
    // Отключаем блэндинг по умолчанию
    m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

    sprintf(dbg, "[SpriteRenderer] Execute: shaderManager=%p, currentShaderID=%d\n",
            m_pShaderManager, m_pShaderManager ? m_pShaderManager->GetCurrentShaderID() : -99);
    OutputDebugStringA(dbg);

    for (uint32_t i = 0; i < batchCount; i++) {
        const RenderBatch& batch = builder.GetBatches()[i];

        sprintf(dbg, "[SpriteRenderer] Batch %d: shader=%d tex=%d blend=%d startIdx=%d idxCount=%d\n",
                i, batch.shaderID, batch.textureID, batch.blendMode, batch.startIndex, batch.indexCount);
        OutputDebugStringA(dbg);

        // 1. Устанавливаем блэндинг
        if (batch.blendMode == 0) {
            m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        } else {
            m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        }

        // 2. Управление шейдерами - ПРОСТОЕ И НАДЕЖНОЕ
        ShaderID currentShader = m_pShaderManager ? m_pShaderManager->GetCurrentShaderID() : SHADER_INVALID;
        
        sprintf(dbg, "[SpriteRenderer]   currentShader=%d, batchShader=%d\n", (int)currentShader, (int)batch.shaderID);
        OutputDebugStringA(dbg);

        if (m_pShaderManager && currentShader != (ShaderID)batch.shaderID) {
            // Завершаем текущий шейдер если он активен
            if (currentShader != SHADER_INVALID) {
                OutputDebugStringA("[SpriteRenderer]   ending previous shader\n");
                m_pShaderManager->EndPass();
                m_pShaderManager->EndShader();
            }
            
            // Активируем новый шейдер ТОЛЬКО ЕСЛИ ОН ВАЛИДЕН
            if ((ShaderID)batch.shaderID != SHADER_INVALID && m_pShaderManager->HasShader((ShaderID)batch.shaderID)) {
                OutputDebugStringA("[SpriteRenderer]   calling SetShader()\n");
                SetShader(batch.shaderID);
            } else {
                // Если шейдер невалиден, сбрасываем активный шейдер
                sprintf(dbg, "[SpriteRenderer]   shader INVALID or not loaded: shaderID=%d hasShader=%d\n",
                        batch.shaderID, m_pShaderManager ? m_pShaderManager->HasShader((ShaderID)batch.shaderID) : 0);
                OutputDebugStringA(dbg);
                if (m_pShaderManager) {
                    m_pShaderManager->SetActiveShader(SHADER_INVALID);
                }
                continue; // Пропускаем этот batch
            }
        }

        // 3. Устанавливаем текстуру
        SetTexture(batch.textureID);

        // 4. Выполняем отрисовку
        bool hasActiveShader = m_pShaderManager && m_pShaderManager->GetActiveShader();
        sprintf(dbg, "[SpriteRenderer]   hasActiveShader=%d\n", hasActiveShader);
        OutputDebugStringA(dbg);

        if (hasActiveShader) {
            OutputDebugStringA("[SpriteRenderer]   >>> BeginPass(0)\n");
            m_pShaderManager->BeginPass(0);
            
            uint32_t primitiveCount = batch.indexCount / 3;
            sprintf(dbg, "[SpriteRenderer]   >>> DrawIndexedPrimitive prims=%d\n", primitiveCount);
            OutputDebugStringA(dbg);
            
            hr = m_pDevice->DrawIndexedPrimitive(
                D3DPT_TRIANGLELIST,
                0,
                0,
                vertexCount,
                batch.startIndex,
                primitiveCount);
                
            sprintf(dbg, "[SpriteRenderer]   <<< DrawIndexedPrimitive hr=0x%08x\n", hr);
            OutputDebugStringA(dbg);
            
            OutputDebugStringA("[SpriteRenderer]   <<< EndPass()\n");
            m_pShaderManager->EndPass();
            m_drawCalls++;
        } else {
            OutputDebugStringA("[SpriteRenderer]   SKIP draw: no active shader\n");
        }
    }

    // ФИНАЛЬНАЯ ОЧИСТКА - только если есть активный шейдер
    ShaderID finalShader = m_pShaderManager ? m_pShaderManager->GetCurrentShaderID() : SHADER_INVALID;
    sprintf(dbg, "[SpriteRenderer] final cleanup: currentShaderID=%d\n", (int)finalShader);
    OutputDebugStringA(dbg);
    
    if (m_pShaderManager && finalShader != SHADER_INVALID) {
        OutputDebugStringA("[SpriteRenderer] final EndShader()\n");
        m_pShaderManager->EndShader();
    }

    m_pDevice->SetTexture(0, NULL);

    sprintf(dbg, "[SpriteRenderer] Execute END: drawCalls=%d\n", m_drawCalls);
    OutputDebugStringA(dbg);

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

    LPDIRECT3DTEXTURE9 tex = NULL;
    std::map<WORD, LPDIRECT3DTEXTURE9>::iterator it = m_textureMap.find(textureID);
    if (it != m_textureMap.end()) {
        tex = it->second;
    }
    m_pDevice->SetTexture(0, tex);
    if (m_pShaderManager) {
        m_pShaderManager->SetLocalUniforms(tex, 0.0f);
    }
}

void SpriteRenderer::SetShader(WORD shaderID) {
    if (!m_pShaderManager) return;
    
    // Проверяем валидность шейдера
    if ((ShaderID)shaderID == SHADER_INVALID || !m_pShaderManager->HasShader((ShaderID)shaderID)) {
        return;
    }
    
    // Завершаем предыдущий шейдер если он активен
    if (m_pShaderManager->GetCurrentShaderID() != SHADER_INVALID) {
        m_pShaderManager->EndPass();
        m_pShaderManager->EndShader();
    }
    
    if (!m_pShaderManager->SetActiveShader((ShaderID)shaderID)) return;
    
    m_pShaderManager->BeginShader();
    
    if (shaderID == SHADER_TERRAIN || shaderID == SHADER_WORLD) {
        const D3DXMATRIX& viewProj = m_pShaderManager->GetShaderMatrix(static_cast<ShaderID>(shaderID));
        m_pShaderManager->SetMatrix("gViewProj", (const float*)&viewProj);
    } else if (shaderID == SHADER_UI) {
        D3DXMATRIX ortho;
        D3DXMatrixOrthoOffCenterLH(&ortho, 0.0f, 1280.0f, 720.0f, 0.0f, 0.0f, 1.0f);
        m_pShaderManager->SetMatrix("gScreenProj", (const float*)&ortho);
    } else {
        m_pShaderManager->SetMatrix("WVP", m_projMatrix);
    }
    
    m_pShaderManager->Commit();
}

}
