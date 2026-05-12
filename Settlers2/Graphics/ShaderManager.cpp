#include "stdafx.h"
#include "ShaderManager.h"
#include "Renderer.h"
#include "SpriteRenderer.h"
#include <d3dx9.h>
#include <d3d9.h>
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
    : m_pDevice(NULL), m_pActiveShader(NULL), m_pActiveEffect(NULL), m_numPasses(0), m_currentShaderID(SHADER_INVALID), m_isLocked(false), m_hasFrameViewProj(false) {
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

// Private shader loading helper
HRESULT ShaderManager::LoadInternal(ShaderID id, const char* path, const char* technique) {
    if (!m_pDevice) {
        OutputDebugStringA("[ShaderManager] ERROR: Device not initialized\n");
        return E_FAIL;
    }

    if (m_effects.find(id) != m_effects.end()) {
        // Shader already loaded
        return S_OK;
    }

    ID3DXEffect* pEffect = NULL;
    ID3DXBuffer* pErrorBuffer = NULL;

    DWORD dwFlags = D3DXSHADER_DEBUG;
    
    HRESULT hr = D3DXCreateEffectFromFileA(
        m_pDevice,
        path,
        NULL,
        NULL,
        dwFlags,
        NULL,
        &pEffect,
        &pErrorBuffer
    );

    if (FAILED(hr)) {
        if (pErrorBuffer) {
            char errorMsg[512];
            sprintf_s(errorMsg, "[ShaderManager] ERROR: Failed to load shader '%s': %s\n", 
                     path, (char*)pErrorBuffer->GetBufferPointer());
            OutputDebugStringA(errorMsg);
            pErrorBuffer->Release();
        } else {
            char errorMsg[256];
            sprintf_s(errorMsg, "[ShaderManager] ERROR: Failed to load shader '%s' (HRESULT: 0x%08X)\n", 
                     path, hr);
            OutputDebugStringA(errorMsg);
        }
        return hr;
    }

    // Set technique
    if (technique && pEffect) {
        D3DXHANDLE hTechnique = pEffect->GetTechniqueByName(technique);
        if (hTechnique) {
            pEffect->SetTechnique(hTechnique);
        }
    }

    m_effects[id] = pEffect;
    
    char logMsg[256];
    sprintf_s(logMsg, "[ShaderManager] Loaded shader ID %d from '%s'\n", id, path);
    OutputDebugStringA(logMsg);

    return S_OK;
}

// Centralized initialization: load all required shaders at startup
bool ShaderManager::Init() {
    if (!m_pDevice) {
        OutputDebugStringA("[ShaderManager] ERROR: Cannot Init - device not initialized\n");
        return false;
    }

    // Load base shaders through centralized registry
    if (!LoadBaseShaders()) {
        OutputDebugStringA("[ShaderManager] ERROR: LoadBaseShaders failed\n");
        return false;
    }
	return true;
}

// Load base shaders (Sprite.fx, UI.fx) for centralized shader registry
bool ShaderManager::LoadBaseShaders() {
    if (!m_pDevice) {
        OutputDebugStringA("[ShaderManager] ERROR: Cannot LoadBaseShaders - device not initialized\n");
        return false;
    }


    bool allSuccess = true;

    // Load World.fx (for world-space rendering: tiles, trees, units)
    if (FAILED(LoadShader(SHADER_WORLD, "game:\\Media\\Shaders\\World.fx", "WorldTech"))) {
        OutputDebugStringA("[ShaderManager] ERROR: Failed to load SHADER_WORLD (World.fx)\n");
        allSuccess = false;
    }

    // Load UI.fx (for UI elements: text, menus, cursor)
    if (FAILED(LoadShader(SHADER_UI, "game:\\Media\\Shaders\\UI.fx", "UITech"))) {
        OutputDebugStringA("[ShaderManager] ERROR: Failed to load SHADER_UI (UI.fx)\n");
        allSuccess = false;
    }

    // Font shader removed - text now uses SHADER_SPRITE through SpriteRenderer

    // Load SpriteShader.fx (for sprite rendering with SpriteBatchTech)
    if (FAILED(LoadShader(SHADER_SPRITE, "game:\\Media\\Shaders\\SpriteShader.fx", "SpriteBatchTech"))) {
        OutputDebugStringA("[ShaderManager] ERROR: Failed to load SHADER_SPRITE (SpriteShader.fx)\n");
        allSuccess = false;
    }

    // Load RadialMenu.fx (for radial menu)
    // Commented out - RadialMenu.fx doesn't exist yet
    // if (FAILED(LoadShader(SHADER_RADIALMENU, "game:\\Media\\Shaders\\RadialMenu.fx", "RadialMenu"))) {
    //     OutputDebugStringA("[ShaderManager] ERROR: Failed to load SHADER_RADIALMENU (RadialMenu.fx)\n");
    //     allSuccess = false;
    // }

    // Load SpriteConstantInstanced.fx (for instanced sprites)
    if (FAILED(LoadShader(SHADER_SPRITE_CONSTANT_INSTANCED, "game:\\Media\\Shaders\\SpriteConstantInstanced.fx", "SpriteConstantInstancedTech"))) {
        OutputDebugStringA("[ShaderManager] ERROR: Failed to load SHADER_SPRITE_CONSTANT_INSTANCED (SpriteConstantInstanced.fx)\n");
        allSuccess = false;
    }

    if (allSuccess) {
        OutputDebugStringA("[ShaderManager] LoadBaseShaders: All base shaders loaded successfully\n");
    } else {
        OutputDebugStringA("[ShaderManager] WARNING: Some base shaders failed to load\n");
    }

    return allSuccess;
}

// Get effect pointer by ShaderID (for direct parameter access when needed)
ID3DXEffect* ShaderManager::GetEffect(ShaderID id) {
    std::map<ShaderID, ID3DXEffect*>::iterator it = m_effects.find(id);
    if (it != m_effects.end()) {
        return it->second;
    }
    return NULL;
}

HRESULT ShaderManager::Initialize(LPDIRECT3DDEVICE9 device) {
    m_pDevice = device;
    return S_OK;
}

void ShaderManager::Shutdown() {
    // Release centralized shader effects
    for (std::map<ShaderID, ID3DXEffect*>::iterator it = m_effects.begin();
         it != m_effects.end(); ++it) {
        if (it->second) {
            it->second->Release();
        }
    }
    m_effects.clear();

    // Release legacy shader map
    for (std::map<ShaderID, Shader>::iterator it = m_shaders.begin();
         it != m_shaders.end(); ++it) {
        if (it->second.pEffect) {
            it->second.pEffect->Release();
            it->second.pEffect = NULL;
        }
    }
    m_shaders.clear();
    m_pActiveShader = NULL;
    m_pActiveEffect = NULL;
    m_pDevice = NULL;
}

HRESULT ShaderManager::LoadShader(ShaderID id, const char* filepath, const char* techniqueName) {
    if (!m_pDevice) return E_FAIL;

    if (HasShader(id)) {
        char msg[256];
        sprintf_s(msg, "[ShaderManager] Shader ID %d already loaded\n", id);
        OutputDebugStringA(msg);
        return S_OK;
    }

    Shader shader;
    shader.pEffect = NULL;
    shader.hTechnique = NULL;
    shader.hMatOrtho = NULL;
    shader.hTexture = NULL;

    ID3DXBuffer* errors = NULL;

    char msg[512];
    sprintf_s(msg, "[ShaderManager] Loading shader ID %d from: %s\n", id, filepath);
    OutputDebugStringA(msg);

    HRESULT hr = D3DXCreateEffectFromFileA(
        m_pDevice,
        filepath,
        NULL, NULL, 0, NULL,
        &shader.pEffect,
        &errors
    );

    if (FAILED(hr)) {
        if (errors) {
            char errorMsg[512];
            sprintf_s(errorMsg, "--- SHADER COMPILE ERROR for ID %d ---\n", id);
            OutputDebugStringA(errorMsg);
            OutputDebugStringA((const char*)errors->GetBufferPointer());
            OutputDebugStringA("--------------------------------------\n");
            errors->Release();
        } else {
            char errorMsg[256];
            sprintf_s(errorMsg, "Shader file not found: %s\n", filepath);
            OutputDebugStringA(errorMsg);
        }
        return hr;
    }

    shader.hTechnique = shader.pEffect->GetTechniqueByName(techniqueName);
    if (!shader.hTechnique) {
        char errorMsg[256];
        sprintf_s(errorMsg, "[ShaderManager] Technique '%s' not found in shader ID %d\n", techniqueName, id);
        OutputDebugStringA(errorMsg);
        shader.pEffect->Release();
        return E_FAIL;
    }

    shader.hMatOrtho = shader.pEffect->GetParameterByName(NULL, "matOrtho");
    shader.hTexture = shader.pEffect->GetParameterByName(NULL, "g_texture");

    m_shaders[id] = shader;
    
    // Also add to m_effects map for Prepare() to find
    m_effects[id] = shader.pEffect;

    char successMsg[256];
    sprintf_s(successMsg, "[ShaderManager] Shader ID %d stored in effects list\n", id);
    OutputDebugStringA(successMsg);

    return S_OK;
}

bool ShaderManager::SetActiveShader(ShaderID id) {
    std::map<ShaderID, Shader>::iterator it = m_shaders.find(id);
    if (it == m_shaders.end()) {
        char errorMsg[256];
        sprintf_s(errorMsg, "[ShaderManager] Shader ID %d not found\n", id);
        OutputDebugStringA(errorMsg);
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

ShaderManager::Shader* ShaderManager::GetShader(ShaderID id) {
    std::map<ShaderID, Shader>::iterator it = m_shaders.find(id);
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
    char dbgBuf[256];
    sprintf(dbgBuf, "[SM::SetTexture] paramName=%s, pTexture=%p, m_pActiveShader=%p\n", 
            paramName ? paramName : "NULL", pTexture, m_pActiveShader);
    OutputDebugStringA(dbgBuf);

    if (!m_pActiveShader || !m_pActiveShader->pEffect) {
        OutputDebugStringA("[SM::SetTexture] ERROR: No active shader or effect\n");
        return;
    }

    D3DXHANDLE hParam = m_pActiveShader->pEffect->GetParameterByName(NULL, paramName);
    if (hParam) {
        sprintf(dbgBuf, "[SM::SetTexture] hParam=%p, calling SetTexture\n", hParam);
        OutputDebugStringA(dbgBuf);
        m_pActiveShader->pEffect->SetTexture(hParam, pTexture);
        sprintf(dbgBuf, "[SM::SetTexture] SetTexture returned, pTexture=%p\n", pTexture);
        OutputDebugStringA(dbgBuf);
        // Removed automatic CommitChanges() for Xbox 360 multi-threading
        // Use explicit Commit() call instead
    } else {
        sprintf(dbgBuf, "[SM::SetTexture] WARNING: hParam is NULL for paramName=%s\n", paramName ? paramName : "NULL");
        OutputDebugStringA(dbgBuf);
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
    for (std::map<ShaderID, Shader>::iterator it = m_shaders.begin();
         it != m_shaders.end(); ++it) {
        if (it->second.pEffect) {
            it->second.pEffect->OnLostDevice();
        }
    }
}

void ShaderManager::OnResetDevice() {
    for (std::map<ShaderID, Shader>::iterator it = m_shaders.begin();
         it != m_shaders.end(); ++it) {
        if (it->second.pEffect) {
            it->second.pEffect->OnResetDevice();
        }
    }
}

bool ShaderManager::HasShader(ShaderID id) const {
    return m_shaders.find(id) != m_shaders.end();
}

// === Queue-based Rendering System Implementation ===

static DWORD s_currentVertexOffset = 0;
static DWORD s_batchIndex = 0;

LPDIRECT3DVERTEXBUFFER9 ShaderManager::GetSharedVertexBuffer() {
    // For now, return nullptr - TextManager will skip buffer copy
    // The actual buffer copying should be handled by SpriteRenderer
    OutputDebugStringA("[ShaderManager::GetSharedVertexBuffer] Returning nullptr - buffer copy skipped\n");
    return nullptr;
}

void ShaderManager::CopyTextVertices(const void* vertices, size_t vertexCount) {
    char debugBuf[256];
    sprintf(debugBuf, "[ShaderManager::CopyTextVertices] Copying %zu text vertices\n", vertexCount);
    OutputDebugStringA(debugBuf);
    
    // Copy to the end of current vertex buffer
    if (s_currentVertexOffset + vertexCount > 4096 * 4) {
        OutputDebugStringA("[ShaderManager::CopyTextVertices] ERROR: Vertex buffer overflow!\n");
        return;
    }
    
    // This will be copied during ExecuteQueue Lock phase
    // For now, just store the vertices for later copying
}

void ShaderManager::Submit(const RenderCommand& cmd) {
    RenderCommand cmdWithOffset = cmd;
    
    // XBOX 360 FIX: Reset vertex offset for background (shaderID 0) to ensure vertexStart=0
    if (cmd.shaderID == 0) {
        s_currentVertexOffset = 0;
        s_batchIndex = 0;
    }
    
    // Use vertexStart as-is - SpriteRenderer already calculates correct offset
    cmdWithOffset.vertexStart = cmd.vertexStart;
    cmdWithOffset.batchIndex = s_batchIndex++;
    
    // Update vertex offset for next batch
    s_currentVertexOffset += cmd.vertexCount;
    
    m_commandQueue.push_back(cmdWithOffset);
}

// Add command to render queue (alias for Submit)
void ShaderManager::PushCommand(const RenderCommand& cmd) {
    Submit(cmd);
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
    // Sort by depth (back-to-front for alpha)
    // Lower depth = draw FIRST (background)
    // Higher depth = draw LAST (foreground)
    
    // Debug: log depths before sorting
    char dbg[256];
    sprintf(dbg, "[SortQueue] Before: %d commands\n", (int)m_commandQueue.size());
    OutputDebugStringA(dbg);
    for (size_t i = 0; i < m_commandQueue.size() && i < 5; i++) {
        sprintf(dbg, "[SortQueue] cmd[%d]: depth=%.2f, texture=%p\n", 
                (int)i, m_commandQueue[i].depth, m_commandQueue[i].pTexture);
        OutputDebugStringA(dbg);
    }
    
    // Sort by depth ASCENDING (lower depth = background = draw first)
    std::sort(m_commandQueue.begin(), m_commandQueue.end(), 
              [](const RenderCommand& a, const RenderCommand& b) {
                  return a.depth < b.depth; // Lower depth first
              });
    
    // Debug: log depths after sorting
    sprintf(dbg, "[SortQueue] After:\n");
    OutputDebugStringA(dbg);
    for (size_t i = 0; i < m_commandQueue.size() && i < 5; i++) {
        sprintf(dbg, "[SortQueue] cmd[%d]: depth=%.2f, texture=%p\n", 
                (int)i, m_commandQueue[i].depth, m_commandQueue[i].pTexture);
        OutputDebugStringA(dbg);
    }
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
    hr = LoadShader(SHADER_SPRITE, "Media/Shaders/Sprite.fx", "SpriteBatchTech");
    if (FAILED(hr)) {
        OutputDebugStringA("[ShaderManager] FATAL: Failed to load SPRITE shader\n");
        return hr;
    }
    
    // Load SPRITE_CONSTANT_INSTANCED shader
    hr = LoadShader(SHADER_SPRITE_CONSTANT_INSTANCED, "Media/Shaders/SpriteConstantInstanced.fx", "SpriteBatchTech");
    if (FAILED(hr)) {
        OutputDebugStringA("[ShaderManager] FATAL: Failed to load SPRITE_CONSTANT_INSTANCED shader\n");
        return hr;
    }
    
    // Load RADIALMENU shader
    hr = LoadShader(SHADER_RADIALMENU, "Media/Shaders/RadialMenu.fx", "RadialMenuTech");
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
    
    // Use centralized m_effects map
    std::map<ShaderID, ID3DXEffect*>::iterator it = m_effects.find(id);
    if (it != m_effects.end() && it->second) {
        m_pActiveEffect = it->second;
        m_currentShaderID = id;
        
        // Get technique
        D3DXHANDLE hTechnique = m_pActiveEffect->GetTechnique(0);
        if (hTechnique) {
            // Set technique (don't Begin/End here - will be done in rendering loop)
            m_pActiveEffect->SetTechnique(hTechnique);
            m_numPasses = 1; // Assume 1 pass for most shaders
        }
        
        // Set ViewProjection if provided
        if (pViewProj) {
            SetGlobalUniforms(pViewProj);
        }
        
        return;
    }
    
    // Shader not found in centralized map
    char errorMsg[256];
    sprintf_s(errorMsg, "[ShaderManager] Prepare: Shader ID %d not found in m_effects (total effects in map: %d)\n", id, (int)m_effects.size());
    OutputDebugStringA(errorMsg);

    // Log all available shader IDs for debugging
    char debugMsg[512];
    sprintf_s(debugMsg, "[ShaderManager] Available shader IDs in m_effects: ");
    OutputDebugStringA(debugMsg);
    for (std::map<ShaderID, ID3DXEffect*>::iterator it = m_effects.begin(); it != m_effects.end(); ++it) {
        char idMsg[64];
        sprintf_s(idMsg, "%d ", it->first);
        OutputDebugStringA(idMsg);
    }
    OutputDebugStringA("\n");

    m_pActiveEffect = NULL;
    m_currentShaderID = SHADER_INVALID;
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
        case SHADER_WORLD:
        {
            // World shader: set texture + camera ViewProj matrix
            if (cmd.pTexture) {
                SetTexture("g_texture", cmd.pTexture);
            }
            // Use camera ViewProj for world-space rendering
            if (m_hasFrameViewProj) {
                SetMatrix("gViewProj", (const float*)&m_frameViewProj);
            }
            break;
        }
            
        case SHADER_UI:
        {
            // UI shader: set texture + screen orthographic matrix + disable Z-write
            if (cmd.pTexture) {
                SetTexture("g_texture", cmd.pTexture);
            }
            // Disable Z-write for UI to prevent depth conflicts
            m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
            m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            
            // Use orthographic projection for UI (screen space)
            D3DXMATRIX matOrtho;
            D3DXMatrixOrthoOffCenterLH(&matOrtho, 0.0f, 1280.0f, 720.0f, 0.0f, -1.0f, 1.0f);
            SetMatrix("matOrtho", (const float*)&matOrtho);
            break;
        }
            
        case SHADER_SPRITE:
        case SHADER_SPRITE_CONSTANT_INSTANCED:
        case SHADER_TERRAIN:
        {
            // Legacy sprite shaders: set texture + ortho/world matrix
            if (cmd.pTexture) {
                SetTexture("g_texture", cmd.pTexture);
            }
            // For world-space objects, apply camera; for UI, use identity
            if (cmd.isUI) {
                D3DXMATRIX matOrtho;
                D3DXMatrixOrthoOffCenterLH(&matOrtho, 0.0f, 1280.0f, 720.0f, 0.0f, -1.0f, 1.0f);
                SetMatrix("matOrtho", (const float*)&matOrtho);
            } else if (m_hasFrameViewProj) {
                SetMatrix("matOrtho", (const float*)&m_frameViewProj);
            }
            break;
        }
            
        case SHADER_RADIALMENU:
        {
            // RadialMenu shader: set WorldViewProjection
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
        }
            
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
    
    for (std::map<ShaderID, Shader>::iterator it = m_shaders.begin(); it != m_shaders.end(); ++it) {
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

    // Reset vertex offset at start of each frame
    s_currentVertexOffset = 0;
    s_batchIndex = 0;
    
    if (m_commandQueue.empty()) {
        OutputDebugStringA("[ShaderManager::ExecuteQueue] Queue is empty, returning\n");
        return;
    }

    // CRITICAL: Check device and effect before proceeding
    if (!m_pDevice) {
        OutputDebugStringA("[ShaderManager::ExecuteQueue] CRITICAL: m_pDevice is NULL!\n");
        return;
    }

    // NOTE: Removed D3DCLEAR_TARGET - it was wiping out previous renderings (including UI text)
    // If you need to clear for debugging, do it BEFORE rendering, not after

    // Set projection matrix for ALL shaders (not just sprite shader)
    D3DXMATRIX ortho;
    D3DXMatrixOrthoOffCenterLH(&ortho, 0, 1280, 720, 0, 0, 1);

    // === GLOBAL CONSTANT BUFFER: Set ViewProj once per frame ===
    // Use provided pViewProj if available, otherwise use ortho for 2D
    const D3DXMATRIX* matrixToUse = pViewProj ? pViewProj : &ortho;
    SetFrameViewProj(matrixToUse);

    // === SORT COMMANDS: depth-first for correct rendering order ===
    // Sort by depth ASCENDING (lower depth = background = draw first)
    std::sort(m_commandQueue.begin(), m_commandQueue.end(), 
              [](const RenderCommand& a, const RenderCommand& b) {
                  return a.depth < b.depth;
              });

    // === STATE LOCKING: Prevent external state corruption ===
    Lock();

    // XBOX 360 CRITICAL: Ensure all buffers are unlocked before drawing
    
    // Verify vertex buffer is not locked (Xbox 360 driver crash prevention)
    if (pVB) {
        // Try to lock with 0 size to check if buffer is already locked
        void* testPtr = NULL;
        HRESULT testLock = pVB->Lock(0, 0, &testPtr, D3DLOCK_NOSYSLOCK);
        if (testLock == D3DERR_INVALIDCALL) {
            OutputDebugStringA("[ShaderManager::ExecuteQueue] WARNING: Vertex buffer appears to be locked! Attempting to unlock...\n");
            // Note: We can't directly unlock here as we don't have the lock context
            // This is just a warning - the real fix is in SpriteRenderer::Flush()
        } else if (SUCCEEDED(testLock)) {
            pVB->Unlock(); // Unlock our test lock
        }
    }

    // Set vertex declaration and streams once for entire frame
    m_pDevice->SetVertexDeclaration(pDecl);
    m_pDevice->SetStreamSource(0, pVB, 0, sizeof(SpriteVertex));
    m_pDevice->SetIndices(pIB);
    
    // === LAZY BATCHING: Process commands with minimal state switches ===
    ShaderID currentShaderID = SHADER_INVALID;
    ShaderID lastShaderID = SHADER_INVALID; // Track shader switches
    bool passActive = false;  // Track if we're inside a BeginPass/EndPass block
    LPDIRECT3DTEXTURE9 lastTexture = nullptr; // Lazy texture binding
    bool lastAlphaBlend = false; // Lazy alpha blend state



    for (size_t i = 0; i < m_commandQueue.size(); ++i) {
        const RenderCommand& cmd = m_commandQueue[i];

        int cmdShader = cmd.shaderID;
        bool cmdIsUI = cmd.isUI;
        
        // === SHADER SWITCH DETECTION ===
        if (cmdShader != lastShaderID) {
            OutputDebugStringA("[ShaderManager::ExecuteQueue] Shader switch detected!\n");
            
            // End previous shader pass if active
            if (passActive) {
                EndPass();
                passActive = false;
                OutputDebugStringA("[ShaderManager::ExecuteQueue] Previous pass ended\n");
            }
            
            // End previous shader
            if (lastShaderID != SHADER_INVALID) {
                EndCurrent();
                OutputDebugStringA("[ShaderManager::ExecuteQueue] Previous shader ended\n");
            }
            
            // Prepare new shader
            Prepare(static_cast<ShaderID>(cmdShader), cmdIsUI ? NULL : &m_frameViewProj);
            lastShaderID = static_cast<ShaderID>(cmdShader);
            
            // XBOX 360 FIX: Use unified vertex declaration and stride for all shaders
            // All 2D objects use sizeof(SpriteVertex) stride regardless of shader type
            m_pDevice->SetVertexDeclaration(pDecl);
            m_pDevice->SetStreamSource(0, pVB, 0, sizeof(SpriteVertex)); // Unified stride
            m_pDevice->SetIndices(pIB);
        }
        IDirect3DTexture9* cmdTex = cmd.pTexture;
        float cmdDepth = cmd.depth;
		        
        // === LAZY ALPHA BLEND AND Z-TEST STATE FOR UI ===
        // XBOX 360 CRITICAL: Disable Z-test for UI elements (text) to ensure visibility
        bool needsAlphaBlend = cmdIsUI;
        bool needsZDisable = cmdIsUI; // UI elements should not use depth testing
        
        if (needsAlphaBlend != lastAlphaBlend) {
            m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, needsAlphaBlend ? TRUE : FALSE);
            lastAlphaBlend = needsAlphaBlend;
        }
        
        // For UI elements (text), disable depth testing to prevent Z-fighting with background
        static bool lastZState = false;
        if (needsZDisable != lastZState) {
            m_pDevice->SetRenderState(D3DRS_ZENABLE, needsZDisable ? FALSE : TRUE);
            lastZState = needsZDisable;
        }
        // === LAZY TEXTURE BINDING ===
        if (cmdTex != lastTexture) {
            m_pDevice->SetTexture(0, cmdTex);
            lastTexture = cmdTex;
        }
        
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
        ShaderID cmdShaderID = static_cast<ShaderID>(cmdShader);
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
            
            // Begin shader and pass ONLY when switching shaders
            if (m_pActiveEffect) {
                m_pActiveEffect->Begin(&m_numPasses, 0);
            }
            BeginPass(0);
            passActive = true;
        } else if (!passActive) {
            // Shader didn't change, but pass is not active - need to reopen
            if (m_pActiveEffect) {
                m_pActiveEffect->Begin(&m_numPasses, 0);
            }
            BeginPass(0);
            passActive = true;
        }
        
        // Apply render states for this command
        m_stateCache.ResetDirtyStates(m_pDevice, cmd.states);
        
        // === CENTRALIZED PARAMETER DISPATCH ===
        SetShaderParameters(cmd);
        
        // === DRAW EXECUTION ===
        // (Removed BeginPass from here - now above)
        
        // XBOX 360 SAFETY: Validate buffer sizes before drawing
        {   
            // Check for potential buffer overrun
            if (cmd.primitiveCount > 4096 * 2) { // Max 2 tris per sprite * 4096 sprites
                continue; // Skip this draw to prevent crash
            }
            
            if (cmd.vertexStart + cmd.vertexCount > 4096 * 4) { // Max 4 vertices per sprite * 4096 sprites
                continue; // Skip this draw to prevent crash
            }
        }
        
// Draw without closing pass - keep it open for next draw with same shader
        char drawDebug[256];
        sprintf(drawDebug, "[ExecuteQueue] Drawing cmd %d: start=%d, vertCount=%d, primCount=%d\n",
                (int)i, cmd.vertexStart, cmd.vertexCount, cmd.primitiveCount);
        OutputDebugStringA(drawDebug);
        OutputDebugStringA("[ExecuteQueue] About to call DrawIndexedPrimitive NOW...");

        switch (cmd.batchType) {
            case 0: // Standard/Single sprite rendering
                m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
                                               cmd.vertexStart,
                                               0,
                                               cmd.vertexCount,
                                               0,
                                               cmd.primitiveCount);
                OutputDebugStringA("[ExecuteQueue] DrawIndexedPrimitive DONE!");
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
        
        // NOTE: Don't close pass here - keep it open for next command with same shader
        // Pass will be closed when shader changes or at end of queue

        char loopBuf[256];
        sprintf(loopBuf, "[ShaderManager::ExecuteQueue] Completed cmd %d/%d, continuing loop...\n", (int)i+1, (int)m_commandQueue.size());
        OutputDebugStringA(loopBuf);
    }
    
    // Close pass at end of all draws
    if (passActive) {
        EndPass();
        passActive = false;
    }
    
     // === CLEANUP: End current shader only ===
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
    m_pDevice->SetStreamSource(0, pVB, 0, sizeof(SpriteVertex));
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
    m_pDevice->SetStreamSource(0, pVB, 0, sizeof(SpriteVertex));
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
