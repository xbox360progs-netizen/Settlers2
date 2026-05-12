#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <map>
#include <vector>
#include <string>

// Global shader ID enum (must be outside class for use across files)
enum ShaderID {
    SHADER_INVALID = -1,
    SHADER_SPRITE = 0,
    SHADER_SPRITE_CONSTANT_INSTANCED = 1,
    SHADER_RADIALMENU = 2,
    SHADER_UI = 3,
    SHADER_TERRAIN = 4,
    SHADER_WORLD = 5,
    SHADER_ENTITY = 6,
    SHADER_FONT = 7,
    SHADER_COUNT
};

// Xbox 360 specific: Ensure we use the right alignment for VMX-related math if needed
__declspec(align(16))
class ShaderManager {
public:
    struct Shader {
        LPD3DXEFFECT pEffect;
        D3DXHANDLE hTechnique;
        D3DXHANDLE hMatOrtho;
        D3DXHANDLE hTexture;
        // Use a fixed-size cache or a thread-safe map if parameters are set
        // across threads. For Xbox 360, we prefer pre-caching everything.
        std::map<std::string, D3DXHANDLE> hParams;
    };

    // Render state block for batch rendering
    struct RenderStateBlock {
        DWORD zEnable;
        DWORD alphaBlendEnable;
        DWORD srcBlend;
        DWORD destBlend;
        DWORD cullMode;
        
        RenderStateBlock() 
            : zEnable(D3DZB_FALSE), alphaBlendEnable(FALSE), 
              srcBlend(D3DBLEND_SRCALPHA), destBlend(D3DBLEND_INVSRCALPHA),
              cullMode(D3DCULL_NONE) {}
    };

    // Draw batch structure for material-based sorting (State Sorting)
    struct DrawBatch {
        IDirect3DTexture9* pTexture;
        Shader* pShader;
        DWORD vertexOffset;  // Offset in vertex buffer
        DWORD indexCount;    // Number of indices to draw
        float depth;       // Z-layer: 1.0=far, 0.1=near
        std::string shaderName;
        int renderType;      // 0 = Single Sprite, 1 = Instanced
        
        // Sorting operator: by texture first (expensive switch), then shader, then depth
        bool operator<(const DrawBatch& other) const {
            // Texture switch is most expensive on Xbox 360
            if (pTexture != other.pTexture)
                return pTexture < other.pTexture;
            // Shader switch is second most expensive
            if (shaderName != other.shaderName)
                return shaderName < other.shaderName;
            // Depth is least expensive (back-to-front for alpha)
            return depth > other.depth;
        }
    };

    // Custom draw callback type for non-standard rendering (e.g. RadialMenu shader)
    typedef void (*CustomDrawFn)(LPDIRECT3DDEVICE9 pDevice, ShaderManager* pShaderMgr, void* pUserData);

    // Render command structure for queue-based rendering (Master Loop)
    struct RenderCommand {
        IDirect3DTexture9* pTexture;
        int shaderID;   // Shader handle (ShaderHandle enum) instead of raw pointer
        DWORD vertexStart;
        DWORD vertexCount;
        DWORD primitiveCount;
        int batchType; // 0 - Standard (Single), 1 - Instanced, 2 - Custom callback
        float depth;   // Z-layer: 1.0=far (ground), 0.5=mid (units), 0.1=near (UI)
        int layer;     // Logical layer: 0=Terrain, 1=Objects, 2=UI (for composite depth)
        bool isUI;     // Screen-space rendering (skip camera matrix)
        RenderStateBlock states;
        DWORD batchIndex; // For tracking batch sequence (for vertex offset calculation)
        
        // World position for camera transform (if not isUI)
        float worldX, worldY;
        
        // UV coordinates for partial texture rendering (text, sprites)
        float u0, v0, u1, v1; // Texture region to sample
        
        // Screen position for UI rendering (if isUI)
        float screenX, screenY, screenW, screenH;
        
        // Color tint for the render command
        D3DCOLOR color;
        
        // Custom draw callback (for batchType == 2)
        CustomDrawFn customDraw;
        void* customUserData;
        
        RenderCommand() : pTexture(NULL), shaderID(SHADER_INVALID), vertexStart(0), vertexCount(0),
            primitiveCount(0), batchType(0), depth(1.0f), layer(0), isUI(false), batchIndex(0),
            worldX(0), worldY(0), u0(0), v0(0), u1(1), v1(1),
            screenX(0), screenY(0), screenW(0), screenH(0), color(0xFFFFFFFF),
            customDraw(NULL), customUserData(NULL) {}
        
        // Sorting operator: shader-first for lazy batching (Xbox 360 optimization)
        // Shader switch is most expensive, then texture, then depth (back-to-front)
        bool operator<(const RenderCommand& other) const {
            if (shaderID != other.shaderID)
                return shaderID < other.shaderID; // Batch by shader (most expensive switch)
            if (pTexture != other.pTexture)
                return pTexture < other.pTexture; // Then by texture
            return depth > other.depth; // Then back-to-front for alpha
        }
    };

    // Render batch structure (legacy, will be replaced by RenderCommand)
    struct RenderBatch {
        IDirect3DTexture9* pTexture;
        Shader* pShader;
        DWORD startVertex;
        DWORD primitiveCount;
        RenderStateBlock states;
        std::string shaderName; // For shader lookup
        
        // Sorting operator: by shader, then texture (critical for Xbox 360)
        bool operator<(const RenderBatch& other) const {
            if (shaderName != other.shaderName)
                return shaderName < other.shaderName;
            return pTexture < other.pTexture;
        }
    };

    // State cache for centralized render state management
    class StateCache {
    public:
        StateCache();
        void SetRenderState(LPDIRECT3DDEVICE9 device, D3DRENDERSTATETYPE state, DWORD value);
        void ResetDirtyStates(LPDIRECT3DDEVICE9 device, const RenderStateBlock& targetStates);
        void MarkDirty() { m_dirty = true; }
        bool IsDirty() const { return m_dirty; }
        void ClearDirty() { m_dirty = false; }
        
    private:
        DWORD m_currentStates[256];
        bool m_dirty;
    };

    ShaderManager();
    ~ShaderManager();

    HRESULT Initialize(LPDIRECT3DDEVICE9 device);
    void Shutdown();

    HRESULT LoadShader(ShaderID id, const char* filepath, const char* techniqueName = "SpriteBatchTech");
    bool SetActiveShader(ShaderID id);

    Shader* GetActiveShader() { return m_pActiveShader; }
    Shader* GetShader(ShaderID id);

    // Rendering Lifecycle
    void BeginShader();
    void BeginPass(UINT pass = 0);
    void EndPass();
    void EndShader();

    UINT GetNumPasses() const { return m_numPasses; }

    // Xbox 360 Optimization: Pre-commit constants
    // This allows the SpriteRenderer to fill buffers on Thread A
    // while Thread B (Render Thread) prepares the GPU state.
    void Commit();

    void SetMatrix(const char* paramName, const float* matrix);
    void SetVector(const char* paramName, const float* vector);
    void SetTexture(const char* paramName, LPDIRECT3DBASETEXTURE9 pTexture);

    // Xbox 360: These are mostly stubs as D3DPOOL_MANAGED doesn't exist,
    // but kept for API compatibility.
    void OnLostDevice();
    void OnResetDevice();

    bool HasShader(ShaderID id) const;
    
    // === Queue-based Rendering System (Master Loop) ===
    
    // Submit a render command to the queue
    void Submit(const RenderCommand& cmd);
    
    // Submit a draw batch for material-based sorting
    void SubmitDrawBatch(const DrawBatch& batch);
    
    // Submit a render batch to the queue (legacy)
    void SubmitBatch(const RenderBatch& batch);
    
    // Clear all commands from the queue
    void ClearQueue();
    
    // Clear draw batches
    void ClearDrawBatches();
    
    // Clear all batches from the queue (legacy)
    void ClearBatches();
    
    // Sort commands by zOrder, shader, and texture (critical for Xbox 360 performance)
    void SortQueue();
    
    // Sort draw batches by material (texture first, then shader) - State Sorting
    void SortDrawBatches();
    
    // Sort batches by shader and texture (legacy)
    void SortBatches();
    
    // === CENTRALIZED SHADER MANAGEMENT (Xbox 360 Safe) ===
    // Load all shaders at startup (FatalError if any fail)
    HRESULT LoadAll();
    
    // Prepare shader for rendering (activate effect with global uniforms)
    void Prepare(ShaderID id, const D3DXMATRIX* pViewProj = NULL);
    
    // End current shader (deactivate effect)
    void EndCurrent();
    
    // Set global uniforms (frame-wide data: view/projection matrix, time, etc.)
    void SetGlobalUniforms(const D3DXMATRIX* pViewProj);
    
    // Set local uniforms (per-entity data: texture, depth, etc.)
    void SetLocalUniforms(LPDIRECT3DTEXTURE9 pTexture, float depth);
    
    // Update constants (unified method for texture + world matrix)
    void UpdateConstants(LPDIRECT3DTEXTURE9 pTexture, const D3DXMATRIX* pWorldMatrix = NULL);
    
    // Set shader parameters from RenderCommand (centralized parameter dispatch)
    // Manager decides which .fx parameter gets which data based on shaderID
    void SetShaderParameters(const RenderCommand& cmd);
    
    // Set frame-wide ViewProjection matrix (Global Constant Buffer)
    // Call once per frame, not per sprite - saves tons of GPU bandwidth
    void SetFrameViewProj(const D3DXMATRIX* pViewProj);
    
    // Update global camera matrices (view + projection) for all loaded shaders
    // Caches internally and propagates to all active effects
    void UpdateGlobalMatrices(const D3DXMATRIX* pView, const D3DXMATRIX* pProj);
    
    // Centralized initialization: load all required shaders at startup
    // Returns false if any shader fails to load
    bool Init();
    
    // Load base shaders (Sprite.fx, UI.fx) for centralized shader registry
    bool LoadBaseShaders();
    
    // Get effect pointer by ShaderID (for direct parameter access when needed)
    ID3DXEffect* GetEffect(ShaderID id);
    
    // Get current frame ViewProjection (for external queries)
    const D3DXMATRIX& GetFrameViewProj() const { return m_frameViewProj; }

    // Get current active shader ID
    ShaderID GetCurrentShaderID() const { return m_currentShaderID; }
    
    // State locking (prevents external state corruption during ExecuteQueue)
    void Lock();
    void Unlock();
    bool IsLocked() const { return m_isLocked; }
    
    // Commit changes (Xbox 360: critical before Draw)
    void CommitChanges();
    
    // Validate shader handle
    bool ValidateShader(ShaderID id) const;
    
    // Legacy ApplyShader (for backward compatibility)
    void ApplyShader(int shaderID);
    void ExecuteQueue(LPDIRECT3DVERTEXBUFFER9 pVB, LPDIRECT3DINDEXBUFFER9 pIB, 
                     LPDIRECT3DVERTEXDECLARATION9 pDecl, DWORD vertexStride,
                     const D3DXMATRIX* pViewProj = NULL); // Optional camera matrix
    
    // Execute draw batches with material-based sorting
    void ExecuteDrawBatches(LPDIRECT3DVERTEXBUFFER9 pVB, LPDIRECT3DINDEXBUFFER9 pIB, 
                           LPDIRECT3DVERTEXDECLARATION9 pDecl, DWORD vertexStride);
    
    // Copy text vertices to shared vertex buffer
    void CopyTextVertices(const void* vertices, size_t vertexCount);
    
    // Execute all batches in the queue (legacy)
    void ExecuteBatches(LPDIRECT3DVERTEXBUFFER9 pVB, LPDIRECT3DINDEXBUFFER9 pIB, 
                       LPDIRECT3DVERTEXDECLARATION9 pDecl, DWORD vertexStride);
    
    // Get command count
    size_t GetCommandCount() const { return m_commandQueue.size(); }
    
    // Get draw batch count
    size_t GetDrawBatchCount() const { return m_drawBatches.size(); }
    
    // Get batch count (legacy)
    size_t GetBatchCount() const { return m_batches.size(); }

    // State management
    StateCache* GetStateCache() { return &m_stateCache; }

private:
    LPDIRECT3DDEVICE9 m_pDevice;
    
    // Centralized shader storage (private for ReadOnly protection)
    std::map<ShaderID, Shader> m_shaders; // Key is ShaderID for fast O(1) lookup
    Shader* m_pActiveShader;
    ID3DXEffect* m_pActiveEffect; // Active effect from centralized m_effects map
    ShaderID m_currentShaderID; // Track current shader for state caching
    UINT m_numPasses;
    
    // State locking (prevents external state corruption during ExecuteQueue)
    bool m_isLocked;
    
    // Global Constant Buffer (set once per frame, not per sprite)
    D3DXMATRIX m_frameViewProj;
    bool m_hasFrameViewProj;
    
    // Cached camera matrices (view + projection separately)
    D3DXMATRIX m_cachedView;
    D3DXMATRIX m_cachedProj;

    // Centralized shader effect storage (mapped by ShaderID)
    std::map<ShaderID, ID3DXEffect*> m_effects;

    // Private shader loading helper
    HRESULT LoadInternal(ShaderID id, const char* path, const char* technique);

    // Render command queue for Master Loop rendering
    std::vector<RenderCommand> m_commandQueue;
    
    // Draw batch queue for material-based sorting (State Sorting)
    std::vector<DrawBatch> m_drawBatches;
    
    // Render queue for batch-based rendering (legacy)
    std::vector<RenderBatch> m_batches;
    
    // State cache for centralized state management
    StateCache m_stateCache;

    // Critical Section for Thread Safety if multiple cores access the Manager
    // CRITICAL_SECTION m_cs;
};
