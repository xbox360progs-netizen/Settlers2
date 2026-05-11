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
    : m_pDevice(NULL), m_pActiveShader(NULL), m_numPasses(0), m_currentShaderID(SHADER_INVALID), m_isLocked(false), m_hasFrameViewProj(false) {
    // Reserve memory for command queue to avoid expensive reallocations on Xbox 360
    m_commandQueue.reserve(2000);
    m_drawBatches.reserve(500);
    m_batches.reserve(500);
    D3DXMatrixIdentity(&m_frameViewProj);
    D3DXMatrixIdentity(&m_cachedView);
    D3DXMatrixIdentity(&m_cachedProj);
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

// === CENTRALIZED SHADER MANAGEMENT (Xbox 360 Safe) ===

// Load all shaders at startup (FatalError if any fail)
HRESULT ShaderManager::LoadAll() {
    HRESULT hr;
    
    // Load SPRITE shader
    hr = LoadShader("SPRITE", "Media/Graphics/Shaders/Sprite.fx", "SpriteBatchTech");
    if (FAILED(hr)) {
        OutputDebugStringA("[ShaderManager] FATAL: Failed to load SPRITE shader\n");
        return hr;
    }
    
    // Load SPRITE_CONSTANT_INSTANCED shader
    hr = LoadShader("SPRITE_CONSTANT_INSTANCED", "Media/Graphics/Shaders/SpriteConstantInstanced.fx", "SpriteBatchTech");
    if (FAILED(hr)) {
        OutputDebugStringA("[ShaderManager] FATAL: Failed to load SPRITE_CONSTANT_INSTANCED shader\n");
        return hr;
    }
    
    // Load RADIALMENU shader
    hr = LoadShader("RADIALMENU", "Media/Graphics/Shaders/RadialMenu.fx", "RadialMenuTech");
    if (FAILED(hr)) {
        OutputDebugStringA("[ShaderManager] FATAL: Failed to load RADIALMENU shader\n");
        return hr;
    }
    
    OutputDebugStringA("[ShaderManager] All shaders loaded successfully\n");
    return S_OK;
}

// Prepare shader for rendering (activate effect with global uniforms)
void ShaderManager::Prepare(ShaderID id, const D3DXMATRIX* pViewProj) {
    if (!ValidateShader(id)) {
        OutputDebugStringA("[ShaderManager] Prepare: Invalid shader ID\n");
        return;
    }
    
    // Lazy switching: only switch if different from current
    if (id == m_currentShaderID) {
        // Already active, just update global uniforms
        if (pViewProj) {
            SetGlobalUniforms(pViewProj);
        }
        return;
    }
    
    // End previous shader if active
    if (m_currentShaderID != SHADER_INVALID) {
        EndCurrent();
    }
    
    // Map ShaderID to shader name for legacy compatibility
    const char* shaderName = nullptr;
    switch (id) {
        case SHADER_SPRITE:
            shaderName = "SPRITE";
            break;
        case SHADER_SPRITE_CONSTANT_INSTANCED:
            shaderName = "SPRITE_CONSTANT_INSTANCED";
            break;
        case SHADER_RADIALMENU:
            shaderName = "RADIALMENU";
            break;
        default:
            return;
    }
    
    // Find and activate shader
    std::map<std::string, Shader>::iterator it = m_shaders.find(shaderName);
    if (it != m_shaders.end() && it->second.pEffect) {
        m_pActiveShader = &(it->second);
        m_currentShaderID = id;
        
        // Begin shader
        BeginShader();
        
        // Set global uniforms
        if (pViewProj) {
            SetGlobalUniforms(pViewProj);
        }
    }
}

// End current shader (deactivate effect)
void ShaderManager::EndCurrent() {
    if (m_pActiveShader) {
        EndShader();
        m_pActiveShader = nullptr;
    }
    m_currentShaderID = SHADER_INVALID;
}

// Set global uniforms (frame-wide data: view/projection matrix, time, etc.)
void ShaderManager::SetGlobalUniforms(const D3DXMATRIX* pViewProj) {
    if (!m_pActiveShader || !pViewProj) return;
    SetMatrix("WorldViewProjection", (const float*)pViewProj);
    Commit();
}

// Set local uniforms (per-entity data: texture, depth, etc.)
void ShaderManager::SetLocalUniforms(LPDIRECT3DTEXTURE9 pTexture, float depth) {
    if (!m_pActiveShader) return;
    if (pTexture) {
        SetTexture("g_texture", pTexture);
    }
    Commit();
}

// Update constants (unified method for texture + world matrix)
void ShaderManager::UpdateConstants(LPDIRECT3DTEXTURE9 pTexture, const D3DXMATRIX* pWorldMatrix) {
    if (!m_pActiveShader) return;
    if (pTexture) {
        SetTexture("g_texture", pTexture);
    }
    if (pWorldMatrix) {
        SetMatrix("World", (const float*)pWorldMatrix);
    }
    Commit();
}

// State locking (prevents external state corruption during ExecuteQueue)
void ShaderManager::Lock() {
    m_isLocked = true;
}

void ShaderManager::Unlock() {
    m_isLocked = false;
}

// Commit changes (Xbox 360: critical before Draw)
void ShaderManager::CommitChanges() {
    if (m_pActiveShader && m_pActiveShader->pEffect) {
        m_pActiveShader->pEffect->CommitChanges();
    }
}

// Set shader parameters from RenderCommand (centralized parameter dispatch)
// Manager decides which .fx parameter gets which data based on shaderID
void ShaderManager::SetShaderParameters(const RenderCommand& cmd) {
    if (!m_pActiveShader) return;
    
    ShaderID id = static_cast<ShaderID>(cmd.shaderID);
    
    switch (id) {
        case SHADER_SPRITE:
        case SHADER_SPRITE_CONSTANT_INSTANCED:
        case SHADER_TERRAIN:
            // Sprite shaders: set texture + ortho/world matrix
            if (cmd.pTexture) {
                SetTexture("g_texture", cmd.pTexture);
            }
            // For world-space objects, apply camera; for UI, use identity
            if (cmd.isUI) {
                D3DXMATRIX matOrtho;
                D3DXMatrixOrthoOffCenterLH(&matOrtho, 0.0f, 1280.0f, 720.0f, 0.0f, 0.0f, 1.0f);
                SetMatrix("matOrtho", (const float*)&matOrtho);
            } else if (m_hasFrameViewProj) {
                SetMatrix("matOrtho", (const float*)&m_frameViewProj);
            }
            break;
            
        case SHADER_RADIALMENU:
        case SHADER_UI:
            // UI/RadialMenu shaders: set WorldViewProjection
            if (cmd.isUI) {
                D3DXMATRIX matOrtho;
                D3DXMatrixOrthoOffCenterLH(&matOrtho, 0.0f, 1280.0f, 720.0f, 0.0f, 0.0f, 1.0f);
                SetMatrix("WorldViewProjection", (const float*)&matOrtho);
            } else if (m_hasFrameViewProj) {
                SetMatrix("WorldViewProjection", (const float*)&m_frameViewProj);
            }
            if (cmd.pTexture) {
                SetTexture("g_texture", cmd.pTexture);
            }
            break;
            
        default:
            break;
    }
    
    Commit();
}

// Set frame-wide ViewProjection matrix (Global Constant Buffer)
void ShaderManager::SetFrameViewProj(const D3DXMATRIX* pViewProj) {
    if (pViewProj) {
        m_frameViewProj = *pViewProj;
        m_hasFrameViewProj = true;
    } else {
        D3DXMatrixIdentity(&m_frameViewProj);
        m_hasFrameViewProj = false;
    }
}

// Update global camera matrices (view + projection) for all loaded shaders
// Caches internally and propagates to all active effects
void ShaderManager::UpdateGlobalMatrices(const D3DXMATRIX* pView, const D3DXMATRIX* pProj) {
    if (pView) m_cachedView = *pView;
    if (pProj) m_cachedProj = *pProj;
    
    // Compute combined ViewProjection
    D3DXMatrixMultiply(&m_frameViewProj, &m_cachedView, &m_cachedProj);
    m_hasFrameViewProj = true;
    
    // Propagate to all loaded shaders (set WorldViewProjection on each)
    Shader* pPreviousShader = m_pActiveShader; // Save current shader
    
    for (std::map<std::string, Shader>::iterator it = m_shaders.begin(); it != m_shaders.end(); ++it) {
        if (it->second.pEffect) {
            m_pActiveShader = &(it->second); // Temporarily set active for SetMatrix
            
            // Set WorldViewProjection (common name for most shaders)
            SetMatrix("WorldViewProjection", (const float*)&m_frameViewProj);
            
            // Also set matOrtho for sprite shaders (same as ViewProj for 2D)
            SetMatrix("matOrtho", (const float*)&m_frameViewProj);
            
            // Also set matWVP for sprite shaders (alternative name)
            SetMatrix("matWVP", (const float*)&m_frameViewProj);
        }
    }
    
    m_pActiveShader = pPreviousShader; // Restore previous shader
}

// Validate shader handle
bool ShaderManager::ValidateShader(ShaderID id) const {
    return id >= SHADER_INVALID && id < SHADER_COUNT;
}

// Legacy ApplyShader (for backward compatibility with int-based API)
void ShaderManager::ApplyShader(int shaderID) {
    ShaderID id = static_cast<ShaderID>(shaderID);
    Prepare(id, NULL);
}

void ShaderManager::ExecuteQueue(LPDIRECT3DVERTEXBUFFER9 pVB, LPDIRECT3DINDEXBUFFER9 pIB, 
                                LPDIRECT3DVERTEXDECLARATION9 pDecl, DWORD vertexStride,
                                const D3DXMATRIX* pViewProj) {
    if (m_commandQueue.empty()) return;
    
    // === GLOBAL CONSTANT BUFFER: Set ViewProj once per frame ===
    SetFrameViewProj(pViewProj);
    
    // === SORT COMMANDS: shader-first for lazy batching ===
    // Sorting: shaderID (most expensive switch) > texture > depth (back-to-front)
    std::sort(m_commandQueue.begin(), m_commandQueue.end());
    
    // === STATE LOCKING: Prevent external state corruption ===
    Lock();
    
    // Set vertex declaration and streams once for entire frame
    m_pDevice->SetVertexDeclaration(pDecl);
    m_pDevice->SetStreamSource(0, pVB, 0, vertexStride);
    m_pDevice->SetIndices(pIB);
    
    // === LAZY BATCHING: Process commands with minimal state switches ===
    ShaderID currentShaderID = SHADER_INVALID;
    bool passActive = false;  // Track if we're inside a BeginPass/EndPass block
    
    for (size_t i = 0; i < m_commandQueue.size(); ++i) {
        const RenderCommand& cmd = m_commandQueue[i];
        
        // Custom draw callback: manages its own shader/state/pass lifecycle
        if (cmd.batchType == 2) {
            // End current pass if active before custom draw
            if (passActive) {
                EndPass();
                passActive = false;
            }
            if (cmd.customDraw) {
                m_stateCache.ResetDirtyStates(m_pDevice, cmd.states);
                cmd.customDraw(m_pDevice, this, cmd.customUserData);
            }
            currentShaderID = SHADER_INVALID; // Force shader re-prepare after custom draw
            continue;
        }
        
        // === LAZY SHADER SWITCHING ===
        ShaderID cmdShaderID = static_cast<ShaderID>(cmd.shaderID);
        if (cmdShaderID != currentShaderID) {
            // End previous pass if active
            if (passActive) {
                EndPass();
                passActive = false;
            }
            // End previous shader if active
            if (currentShaderID != SHADER_INVALID) {
                EndCurrent();
            }
            // Prepare new shader (handles BeginShader + global uniforms)
            Prepare(cmdShaderID, pViewProj);
            currentShaderID = cmdShaderID;
        }
        
        // Apply render states for this command
        m_stateCache.ResetDirtyStates(m_pDevice, cmd.states);
        
        // === CENTRALIZED PARAMETER DISPATCH ===
        SetShaderParameters(cmd);
        
        // === LAZY PASS MANAGEMENT ===
        // Only begin a new pass if we're not already inside one
        if (!passActive) {
            BeginPass(0);
            passActive = true;
        }
        
        // Xbox 360: CommitChanges() CRITICAL before Draw (apply constant buffer updates)
        CommitChanges();
        
        // Draw this command based on batch type
        switch (cmd.batchType) {
            case 0: // Standard/Single sprite rendering
                m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 
                                               cmd.vertexStart,
                                               0,
                                               cmd.vertexCount,
                                               0,
                                               cmd.primitiveCount);
                break;
                
            case 1: // Instanced rendering
                m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 
                                               cmd.vertexStart,
                                               0,
                                               cmd.vertexCount,
                                               0,
                                               cmd.primitiveCount);
                break;
                
            default:
                m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 
                                               cmd.vertexStart,
                                               0,
                                               cmd.vertexCount,
                                               0,
                                               cmd.primitiveCount);
                break;
        }
    }
    
    // === CLEANUP: End any active pass and shader ===
    if (passActive) {
        EndPass();
    }
    EndCurrent();
    
    // Unlock state after ExecuteQueue completes
    Unlock();
    
    // Clear command queue after execution
    m_commandQueue.clear();
    
    // === CLEANUP: Reset all texture slots to NULL ===
    m_pDevice->SetTexture(0, NULL);
    m_pDevice->SetTexture(1, NULL);
    m_pDevice->SetTexture(2, NULL);
    m_pDevice->SetTexture(3, NULL);
    
    // Reset vertex offset tracking for next frame
    s_currentVertexOffset = 0;
    s_batchIndex = 0;
    
    // Clear frame ViewProj for next frame
    m_hasFrameViewProj = false;
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
