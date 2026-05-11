#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include <string>
#include "Renderer.h"
#include "ShaderManager.h"
#include "SpriteAtlas.h"
#include <xtl.h>

// Forward declaration for ThreadData struct
class SpriteRenderer;

// SpriteData structure for thread-safe data passing
struct SpriteData {
    float x, y;
    float width, height;
    float angle;
    float u0, v0, u1, v1;
    DWORD color;
};

// Thread data structure for worker threads
struct ThreadData {
    int startIdx;
    int count;
    const SpriteData* source;
    SpriteRenderer* pRenderer;
};

// Render command structure for batch sorting (by shaderId)
struct RenderCommand {
    SpriteAtlas* pAtlas;
    int shaderId;            // ID of the shader to use
    float x, y, w, h;
    float u0, v0, u1, v1;
    DWORD color;
    float depth; // For Z-order sorting

    // Sorting operator: by shaderId, then atlas, then depth
    bool operator<(const RenderCommand& other) const {
        if (shaderId != other.shaderId)
            return shaderId < other.shaderId;
        if (pAtlas != other.pAtlas)
            return pAtlas < other.pAtlas;
        return depth < other.depth;
    }
};

// Instance data structure for hardware instancing on Xbox 360
struct SpriteInstance {
    float positionSize[4]; // x, y, width, height
    float uvRect[4];       // u0, v0, u1, v1
    DWORD color;           // Tint color
    float padding[3];      // Align to 48 bytes for VMX alignment
};

// Rendering strategy modes
enum RenderMode {
    MODE_STANDARD,           // FlushStandard (CPU prepares all vertices, 1 stream)
    MODE_INSTANCED_STREAM,   // FlushInstanced (Two streams: geometry + VB with data)
    MODE_INSTANCED_CONST     // FlushConstantInstanced (Geometry + Constant Registers)
};

class SpriteRenderer {
public:
    SpriteRenderer();
    ~SpriteRenderer();

    // Initialize with device, shader manager, and max sprites per batch
    HRESULT Initialize(LPDIRECT3DDEVICE9 device, ShaderManager* shaderManager, int maxSprites = 4096);
    void Shutdown();

    // Xbox 360 device loss handling
    void OnLostDevice();
    void OnResetDevice();

    // Begin a batch with specific shader and texture
    void Begin(const char* shaderName, LPDIRECT3DTEXTURE9 pTexture);
    void Begin(const char* shaderName, LPDIRECT3DTEXTURE9 pTexture, float zOrder);

    // Submit current batch to ShaderManager queue (for manual control)
    void SubmitBatch(ShaderManager* pShader);

    // Draw a sprite at position with UV rect and color
    void Draw(float x, float y, float width, float height,
              float u0, float v0, float u1, float v1,
              DWORD color = 0xFFFFFFFF);

    // Draw sprite with automatic texture flush on texture change
    void DrawWithTexture(float x, float y, float width, float height,
                        float u0, float v0, float u1, float v1,
                        LPDIRECT3DTEXTURE9 pTexture,
                        DWORD color = 0xFFFFFFFF);

    // Draw a rotated sprite (VMX-optimized on Xbox 360)
    void DrawRotated(float x, float y, float width, float height, float angle,
                     float u0, float v0, float u1, float v1,
                     DWORD color = 0xFFFFFFFF);

    // End batch and flush remaining sprites
    void End();

    // Flush with explicit ShaderManager pointer for multi-threaded rendering
    void Flush(ShaderManager* pShader);

    // Manual flush (uses internal shader manager)
    void Flush();
    // Configure streams for hardware instancing (Stream 0: geometry, Stream 1: per-instance data)
    void SetupInstancingStates(int spriteCount);

	 // Public wrapper (for compatibility when private access is an issue)
    void DrawAtlasSpritePublic(SpriteAtlas* atlas, DWORD index, float x, float y, DWORD color = 0xFFFFFFFF);

    // Get current sprite count
    int GetSpriteCount() const { return m_spriteCount; }

    // Get current vertex count
    int GetVertexCount() const { return m_spriteCount * 4; }

    // Get device for external state management
    LPDIRECT3DDEVICE9 GetDevice() const { return m_pDevice; }

    // Get rendering resources for queue-based execution
    LPDIRECT3DVERTEXBUFFER9 GetVertexBuffer() const { return m_pVB[m_activeBuffer]; }
    LPDIRECT3DINDEXBUFFER9 GetIndexBuffer() const { return m_pIndexBuffer; }
    LPDIRECT3DVERTEXDECLARATION9 GetVertexDeclaration() const { return m_pVertexDecl; }

    // Public wrapper for staging area (external callers)
    void FillStagingAreaPublic(int startIdx, int count, const SpriteData* data);

    // Shader registry helpers
private:
    int GetShaderId(const char* name);
    std::vector<std::string> m_shaderNameRegistry;

    // Multi-threaded: Fill staging buffer from worker threads
    void FillStagingArea(int startIdx, int count, const SpriteData* data);

    // Multi-threaded orchestration: Update and flush with worker threads
    void UpdateAndFlush(const SpriteData* allSprites, int totalCount);

    // Multi-threaded batch rendering: Process sprite data array with worker threads
    void RenderBatch(const SpriteData* pData, int count);

    // Atlas-based rendering: Draw sprite by name (for convenience)
    void DrawSprite(SpriteAtlas* atlas, const char* spriteName, float x, float y, DWORD color = 0xFFFFFFFF);
   

    // Atlas-based rendering: Draw sprite by index (optimized for per-frame use)
    void DrawIndexedSprite(SpriteAtlas* atlas, uint32_t spriteIndex, float x, float y, DWORD color = 0xFFFFFFFF);

    // Atlas-based rendering: Draw sprite by index (optimized for per-frame use)
    void DrawAtlasSprite(SpriteAtlas* atlas, uint32_t index, float x, float y, DWORD color = 0xFFFFFFFF);

    // Batch rendering with automatic sorting for Xbox 360
    void Submit(const char* shader, SpriteAtlas* atlas, uint32_t spriteIdx, float x, float y, float depth, DWORD color);
    void ExecuteBatch();

    // Hardware instancing for Xbox 360 (maximum performance)
    void SubmitInstanced(const char* shader, SpriteAtlas* atlas, uint32_t spriteIdx, float x, float y, float depth, DWORD color);
    void FlushInstanced();

    // Strategy pattern: rendering mode switching
    void SetRenderMode(RenderMode mode) { 
        m_currentMode = mode; 
        switch(mode) {
            case MODE_STANDARD:         m_currentShaderName = "sprite"; break;
            case MODE_INSTANCED_STREAM:   m_currentShaderName = "sprite_instanced"; break;
            case MODE_INSTANCED_CONST:    m_currentShaderName = "sprite_constant_instanced"; break;
        }
    }
    RenderMode GetRenderMode() const { return m_currentMode; }

    // Constant buffer instancing for 4096+ sprites (Register Instancing)
    void FlushConstantInstanced();

    // Hybrid rendering: automatic path switching based on sprite count
    void FlushStandard();  // For small batches (< 128 sprites)
    void FlushConstantInstancedHybrid();  // For large batches (>= 128 sprites)

    // Initialize worker threads for Xbox 360 multi-threading
    HRESULT InitializeWorkerThreads();

    // Shutdown worker threads
    void ShutdownWorkerThreads();

 private:
    void CreateQuad(float x, float y, float width, float height,
                    float u0, float v0, float u1, float v1,
                    DWORD color);

    void CreateQuadRotated(float x, float y, float width, float height, float angle,
                          float u0, float v0, float u1, float v1,
                          DWORD color);

    void ComputeRotatedVerticesVMX(const SpriteData& data, SpriteVertex* pOutVertices);

    void InternalDraw(const RenderCommand& cmd);

    LPDIRECT3DDEVICE9 m_pDevice;
    ShaderManager* m_pShaderManager;
    LPDIRECT3DVERTEXBUFFER9 m_pVB[2]; // Double buffering for Xbox 360
    int m_activeBuffer;               // Current buffer index (0 or 1)
    LPDIRECT3DINDEXBUFFER9 m_pIndexBuffer;
    LPDIRECT3DVERTEXDECLARATION9 m_pVertexDecl;

    // Hardware instancing buffers for Xbox 360
    LPDIRECT3DVERTEXBUFFER9 m_pStaticQuadVB;          // 4 vertices (0,0 to 1,1)
    LPDIRECT3DINDEXBUFFER9 m_pStaticQuadIB;          // 6 indices for quad
    LPDIRECT3DVERTEXBUFFER9 m_pInstanceVB[2];        // Instance data with double buffering
    LPDIRECT3DVERTEXDECLARATION9 m_pInstancedDecl;   // Two-stream declaration
    LPDIRECT3DVERTEXDECLARATION9 m_pConstantInstancedDecl; // POSITION-only for constant buffer instancing

    // Staging buffer in system memory (CPU-side), aligned for VMX
    // Used for multi-threaded rendering on Xbox 360
    __declspec(align(16)) SpriteVertex* m_pStagingBuffer;

    // CPU mirror for vertex data (legacy, kept for compatibility)
    std::vector<SpriteVertex> m_vertices;

    // Render queue for batch sorting (Xbox 360 optimization)
    std::vector<RenderCommand> m_renderQueue;

    // Instance queue for hardware instancing (Xbox 360 maximum performance)
    std::vector<SpriteInstance> m_instanceQueue;

    // State tracking
    RenderMode m_currentMode;
    std::string m_currentShaderName;  // Shader name for current mode (replaces m_currentShader)
    LPDIRECT3DTEXTURE9 m_currentTexture;
    bool m_isBatching;
    float m_currentZOrder; // Current Z-order for batch

    // Configuration
    int m_maxSprites;
    int m_vertexBufferSize; // in bytes
    int m_indexBufferSize; // in bytes
    int m_spriteCount;

    // Thread synchronization objects for Xbox 360
    HANDLE m_hStartWorkEvent;        // Event to start worker threads
    HANDLE m_hThreadDoneEvents[2];   // Events for thread completion (Core 1, Core 2)
    HANDLE m_hWorkerThreads[2];      // Worker thread handles
    ThreadData m_workerData[2];      // Data for worker threads
    bool m_threadsInitialized;
};
