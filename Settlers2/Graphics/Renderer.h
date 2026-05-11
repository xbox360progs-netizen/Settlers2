#pragma once
#include "ShaderManager.h"
#include <d3d9.h>
#include <d3dx9.h>

struct SpriteVertex {
    float x, y, z;
    DWORD color;
    float u, v;
    float padding[2]; // 32-byte alignment for Xbox 360
};
static_assert(sizeof(SpriteVertex) == 32, "SpriteVertex must be 32 bytes");

class Texture;

class Renderer {
public:
    Renderer();
    ~Renderer();

    HRESULT Initialize();
    void Shutdown();
    void BeginFrame();
    void EndFrame();
    void Clear(D3DCOLOR color);

    // Xbox 360 device loss handling
    void OnLostDevice();
    void OnResetDevice();

    LPDIRECT3DDEVICE9 GetDevice() const { return m_pDevice; }
    D3DPRESENT_PARAMETERS* GetD3DPP() { return &m_d3dpp; }
    int GetScreenWidth() const { return m_d3dpp.BackBufferWidth; }
    int GetScreenHeight() const { return m_d3dpp.BackBufferHeight; }
    float* GetProjectionMatrix() { return m_projMatrix; }
    LPDIRECT3DVERTEXDECLARATION9 GetVertexDecl() const { return m_pVertexDecl; }

    // Shader management
    ShaderManager* GetShaderManager() { return &m_shaderManager; }
    HRESULT LoadShader(ShaderID id, const char* filepath, const char* techniqueName = "SpriteBatchTech");
    bool SetShader(ShaderID id);
    void ResetToDefaultShader();
    // Prepare render states for UI rendering
    void PrepareForUI();

    // Restore render states after UI rendering (for map rendering)
    void RestoreFromUI();

    // Setup 2D render states (preset to prevent 3D world states from breaking UI)
    void Setup2DRenderStates();

    // Simple sprite rendering (for testing/diagnostics - no batching)
    void DrawSingleSprite(Texture* texture, float x, float y, float width, float height, D3DCOLOR color = 0xFFFFFFFF);
    void DrawSingleSprite(Texture* texture, float x, float y, float width, float height,
                          float u0, float v0, float u1, float v1, D3DCOLOR color = 0xFFFFFFFF);

private:
    void SetProjectionMatrix(float width, float height);

    LPDIRECT3D9         m_pD3D;
    LPDIRECT3DDEVICE9   m_pDevice;
    LPDIRECT3DSURFACE9  m_pBackBuffer;
    D3DPRESENT_PARAMETERS m_d3dpp;

    ShaderManager m_shaderManager;

    LPDIRECT3DVERTEXSHADER9 m_pVertexShader;
    LPDIRECT3DPIXELSHADER9  m_pPixelShader;

    float m_projMatrix[16];
    LPDIRECT3DVERTEXDECLARATION9 m_pVertexDecl;
};
