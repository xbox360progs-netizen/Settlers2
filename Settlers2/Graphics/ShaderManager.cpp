#include "stdafx.h"
#include "ShaderManager.h"
#include <stdio.h>
#include <string>
#include <algorithm>

// StateCache Implementation
ShaderManager::StateCache::StateCache() 
    : m_dirty(false) {
    // Initialize all states to default values
    memset(m_currentStates, 0, sizeof(m_currentStates));
    m_currentStates[D3DRS_ZENABLE] = D3DZB_FALSE;
    m_currentStates[D3DRS_ALPHABLENDENABLE] = FALSE;
    m_currentStates[D3DRS_SRCBLEND] = D3DBLEND_SRCALPHA;
    m_currentStates[D3DRS_DESTBLEND] = D3DBLEND_INVSRCALPHA;
    m_currentStates[D3DRS_CULLMODE] = D3DCULL_NONE;
}

void ShaderManager::StateCache::SetRenderState(LPDIRECT3DDEVICE9 device, D3DRENDERSTATETYPE state, DWORD value) {
    if (m_currentStates[state] != value) {
        device->SetRenderState(state, value);
        m_currentStates[state] = value;
        m_dirty = true;
    }
}

void ShaderManager::StateCache::ResetDirtyStates(LPDIRECT3DDEVICE9 device, const RenderStateBlock& targetStates) {
    if (!m_dirty) return;
    
    // Reset to target states
    SetRenderState(device, D3DRS_ZENABLE, targetStates.zEnable);
    SetRenderState(device, D3DRS_ALPHABLENDENABLE, targetStates.alphaBlendEnable);
    SetRenderState(device, D3DRS_SRCBLEND, targetStates.srcBlend);
    SetRenderState(device, D3DRS_DESTBLEND, targetStates.destBlend);
    SetRenderState(device, D3DRS_CULLMODE, targetStates.cullMode);
    
    m_dirty = false;
}

ShaderManager::ShaderManager()
    : m_pDevice(NULL), m_pActiveShader(NULL), m_numPasses(0) {
}

ShaderManager::~ShaderManager() {
    Shutdown();
}

HRESULT ShaderManager::Initialize(LPDIRECT3DDEVICE9 device) {
    m_pDevice = device;
    return S_OK;
}

void ShaderManager::Shutdown() {
    for (std::map<std::string, Shader>::iterator it = m_shaders.begin();
         it != m_shaders.end(); ++it) {
        if (it->second.pEffect) {
            it->second.pEffect->Release();
            it->second.pEffect = NULL;
        }
    }
    m_shaders.clear();
    m_pActiveShader = NULL;
    m_pDevice = NULL;
}

HRESULT ShaderManager::LoadShader(const char* name, const char* filepath, const char* techniqueName) {
    if (!m_pDevice) return E_FAIL;

    if (HasShader(name)) {
        std::string msg = "Shader '" + std::string(name) + "' already loaded\n";
        OutputDebugStringA(msg.c_str());
        return S_OK;
    }

    Shader shader;
    shader.pEffect = NULL;
    shader.hTechnique = NULL;
    shader.hMatOrtho = NULL;
    shader.hTexture = NULL;

    ID3DXBuffer* errors = NULL;

    std::string msg = "Loading shader '" + std::string(name) + "' from: " + std::string(filepath) + "\n";
    OutputDebugStringA(msg.c_str());

    HRESULT hr = D3DXCreateEffectFromFileA(
        m_pDevice,
        filepath,
        NULL, NULL, 0, NULL,
        &shader.pEffect,
        &errors
    );

    if (FAILED(hr)) {
        if (errors) {
            msg = "--- SHADER COMPILE ERROR for '" + std::string(name) + "' ---\n";
            OutputDebugStringA(msg.c_str());
            OutputDebugStringA((const char*)errors->GetBufferPointer());
            OutputDebugStringA("--------------------------------------\n");
            errors->Release();
        } else {
            msg = "Shader file not found: " + std::string(filepath) + "\n";
            OutputDebugStringA(msg.c_str());
        }
        return hr;
    }

    shader.hTechnique = shader.pEffect->GetTechniqueByName(techniqueName);
    if (!shader.hTechnique) {
        msg = "Technique '" + std::string(techniqueName) + "' not found in shader '" + std::string(name) + "'\n";
        OutputDebugStringA(msg.c_str());
        shader.pEffect->Release();
        return E_FAIL;
    }

    shader.hMatOrtho = shader.pEffect->GetParameterByName(NULL, "matOrtho");
    shader.hTexture = shader.pEffect->GetParameterByName(NULL, "g_texture");

    m_shaders[name] = shader;

    msg = "Shader '" + std::string(name) + "' loaded successfully\n";
    OutputDebugStringA(msg.c_str());

    return S_OK;
}

bool ShaderManager::SetActiveShader(const char* name) {
    std::map<std::string, Shader>::iterator it = m_shaders.find(name);
    if (it == m_shaders.end()) {
        std::string msg = "Shader '" + std::string(name) + "' not found\n";
        OutputDebugStringA(msg.c_str());
        return false;
    }

    // If the same shader is already active, don't EndShader+restart it
    // (calling End() on an already-ended effect causes BSOD on Xbox 360)
    if (m_pActiveShader == &it->second) {
        return true;
    }

    if (m_pActiveShader) {
        EndShader();
    }

    m_pActiveShader = &it->second;
    return true;
}

ShaderManager::Shader* ShaderManager::GetShader(const char* name) {
    std::map<std::string, Shader>::iterator it = m_shaders.find(name);
    if (it == m_shaders.end()) {
        return NULL;
    }
    return &it->second;
}

void ShaderManager::BeginShader() {
    if (!m_pActiveShader || !m_pActiveShader->pEffect) return;

    m_pActiveShader->pEffect->SetTechnique(m_pActiveShader->hTechnique);
    m_pActiveShader->pEffect->Begin(&m_numPasses, 0);
}

void ShaderManager::BeginPass(UINT pass) {
    if (m_pActiveShader && m_pActiveShader->pEffect) {
        m_pActiveShader->pEffect->BeginPass(pass);
    }
}

void ShaderManager::EndPass() {
    if (m_pActiveShader && m_pActiveShader->pEffect) {
        m_pActiveShader->pEffect->EndPass();
    }
}

void ShaderManager::EndShader() {
    if (!m_pActiveShader || !m_pActiveShader->pEffect) return;

    m_pActiveShader->pEffect->End();
    m_pActiveShader = NULL;  // Clear so SetActiveShader won't call End() again
}

void ShaderManager::Commit() {
    // Xbox 360 Optimization: Explicit commit for multi-threaded rendering
    // This ensures the command buffer is filled only once per batch
    if (m_pActiveShader && m_pActiveShader->pEffect) {
        m_pActiveShader->pEffect->CommitChanges();
    }
}

void ShaderManager::SetMatrix(const char* paramName, const float* matrix) {
    if (!m_pActiveShader || !m_pActiveShader->pEffect) return;

    auto it = m_pActiveShader->hParams.find(paramName);
    D3DXHANDLE hParam = (it != m_pActiveShader->hParams.end()) ? it->second : NULL;

    if (it == m_pActiveShader->hParams.end()) {
        hParam = m_pActiveShader->pEffect->GetParameterByName(NULL, paramName);
        m_pActiveShader->hParams[paramName] = hParam;
    }

    if (hParam) {
        m_pActiveShader->pEffect->SetMatrix(hParam, (const D3DXMATRIX*)matrix);
        // Removed automatic CommitChanges() for Xbox 360 multi-threading
        // Use explicit Commit() call instead
    }
}

void ShaderManager::SetVector(const char* paramName, const float* vector) {
    if (!m_pActiveShader || !m_pActiveShader->pEffect) return;

    auto it = m_pActiveShader->hParams.find(paramName);
    D3DXHANDLE hParam = (it != m_pActiveShader->hParams.end()) ? it->second : NULL;

    if (it == m_pActiveShader->hParams.end()) {
        hParam = m_pActiveShader->pEffect->GetParameterByName(NULL, paramName);
        m_pActiveShader->hParams[paramName] = hParam;
    }

    if (hParam) {
        m_pActiveShader->pEffect->SetVector(hParam, (const D3DXVECTOR4*)vector);
        // Removed automatic CommitChanges() for Xbox 360 multi-threading
        // Use explicit Commit() call instead
    }
}

void ShaderManager::SetTexture(const char* paramName, LPDIRECT3DBASETEXTURE9 pTexture) {
    if (!m_pActiveShader || !m_pActiveShader->pEffect) {
        return;
    }

    D3DXHANDLE hParam = m_pActiveShader->pEffect->GetParameterByName(NULL, paramName);
    if (hParam) {
        m_pActiveShader->pEffect->SetTexture(hParam, pTexture);
        // Removed automatic CommitChanges() for Xbox 360 multi-threading
        // Use explicit Commit() call instead
    }
    
    // === BATCH BREAKING ===
    // Track texture changes to mark current batch as complete
    // This allows material-based sorting to work correctly
    static LPDIRECT3DBASETEXTURE9 s_lastTexture = NULL;
    if (s_lastTexture != pTexture) {
        // Texture changed - this is a batch break point
        s_lastTexture = pTexture;
        // The sorting system will handle grouping by texture automatically
    }
}

void ShaderManager::OnLostDevice() {
    for (std::map<std::string, Shader>::iterator it = m_shaders.begin();
         it != m_shaders.end(); ++it) {
        if (it->second.pEffect) {
            it->second.pEffect->OnLostDevice();
        }
    }
}

void ShaderManager::OnResetDevice() {
    for (std::map<std::string, Shader>::iterator it = m_shaders.begin();
         it != m_shaders.end(); ++it) {
        if (it->second.pEffect) {
            it->second.pEffect->OnResetDevice();
        }
    }
}

bool ShaderManager::HasShader(const char* name) const {
    return m_shaders.find(name) != m_shaders.end();
}

// === Queue-based Rendering System Implementation ===

static DWORD s_currentVertexOffset = 0;
static DWORD s_batchIndex = 0;

void ShaderManager::Submit(const RenderCommand& cmd) {
    RenderCommand cmdWithOffset = cmd;
    cmdWithOffset.vertexStart = s_currentVertexOffset;
    cmdWithOffset.batchIndex = s_batchIndex++;
    
    // Update vertex offset for next batch
    s_currentVertexOffset += cmd.vertexCount;
    
    m_commandQueue.push_back(cmdWithOffset);
}

void ShaderManager::SubmitDrawBatch(const DrawBatch& batch) {
    m_drawBatches.push_back(batch);
}

void ShaderManager::SubmitBatch(const RenderBatch& batch) {
    m_batches.push_back(batch);
}

void ShaderManager::ClearQueue() {
    m_commandQueue.clear();
}

void ShaderManager::ClearDrawBatches() {
    m_drawBatches.clear();
}

void ShaderManager::ClearBatches() {
    m_batches.clear();
}

void ShaderManager::SortQueue() {
    // Sort by depth (back-to-front for alpha), then shader, then texture
    std::sort(m_commandQueue.begin(), m_commandQueue.end());
}

void ShaderManager::SortDrawBatches() {
    // Sort by texture first (expensive switch), then shader, then depth (State Sorting)
    std::sort(m_drawBatches.begin(), m_drawBatches.end());
}

void ShaderManager::SortBatches() {
    // Sort commands by depth (back-to-front), shader, and texture
    std::sort(m_batches.begin(), m_batches.end());
}

void ShaderManager::ExecuteQueue(LPDIRECT3DVERTEXBUFFER9 pVB, LPDIRECT3DINDEXBUFFER9 pIB, 
                                LPDIRECT3DVERTEXDECLARATION9 pDecl, DWORD vertexStride) {
    if (m_commandQueue.empty()) return;
    
    // === SORT COMMANDS BY DEPTH (back-to-front for alpha) ===
    // This ensures proper layering regardless of submission order
    // Ground tiles (depth=1.0) render first, UI (depth=0.1) renders on top
    std::sort(m_commandQueue.begin(), m_commandQueue.end());
    
    // === RESET STATES: Force clean slate at start of render pass ===
    // This prevents "garbage" states from previous rendering operations
    m_pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    
    // Set vertex declaration and streams once
    m_pDevice->SetVertexDeclaration(pDecl);
    m_pDevice->SetStreamSource(0, pVB, 0, vertexStride);
    m_pDevice->SetIndices(pIB);
    
    // Process commands in sorted order (Master Loop - final render pass)
    for (size_t i = 0; i < m_commandQueue.size(); ++i) {
        const RenderCommand& cmd = m_commandQueue[i];
        
        // Set shader if changed
        if (m_pActiveShader != cmd.pShader) {
            if (m_pActiveShader) {
                EndShader();
            }
            m_pActiveShader = cmd.pShader;
            BeginShader();
        }
        
        // Apply render states for this command
        m_stateCache.ResetDirtyStates(m_pDevice, cmd.states);
        
        // Set texture
        SetTexture("g_texture", cmd.pTexture);
        Commit();
        
        // Draw this command based on batch type (Single vs Instanced)
        BeginPass(0);
        
        // Switch based on render type
        switch (cmd.batchType) {
            case 0: // Standard/Single sprite rendering
                // Use BaseVertexIndex parameter for ring buffer support
                // This tells GPU where this batch's vertices start in the buffer
                m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 
                                               cmd.vertexStart, // BaseVertexIndex
                                               0,               // MinIndex
                                               cmd.vertexCount, // NumVertices
                                               0,               // StartIndex
                                               cmd.primitiveCount);
                break;
                
            case 1: // Instanced rendering
                // For instanced rendering, we would need to set instance buffer here
                // For now, fall back to standard rendering with vertex offset
                m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 
                                               cmd.vertexStart, // BaseVertexIndex
                                               0,               // MinIndex
                                               cmd.vertexCount, // NumVertices
                                               0,               // StartIndex
                                               cmd.primitiveCount);
                break;
                
            default:
                // Default to standard rendering with vertex offset
                m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 
                                               cmd.vertexStart, // BaseVertexIndex
                                               0,               // MinIndex
                                               cmd.vertexCount, // NumVertices
                                               0,               // StartIndex
                                               cmd.primitiveCount);
                break;
        }
        
        EndPass();
    }
    
    // Clean up
    if (m_pActiveShader) {
        EndShader();
    }
    
    // Clear command queue after execution
    m_commandQueue.clear();
    
    // === CLEANUP: Reset all texture slots to NULL ===
    // This guarantees TextManager starts with clean slate next frame
    m_pDevice->SetTexture(0, NULL);
    m_pDevice->SetTexture(1, NULL);
    m_pDevice->SetTexture(2, NULL);
    m_pDevice->SetTexture(3, NULL);
    
    // Reset vertex offset tracking for next frame
    s_currentVertexOffset = 0;
    s_batchIndex = 0;
}

void ShaderManager::ExecuteBatches(LPDIRECT3DVERTEXBUFFER9 pVB, LPDIRECT3DINDEXBUFFER9 pIB, 
                                   LPDIRECT3DVERTEXDECLARATION9 pDecl, DWORD vertexStride) {
    if (m_batches.empty()) return;
    
    // Set vertex declaration and streams once
    m_pDevice->SetVertexDeclaration(pDecl);
    m_pDevice->SetStreamSource(0, pVB, 0, vertexStride);
    m_pDevice->SetIndices(pIB);
    
    // Process batches in sorted order
    for (size_t i = 0; i < m_batches.size(); ++i) {
        const RenderBatch& batch = m_batches[i];
        
        // Set shader if changed
        if (m_pActiveShader != batch.pShader) {
            if (m_pActiveShader) {
                EndShader();
            }
            m_pActiveShader = batch.pShader;
            BeginShader();
        }
        
        // Apply render states for this batch
        m_stateCache.ResetDirtyStates(m_pDevice, batch.states);
        
        // Set texture
        SetTexture("g_texture", batch.pTexture);
        Commit();
        
        // Draw this batch
        BeginPass(0);
        m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 
                                       batch.primitiveCount * 3, 
                                       batch.startVertex, 
                                       batch.primitiveCount);
        EndPass();
    }
    
    // Clean up
    if (m_pActiveShader) {
        EndShader();
    }
    
    // Clear batches after execution
    m_batches.clear();
    
    // === CLEANUP: Reset all texture slots to NULL ===
    m_pDevice->SetTexture(0, NULL);
    m_pDevice->SetTexture(1, NULL);
    m_pDevice->SetTexture(2, NULL);
    m_pDevice->SetTexture(3, NULL);
}

void ShaderManager::ExecuteDrawBatches(LPDIRECT3DVERTEXBUFFER9 pVB, LPDIRECT3DINDEXBUFFER9 pIB, 
                                       LPDIRECT3DVERTEXDECLARATION9 pDecl, DWORD vertexStride) {
    if (m_drawBatches.empty()) return;
    
    // Set vertex declaration and streams once
    m_pDevice->SetVertexDeclaration(pDecl);
    m_pDevice->SetStreamSource(0, pVB, 0, vertexStride);
    m_pDevice->SetIndices(pIB);
    
    // Process batches in sorted order (material-based sorting)
    for (size_t i = 0; i < m_drawBatches.size(); ++i) {
        const DrawBatch& batch = m_drawBatches[i];
        
        // Set shader if changed
        if (m_pActiveShader != batch.pShader) {
            if (m_pActiveShader) {
                EndShader();
            }
            m_pActiveShader = batch.pShader;
            BeginShader();
        }
        
        // Set texture if changed (batch breaking)
        SetTexture("g_texture", batch.pTexture);
        Commit();
        
        // Draw this batch
        BeginPass(0);
        m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 
                                       batch.indexCount / 3 * 4, 
                                       batch.vertexOffset, 
                                       batch.indexCount / 3);
        EndPass();
    }
    
    // Clean up
    if (m_pActiveShader) {
        EndShader();
    }
    
    // Clear draw batches after execution
    m_drawBatches.clear();
    
    // === CLEANUP: Reset all texture slots to NULL ===
    m_pDevice->SetTexture(0, NULL);
    m_pDevice->SetTexture(1, NULL);
    m_pDevice->SetTexture(2, NULL);
    m_pDevice->SetTexture(3, NULL);
}
