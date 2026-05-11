#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <map>
#include <string>
#include <vector>

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

    // Render batch structure for queue-based rendering
    struct RenderBatch {
        IDirect3DTexture9* pTexture;
        Shader* pShader;
        unsigned int startVertex;
        unsigned int primitiveCount;
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

    HRESULT LoadShader(const char* name, const char* filepath, const char* techniqueName = "SpriteBatchTech");
    bool SetActiveShader(const char* name);

    Shader* GetActiveShader() { return m_pActiveShader; }
    Shader* GetShader(const char* name);

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

    bool HasShader(const char* name) const;

    // === Queue-based Rendering System ===
    
    // Submit a render batch to the queue
    void SubmitBatch(const RenderBatch& batch);
    
    // Clear all batches from the queue
    void ClearBatches();
    
    // Sort batches by shader and texture (critical for Xbox 360 performance)
    void SortBatches();
    
    // Execute all batches in the queue
    void ExecuteBatches(LPDIRECT3DVERTEXBUFFER9 pVB, LPDIRECT3DINDEXBUFFER9 pIB, 
                       LPDIRECT3DVERTEXDECLARATION9 pDecl, DWORD vertexStride);
    
    // Get batch count
    size_t GetBatchCount() const { return m_batches.size(); }

    // State management
    StateCache* GetStateCache() { return &m_stateCache; }

private:
    LPDIRECT3DDEVICE9 m_pDevice;
    std::map<std::string, Shader> m_shaders;
    Shader* m_pActiveShader;
    UINT m_numPasses;

    // Render queue for batch-based rendering
    std::vector<RenderBatch> m_batches;
    
    // State cache for centralized state management
    StateCache m_stateCache;

    // Critical Section for Thread Safety if multiple cores access the Manager
    // CRITICAL_SECTION m_cs;
};
