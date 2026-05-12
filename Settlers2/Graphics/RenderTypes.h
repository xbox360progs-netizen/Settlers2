#pragma once
#include <d3d9.h>

// Forward declarations
class ShaderManager;

// Custom draw callback type for non-standard rendering (e.g. RadialMenu shader)
typedef void (*CustomDrawFn)(LPDIRECT3DDEVICE9 pDevice, ShaderManager* pShaderMgr, void* pUserData);

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

struct RenderCommand {
    LPDIRECT3DTEXTURE9 pTexture;
    int shaderID;   // Shader handle (ShaderHandle enum) instead of raw pointer
    int vertexStart;
    int vertexCount;
    int primitiveCount;
    int batchType; // 0 - Standard (Single), 1 - Instanced, 2 - Custom callback
    float depth;   // Z-layer: 1.0=far (ground), 0.5=mid (units), 0.1=near (UI)
    int layer;     // Logical layer: 0=Terrain, 1=Objects, 2=UI (for composite depth)
    bool isUI;     // Screen-space rendering (skip camera matrix)
    RenderStateBlock states;
    int batchIndex; // For tracking batch sequence (for vertex offset calculation)
    
    // World position for camera transform (if not isUI)
    float worldX, worldY;
    
    // UV coordinates for partial texture rendering (text, sprites)
    float u0, v0, u1, v1; // Texture region to sample
    
    // Screen position for UI rendering (if isUI)
    float screenX, screenY, screenW, screenH;
    
    // Color tint for the render command
    DWORD color;
    
    // Custom draw callback (for batchType == 2)
    CustomDrawFn customDraw;
    void* customUserData;
    
    RenderCommand() : pTexture(NULL), shaderID(-1), vertexStart(0), vertexCount(0),
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
