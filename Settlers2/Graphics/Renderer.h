#pragma once
#include "ShaderManager.h"
#include "GPUTimer.h"
#include "RenderFrame.h"
#include <d3d9.h>
#include <d3dx9.h>
#include "RenderTypes.h"

static_assert(sizeof(SpriteVertex) == 32, "SpriteVertex must be 32 bytes");

class Texture;
namespace Graphics { class ShaderManager; }
using Graphics::ShaderManager;
using Graphics::SpriteRenderer;
using Graphics::RenderFrame;
using Graphics::GPUTimer;

class Renderer {
public:
    Renderer();
    ~Renderer();

    HRESULT Initialize();
    void Shutdown();
    void BeginFrame();
    void EndFrame();
    void EndSceneOnly();
    void Clear(D3DCOLOR color);

    void OnLostDevice();
    void OnResetDevice();

    LPDIRECT3DDEVICE9 GetDevice() const { return m_pDevice; }
    D3DPRESENT_PARAMETERS* GetD3DPP() { return &m_d3dpp; }
    int GetScreenWidth() const { return m_d3dpp.BackBufferWidth; }
    int GetScreenHeight() const { return m_d3dpp.BackBufferHeight; }
    float* GetProjectionMatrix() { return m_projMatrix; }
    LPDIRECT3DVERTEXDECLARATION9 GetVertexDecl() const { return m_pVertexDecl; }

    ShaderManager* GetShaderManager() { return m_pShaderManager; }
    void SetShaderManager(ShaderManager* pShaderManager) { m_pShaderManager = pShaderManager; }

    SpriteRenderer* GetSpriteRenderer() { return m_pSpriteRenderer; }
    void SetSpriteRenderer(SpriteRenderer* pSpriteRenderer);
    RenderFrame* GetRenderFrame() { return m_pRenderFrame; }
    void SetRenderFrame(RenderFrame* frame) { m_pRenderFrame = frame; }
    HRESULT LoadShader(ShaderID id, const char* filepath, const char* techniqueName = "SpriteBatchTech");
    bool SetShader(ShaderID id);
    void ResetToDefaultShader();

    void PrepareForUI();
    void Setup2DRenderStates();

private:
    void SetProjectionMatrix(float width, float height);

    LPDIRECT3D9         m_pD3D;
    LPDIRECT3DDEVICE9   m_pDevice;
    LPDIRECT3DSURFACE9  m_pBackBuffer;
    D3DPRESENT_PARAMETERS m_d3dpp;

    ShaderManager* m_pShaderManager;
    SpriteRenderer* m_pSpriteRenderer;
    RenderFrame* m_pRenderFrame;
    GPUTimer* m_pGPUTimer;

    LPDIRECT3DVERTEXSHADER9 m_pVertexShader;
    LPDIRECT3DPIXELSHADER9 m_pPixelShader;

    float m_projMatrix[16];
    LPDIRECT3DVERTEXDECLARATION9 m_pVertexDecl;
};
