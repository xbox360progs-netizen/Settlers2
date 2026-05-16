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
    // Guard against invalid state indexes
    const size_t MAX_STATES = sizeof(m_currentStates) / sizeof(m_currentStates[0]);
    if ((size_t)state >= MAX_STATES) {
        return;
    }

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
    // m_commandQueue is now a fixed-size array for lock-free ring buffer
    m_drawBatches.reserve(500);
    m_batches.reserve(500);
    D3DXMatrixIdentity(&m_frameViewProj);
    D3DXMatrixIdentity(&m_cachedView);
    D3DXMatrixIdentity(&m_cachedProj);
    // Initialize synchronization primitive
    InitializeCriticalSection(&m_cs);
    m_sharedVB = NULL;

    // Initialize command queue status to 0 (free) for lock-free ring buffer
    for (int i = 0; i < MAX_GLOBAL_COMMANDS; ++i) {
        m_commandQueue[i].status = 0;
    }
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
     if (FAILED(LoadShader(SHADER_RADIALMENU, "game:\\Media\\Shaders\\RadialMenu.fx", "RadialMenu"))) {
         OutputDebugStringA("[ShaderManager] ERROR: Failed to load SHADER_RADIALMENU (RadialMenu.fx)\n");
         allSuccess = false;
     }

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
    // Release centralized shader effects (each effect is owned once in m_effects)
    for (std::map<ShaderID, ID3DXEffect*>::iterator it = m_effects.begin();
         it != m_effects.end(); ++it) {
        if (it->second) {
            it->second->Release();
            it->second = NULL;
        }
    }
    m_effects.clear();

    // Clear legacy shader map without double-releasing effect pointers
    for (std::map<ShaderID, Shader>::iterator it = m_shaders.begin();
         it != m_shaders.end(); ++it) {
        it->second.pEffect = NULL;
        it->second.hTechnique = NULL;
        it->second.hMatOrtho = NULL;
        it->second.hTexture = NULL;
        it->second.hParams.clear();
    }
    m_shaders.clear();

    m_pActiveShader = NULL;
    m_pActiveEffect = NULL;
    // Release shared VB if created
    if (m_sharedVB) {
        m_sharedVB->Release();
        m_sharedVB = NULL;
    }

    m_pDevice = NULL;
    // Delete critical section
    DeleteCriticalSection(&m_cs);
    m_hasFrameViewProj = false;
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

    // Try cached handle first
    D3DXHANDLE hParam = NULL;
    auto it = m_pActiveShader->hParams.find(paramName);
    if (it != m_pActiveShader->hParams.end()) {
        hParam = it->second;
    } else {
        hParam = m_pActiveShader->pEffect->GetParameterByName(NULL, paramName);
        m_pActiveShader->hParams[paramName] = hParam; // cache even if NULL to avoid repeated lookups
    }

    if (hParam) {
        sprintf(dbgBuf, "[SM::SetTexture] hParam=%p (cached), calling SetTexture\n", hParam);
        OutputDebugStringA(dbgBuf);
        m_pActiveShader->pEffect->SetTexture(hParam, pTexture);
        sprintf(dbgBuf, "[SM::SetTexture] SetTexture returned, pTexture=%p\n", pTexture);
        OutputDebugStringA(dbgBuf);
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
    // Lazily create a shared VB used for text/sprite staging if needed
    if (m_sharedVB) return m_sharedVB;

    if (!m_pDevice) {
        OutputDebugStringA("[ShaderManager::GetSharedVertexBuffer] ERROR: Device not initialized\n");
        return nullptr;
    }

    // Create a reasonably large VB (match SpriteRenderer MAX vertices)
    const size_t MAX_VERTICES = 16384; // same as SpriteRenderer MAX_BUFFER_VERTICES
    const UINT vbSize = (UINT)(MAX_VERTICES * sizeof(SpriteVertex));

    HRESULT hr = m_pDevice->CreateVertexBuffer(vbSize, 0, 0, D3DPOOL_DEFAULT, &m_sharedVB, NULL);
    if (FAILED(hr) || !m_sharedVB) {
        char buf[256];
        sprintf(buf, "[ShaderManager::GetSharedVertexBuffer] Failed to create VB (hr=0x%08X)\n", hr);
        OutputDebugStringA(buf);
        m_sharedVB = NULL;
        return NULL;
    }

    OutputDebugStringA("[ShaderManager::GetSharedVertexBuffer] Created shared VB successfully\n");
    return m_sharedVB;
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
    // Lifecycle check: prevent Submit() after batch is sealed
    // This prevents accidental writes after FinalizeFrameCommands()
    // Note: We can't directly check SpriteRenderer state here, but the caller should ensure
    // Submit() is only called during accumulation phase
    
    RenderCommand cmdWithOffset = cmd;
    
    // XBOX 360 FIX: Reset vertex offset for background (shaderID 0) to ensure vertexStart=0
    // REMOVED: This breaks the lifecycle architecture. Offsets should only reset in BeginFrame()
    // if (cmd.shaderID == 0) {
    //     s_currentVertexOffset = 0;
    //     s_batchIndex = 0;
    // }

    // Use vertexStart and baseVertex as-is - SpriteRenderer already calculates correct offsets
    cmdWithOffset.vertexStart = cmd.vertexStart;
    cmdWithOffset.baseVertex = cmd.baseVertex;
    cmdWithOffset.batchIndex = s_batchIndex++;

    // Update vertex offset for next batch
    s_currentVertexOffset += cmd.vertexCount;

    // For lock-free ring buffer, use atomic operations instead of push_back
    // Find free slot and atomically capture it
    int targetSlot = -1;
    for (int i = 0; i < MAX_GLOBAL_COMMANDS; ++i) {
        if (InterlockedCompareExchange(&m_commandQueue[i].status, 2, 0) == 0) {
            targetSlot = i;
            break;
        }
    }

    if (targetSlot != -1) {
        m_commandQueue[targetSlot] = cmdWithOffset;
        InterlockedExchange(&m_commandQueue[targetSlot].status, 1);
        
        char debugBuf[256];
        sprintf(debugBuf, "[ShaderManager::Submit] Submitting command %d: baseVert=%d, verts=%d, texture=%p\n",
                targetSlot, cmd.baseVertex, cmd.vertexCount, cmd.pTexture);
        OutputDebugStringA(debugBuf);
    } else {
        OutputDebugStringA("[ShaderManager::Submit] CRITICAL: Command Ring Buffer Overflow!\n");
    }
}

// Add command to render queue (alias for Submit)
void ShaderManager::PushCommand(const RenderCommand& cmd) {
    Submit(cmd);
}

// Push command for Xbox 360 ring buffer architecture
void ShaderManager::PushXbox360Command(const RenderCommand& newCmd) {
    int targetSlot = -1;
    int attempts = 0;
    const int MAX_ATTEMPTS = 1000; // Protection against infinite loop

    // Ищем свободную ячейку в циклическом кольцевом буфере команд кадра
    while (attempts < MAX_ATTEMPTS) {
        for (int i = 0; i < MAX_GLOBAL_COMMANDS; ++i) {
            // Статус 0 означает, что ячейка свободна.
            // Атомарно переводим её в статус 2 (блокировка под запись Потоком Логики)
            if (InterlockedCompareExchange(&m_commandQueue[i].status, 2, 0) == 0) {
                targetSlot = i;
                break;
            }
        }
        
        if (targetSlot != -1) break;
        
        // Если нет свободных слотов, ждем немного
        attempts++;
        Sleep(1);
    }

    if (targetSlot != -1) {
        // Копируем параметры команды из структуры RenderTypes.h
        RenderCommand& queuedCmd = m_commandQueue[targetSlot];
        queuedCmd.shaderID = newCmd.shaderID;
        queuedCmd.pTexture = newCmd.pTexture;
        queuedCmd.depth = newCmd.depth;
        queuedCmd.vertexCount = newCmd.vertexCount;
        queuedCmd.primitiveCount = newCmd.primitiveCount;
        queuedCmd.baseVertex = newCmd.baseVertex;
        queuedCmd.vertexStart = newCmd.vertexStart;
        queuedCmd.isUI = newCmd.isUI;

        // Атомарно выставляем статус 1: "Пачка полностью готова к рендеру"
        InterlockedExchange(&queuedCmd.status, 1);
        OutputDebugStringA("[ShaderManager] Command submitted successfully\n");
    } else {
        OutputDebugStringA("[ShaderManager] CRITICAL: Command queue overflow after retries!\n");
    }
}

void ShaderManager::SubmitDrawBatch(const DrawBatch& batch) {
    Lock();
    m_drawBatches.push_back(batch);
    Unlock();
}

void ShaderManager::SubmitBatch(const RenderBatch& batch) {
    Lock();
    m_batches.push_back(batch);
    Unlock();
}

void ShaderManager::ClearQueue() {
    // Lock();
    for (int i = 0; i < MAX_GLOBAL_COMMANDS; ++i) {
        m_commandQueue[i].status = 0;
    }
    // Unlock();
}

void ShaderManager::ClearDrawBatches() {
    Lock();
    m_drawBatches.clear();
    Unlock();
}

void ShaderManager::ClearBatches() {
    Lock();
    m_batches.clear();
    Unlock();
}

void ShaderManager::SortQueue() {
    // For lock-free ring buffer, sorting is not applicable
    // Commands are submitted with atomic operations and ExecuteQueue scans by status
    // Sorting is done by the submitter (SpriteRenderer) if needed
    OutputDebugStringA("[SortQueue] WARNING: Sorting not applicable for lock-free ring buffer\n");

#ifdef _XBOX
    __sync();
#endif
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
        
        // Set ViewProjection if provided (CRITICAL: must set after activating effect)
        if (pViewProj) {
            char buf[256];
            sprintf_s(buf, "[ShaderManager::Prepare] Calling SetGlobalUniforms for new shader ID %d\n", id);
            OutputDebugStringA(buf);
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
    if (!m_pActiveShader || !pViewProj) {
        OutputDebugStringA("[SM::SetGlobalUniforms] ERROR: m_pActiveShader or pViewProj is NULL\n");
        return;
    }
    OutputDebugStringA("[SM::SetGlobalUniforms] Setting WVP and WorldViewProjection matrices\n");

    // Debug: Log matrix values
    char matBuf[512];
    const float* m = (const float*)pViewProj;
    sprintf(matBuf, "[SM::SetGlobalUniforms] Matrix: [%f %f %f %f; %f %f %f %f; %f %f %f %f; %f %f %f %f]\n",
            m[0], m[1], m[2], m[3],
            m[4], m[5], m[6], m[7],
            m[8], m[9], m[10], m[11],
            m[12], m[13], m[14], m[15]);
    OutputDebugStringA(matBuf);

    // Set both parameter names for compatibility with different shaders
    SetMatrix("WVP", (const float*)pViewProj);
    SetMatrix("WorldViewProjection", (const float*)pViewProj);
    Commit();
    OutputDebugStringA("[SM::SetGlobalUniforms] Matrices set and committed\n");
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
    EnterCriticalSection(&m_cs);
    m_isLocked = true;
}

void ShaderManager::Unlock() {
    m_isLocked = false;
    LeaveCriticalSection(&m_cs);
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
            // CRITICAL FIX: Use near=0.0f, far=1.0f to avoid Z offset that causes clipping
            D3DXMatrixOrthoOffCenterLH(&matOrtho, 0.0f, 1280.0f, 720.0f, 0.0f, 0.0f, 1.0f);
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
                // CRITICAL FIX: Use near=0.0f, far=1.0f to avoid Z offset that causes clipping
                D3DXMatrixOrthoOffCenterLH(&matOrtho, 0.0f, 1280.0f, 720.0f, 0.0f, 0.0f, 1.0f);
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
    // Valid IDs are (SHADER_INVALID + 1) .. (SHADER_COUNT - 1)
    return id > SHADER_INVALID && id < SHADER_COUNT;
}

// Legacy ApplyShader (for backward compatibility with int-based API)
void ShaderManager::ApplyShader(int shaderID) {
    ShaderID id = static_cast<ShaderID>(shaderID);
    Prepare(id, NULL);
}

void ShaderManager::ExecuteQueue(LPDIRECT3DVERTEXBUFFER9 pVB, LPDIRECT3DINDEXBUFFER9 pIB,
                                LPDIRECT3DVERTEXDECLARATION9 pDecl, DWORD vertexStride,
                                const D3DXMATRIX* pViewProj, SpriteRenderer* pSpriteRenderer) {

    OutputDebugStringA("[SMgr::ExecuteQueue] ENTRY\n");

    // Reset vertex offset at start of each frame
    // REMOVED: This should be done in BeginFrame(), not here
    // s_currentVertexOffset = 0;
    // s_batchIndex = 0;

    // CRITICAL: Check device before proceeding
    if (!m_pDevice) {
        OutputDebugStringA("[SMgr::ExecuteQueue] ERROR: m_pDevice is NULL!\n");
        return;
    }
    OutputDebugStringA("[SMgr::ExecuteQueue] m_pDevice OK\n");

    // Guard against null buffers
    if (!pVB || !pIB || !pDecl) {
        OutputDebugStringA("[SMgr::ExecuteQueue] ERROR: NULL buffers!\n");
        return;
    }
    OutputDebugStringA("[SMgr::ExecuteQueue] Buffers valid\n");

    // Set projection matrix
    D3DXMATRIX ortho;
    D3DXMatrixOrthoOffCenterLH(&ortho, 0, 1280, 720, 0, 0, 1);

    D3DXMATRIX localOrtho;
    // CRITICAL FIX: Use near=0.0f, far=1.0f to avoid Z offset of 0.5 that causes clipping
    D3DXMatrixOrthoOffCenterLH(&localOrtho, 0.0f, 1280.0f, 720.0f, 0.0f, 0.0f, 1.0f);

    OutputDebugStringA("[SMgr::ExecuteQueue] Setting state...\n");
    // Lock();

    OutputDebugStringA("[SMgr::ExecuteQueue] Setting stream source...\n");
    m_pDevice->SetVertexDeclaration(pDecl);
    m_pDevice->SetStreamSource(0, pVB, 0, sizeof(SpriteVertex));
    m_pDevice->SetIndices(pIB);

    OutputDebugStringA("[SMgr::ExecuteQueue] Disabling Z buffer and culling...\n");
    m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE); // Disable backface culling

    const D3DXMATRIX* matrixToUse = pViewProj ? pViewProj : &ortho;
    SetFrameViewProj(matrixToUse);

    // Set vertex declaration and stream source once for entire frame
    OutputDebugStringA("[SMgr::ExecuteQueue] Setting vertex data...\n");
    m_pDevice->SetVertexDeclaration(pDecl);
    m_pDevice->SetStreamSource(0, pVB, 0, sizeof(SpriteVertex));
    m_pDevice->SetIndices(pIB);

    // Disable Z-buffer for 2D UI rendering
    OutputDebugStringA("[SMgr::ExecuteQueue] Disabling Z...\n");
    m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

    OutputDebugStringA("[SMgr::ExecuteQueue] Unlocking...\n");
    Unlock();

    ShaderID currentShaderID = SHADER_INVALID;
    LPDIRECT3DTEXTURE9 lastTexture = nullptr;
    bool passActive = false;
    bool hasAnyWorkBeenDone = false;

    int commandCount = 0;
    for (int i = 0; i < MAX_GLOBAL_COMMANDS; ++i) {
        RenderCommand& cmd = m_commandQueue[i];
        if (cmd.status == 1) {
            commandCount++;
            char dbg[256];
            sprintf(dbg, "[ExecuteQueue] Command %d: baseVert=%d, verts=%d, texture=%p\n",
                    i, cmd.baseVertex, cmd.vertexCount, cmd.pTexture);
            OutputDebugStringA(dbg);
        }
    }

    char cmdBuf[256];
    sprintf(cmdBuf, "[SMgr::ExecuteQueue] Found %d commands to execute\n", commandCount);
    OutputDebugStringA(cmdBuf);

    for (int i = 0; i < MAX_GLOBAL_COMMANDS; ++i) {
        RenderCommand& cmd = m_commandQueue[i];

        if (cmd.status == 1) {

            InterlockedExchange(&cmd.status, 2);

            if (!cmd.pTexture) {
                OutputDebugStringA("[SMgr::ExecuteQueue] WARNING: cmd.pTexture is NULL!\n");
                InterlockedExchange(&cmd.status, 0);
                continue;
            }

            hasAnyWorkBeenDone = true;

            char dbg[256];
            sprintf(dbg, "[SMgr::ExecuteQueue] cmd: shaderID=%d, texture=%p\n", cmd.shaderID, cmd.pTexture);
            OutputDebugStringA(dbg);

            if (cmd.shaderID != currentShaderID) {
                OutputDebugStringA("[SMgr::ExecuteQueue] Switching shader...\n");
                if (passActive) {
                    EndPass();
                    EndCurrent();
                }

                const D3DXMATRIX* activeMatrix = cmd.isUI ? &localOrtho : &m_frameViewProj;

                Prepare(static_cast<ShaderID>(cmd.shaderID), activeMatrix);
                OutputDebugStringA("[SMgr::ExecuteQueue] Prepare done\n");
                currentShaderID = static_cast<ShaderID>(cmd.shaderID);

                if (m_pActiveEffect) {
                    m_pActiveEffect->Begin(&m_numPasses, 0);
                    BeginPass(0);
                    passActive = true;
                    OutputDebugStringA("[SMgr::ExecuteQueue] Shader activated\n");
                    // CRITICAL: Set matrix AFTER BeginPass for effect framework
                    SetGlobalUniforms(activeMatrix);
                } else {
                    OutputDebugStringA("[SMgr::ExecuteQueue] WARNING: m_pActiveEffect is NULL!\n");
                }
            }

            if (cmd.pTexture != lastTexture) {
                m_pDevice->SetTexture(0, cmd.pTexture);
                // Also set texture as shader parameter for the shader to use
                if (m_pActiveEffect) {
                    SetTexture("g_texture", cmd.pTexture);
                    m_pActiveEffect->CommitChanges();
                }
                lastTexture = cmd.pTexture;
            }

            if (passActive) {
                if (m_pActiveEffect) {
                    m_pActiveEffect->CommitChanges();
                }

                char renderMsg[512];
                sprintf(renderMsg, "[SMgr::ExecuteQueue] Draw: depth=%.2f, baseVert=%d, startIdx=%d, verts=%d, prims=%d\n",
                        cmd.depth, cmd.baseVertex, cmd.vertexStart, cmd.vertexCount, cmd.primitiveCount);
                OutputDebugStringA(renderMsg);

                OutputDebugStringA("[SMgr::ExecuteQueue] Calling DrawIndexedPrimitive...\n");
                m_pDevice->DrawIndexedPrimitive(
                    D3DPT_TRIANGLELIST,
                    cmd.baseVertex,    // CORRECT vertex offset from ring buffer
                    0,                 // MinIndex = 0
                    cmd.vertexCount,
                    cmd.vertexStart,   // CORRECT index buffer offset
                    cmd.primitiveCount
                );
                OutputDebugStringA("[SMgr::ExecuteQueue] DrawIndexedPrimitive DONE\n");

                InterlockedExchange(&cmd.status, 0);
                OutputDebugStringA("[SMgr::ExecuteQueue] status reset\n");
            }
        }
    }

    OutputDebugStringA("[SMgr::ExecuteQueue] Loop done - about to close shader pass...\n");
    if (passActive) {
        OutputDebugStringA("[SMgr::ExecuteQueue] Calling EndPass()...\n");
        EndPass();
        OutputDebugStringA("[SMgr::ExecuteQueue] EndPass done, calling EndCurrent()...\n");
        EndCurrent();
        OutputDebugStringA("[SMgr::ExecuteQueue] EndCurrent done\n");
    }

    // Disable culling to ensure triangles are rendered
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    OutputDebugStringA("[SMgr::ExecuteQueue] About to Unlock...\n");
    // Unlock();
    OutputDebugStringA("[SMgr::ExecuteQueue] Unlocked\n");

    OutputDebugStringA("[SMgr::ExecuteQueue] Clearing textures...\n");
    m_pDevice->SetTexture(0, NULL);
    m_pDevice->SetTexture(1, NULL);
    m_pDevice->SetTexture(2, NULL);
    m_pDevice->SetTexture(3, NULL);
    OutputDebugStringA("[SMgr::ExecuteQueue] Textures cleared\n");

    // REMOVED: This should be done in ResetBatchState(), not here
    // s_currentVertexOffset = 0;
    // s_batchIndex = 0;

    OutputDebugStringA("[SMgr::ExecuteQueue] About to return...\n");
}

// Reset static offsets (called by SpriteRenderer::BeginFrame)
void ShaderManager::ResetOffsets() {
    s_currentVertexOffset = 0;
    s_batchIndex = 0;
    OutputDebugStringA("[ShaderManager::ResetOffsets] Static offsets reset\n");
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
