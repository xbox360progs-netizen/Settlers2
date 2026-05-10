#include "stdafx.h"
#include "Renderer.h"
#include "Texture.h"
#include <stdio.h>
#include <d3dx9.h>

Renderer::Renderer()  
    : m_pD3D(NULL), m_pDevice(NULL), m_pBackBuffer(NULL),
      m_pVertexDecl(NULL), m_pVertexShader(NULL), m_pPixelShader(NULL) {
    ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));
    ZeroMemory(m_projMatrix, sizeof(m_projMatrix));
}

Renderer::~Renderer() {
    Shutdown();
}

HRESULT Renderer::Initialize() {
    m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!m_pD3D) { OutputDebugStringA("D3D Create Failed!\n"); return E_FAIL; }

    m_d3dpp.BackBufferWidth = 1280;
    m_d3dpp.BackBufferHeight = 720;
    m_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    m_d3dpp.BackBufferCount = 1;
    m_d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
    m_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    m_d3dpp.EnableAutoDepthStencil = FALSE;
    // Disable VSync for maximum performance - force IMMEDIATE
    // The Present interval must be set BEFORE CreateDevice
    // DO NOT ZeroMemory here - it resets all settings!
    OutputDebugStringA("[R] Setting IMMEDIATE mode\n");
    
    // Force settings for Xbox 360 performance
    m_d3dpp.BackBufferWidth = 1280;
    m_d3dpp.BackBufferHeight = 720;
    m_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    m_d3dpp.BackBufferCount = 1;
    m_d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
    m_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    m_d3dpp.EnableAutoDepthStencil = FALSE;
    m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE; // No VSync for max FPS
    
    DWORD flags = D3DCREATE_HARDWARE_VERTEXPROCESSING;

    // Xbox 360: ensure no VSync in the created device
    HRESULT hr = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL, flags, &m_d3dpp, &m_pDevice);
    if (FAILED(hr)) { OutputDebugStringA("Device Create Failed!\n"); return hr; }
    
    // Double-check: SetRenderState to ensure no sync
    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

    // Initialize shader manager
    hr = m_shaderManager.Initialize(m_pDevice);
    if (FAILED(hr)) { OutputDebugStringA("ShaderManager Initialize Failed!\n"); return hr; }

    // Create vertex declaration for VFETCH (32-byte aligned for Xbox 360)
    D3DVERTEXELEMENT9 decl[] = {
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
        { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 }, // Padding for 32-byte alignment
        D3DDECL_END()
    };
    HRESULT hrDecl = m_pDevice->CreateVertexDeclaration(decl, &m_pVertexDecl);
    if (FAILED(hrDecl)) {
        OutputDebugStringA("FAILED to create vertex declaration!\n");
    } else {
        OutputDebugStringA("Vertex declaration created OK\n");
    }

    // Load default sprite shader
//	hr = LoadShader("Basic2D", "game:\\Media\\Shaders\\Basic2D.fx", "Basic2D");
//    if (FAILED(hr)) {
//        OutputDebugStringA("Failed to load default Basic2D shader\n");
//    }

    hr = LoadShader("sprite", "game:\\Media\\Shaders\\SpriteShader.fx", "SpriteBatchTech");
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to load default sprite shader\n");
    }

    // Load instanced sprite shaders for maximum performance (4096+ sprites)
    hr = LoadShader("sprite_instanced", "game:\\Media\\Shaders\\SpriteInstanced.fx", "SpriteInstancedTech");
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to load instanced sprite shader\n");
    }

    hr = LoadShader("sprite_constant_instanced", "game:\\Media\\Shaders\\SpriteConstantInstanced.fx", "SpriteConstantInstancedTech");
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to load constant instanced sprite shader\n");
    }

    SetProjectionMatrix(1280.0f, 720.0f);

    m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

    m_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    m_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    return S_OK;
}

HRESULT Renderer::LoadShader(const char* name, const char* filepath, const char* techniqueName) {
    return m_shaderManager.LoadShader(name, filepath, techniqueName);
}

bool Renderer::SetShader(const char* name) {
    return m_shaderManager.SetActiveShader(name);
}

void Renderer::Shutdown() {
    m_shaderManager.Shutdown();
    
    if (m_pVertexShader) { m_pVertexShader->Release(); m_pVertexShader = NULL; }
    if (m_pPixelShader) { m_pPixelShader->Release(); m_pPixelShader = NULL; }
    if (m_pVertexDecl) { m_pVertexDecl->Release(); m_pVertexDecl = NULL; }
    if (m_pBackBuffer) { m_pBackBuffer->Release(); m_pBackBuffer = NULL; }
    if (m_pDevice) { m_pDevice->Release(); m_pDevice = NULL; }
    if (m_pD3D) { m_pD3D->Release(); m_pD3D = NULL; }
}

void Renderer::BeginFrame() {
    if (!m_pDevice) return;
    
    m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);
    m_pDevice->BeginScene();
    
    // Set projection matrix on the sprite shader (but don't BeginShader/BeginPass here)
    // SpriteRenderer manages its own shader state in FlushStandard
    ShaderManager::Shader* pSpriteShader = m_shaderManager.GetShader("sprite");
    if (pSpriteShader && pSpriteShader->pEffect && pSpriteShader->hMatOrtho) {
        pSpriteShader->pEffect->SetMatrix(pSpriteShader->hMatOrtho, (D3DXMATRIX*)&m_projMatrix);
    }
    
}


void Renderer::EndFrame() {
    if (!m_pDevice) return;

    m_pDevice->EndScene();

    HRESULT hr = m_pDevice->Present(NULL, NULL, NULL, NULL);
    if (FAILED(hr)) {
        if (hr == D3DERR_DEVICELOST) {
            OutputDebugStringA("[R] Device lost during Present!\n");
            OnLostDevice();
            Sleep(100);
            hr = m_pDevice->Reset(&m_d3dpp);
            if (SUCCEEDED(hr)) {
                OutputDebugStringA("[R] Device reset successful\n");
                OnResetDevice();
            } else {
                OutputDebugStringA("[R] Device reset failed!\n");
            }
        }
    }

}

void Renderer::Clear(D3DCOLOR color) {
    if (m_pDevice) m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET, color, 1.0f, 0);
}

void Renderer::OnLostDevice() {

    // Tell ShaderManager to release effect internal states
    m_shaderManager.OnLostDevice();
}

void Renderer::OnResetDevice() {

    // Restore ShaderManager (reloads constants and techniques)
    m_shaderManager.OnResetDevice();

    // Restore global device states (since Reset() wipes them)
    m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    // Restore sampler states
    m_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    m_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    // Update the Projection Matrix for the new state
    SetProjectionMatrix(1280.0f, 720.0f);

}

void Renderer::SetProjectionMatrix(float width, float height) {
    D3DXMatrixOrthoOffCenterLH((D3DXMATRIX*)m_projMatrix, 0.0f, width, height, 0.0f, 0.0f, 1.0f);
}

void Renderer::DrawSingleSprite(Texture* texture, float x, float y, float width, float height, D3DCOLOR color) {
    DrawSingleSprite(texture, x, y, width, height, 0.0f, 0.0f, 1.0f, 1.0f, color);
}

void Renderer::DrawSingleSprite(Texture* texture, float x, float y, float width, float height,
                                float u0, float v0, float u1, float v1, D3DCOLOR color) {
    if (!m_pDevice || !texture || !texture->GetTexture()) return;

    static_assert(sizeof(SpriteVertex) == 32, "SpriteVertex size mismatch");

    SpriteVertex vertices[4];

    vertices[0].x = x; vertices[0].y = y; vertices[0].z = 0.0f;
    vertices[0].color = color; vertices[0].u = u0; vertices[0].v = v0;
    vertices[0].padding[0] = 0; vertices[0].padding[1] = 0;

    vertices[1].x = x + width; vertices[1].y = y; vertices[1].z = 0.0f;
    vertices[1].color = color; vertices[1].u = u1; vertices[1].v = v0;
    vertices[1].padding[0] = 0; vertices[1].padding[1] = 0;

    vertices[2].x = x + width; vertices[2].y = y + height; vertices[2].z = 0.0f;
    vertices[2].color = color; vertices[2].u = u1; vertices[2].v = v1;
    vertices[2].padding[0] = 0; vertices[2].padding[1] = 0;

    vertices[3].x = x; vertices[3].y = y + height; vertices[3].z = 0.0f;
    vertices[3].color = color; vertices[3].u = u0; vertices[3].v = v1;
    vertices[3].padding[0] = 0; vertices[3].padding[1] = 0;

    ShaderManager::Shader* pShader = m_shaderManager.GetShader("sprite");
    if (pShader && pShader->pEffect && m_pVertexDecl) {
        m_shaderManager.SetActiveShader("sprite");
        m_shaderManager.SetTexture("g_texture", texture->GetTexture());
        m_shaderManager.BeginShader();
        m_shaderManager.BeginPass(0);
        pShader->pEffect->CommitChanges();
        m_pDevice->SetTexture(0, texture->GetTexture());

        m_pDevice->SetVertexDeclaration(m_pVertexDecl);
        m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vertices, sizeof(SpriteVertex));

        m_shaderManager.EndPass();
        m_shaderManager.EndShader();
    } else {
        OutputDebugStringA("[Renderer] DrawSingleSprite: shader or vertex decl is NULL\n");
    }
}
