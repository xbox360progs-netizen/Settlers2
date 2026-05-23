#include "stdafx.h"
#include "Renderer.h"
#include "Texture.h"
#include "SpriteRenderer.h"
#include "RenderFrame.h"
#include "RenderQueue.h"
#include "GPUTimer.h"
#include <stdio.h>
#include <d3dx9.h>

Renderer::Renderer()
    : m_pD3D(NULL), m_pDevice(NULL), m_pBackBuffer(NULL),
      m_pVertexDecl(NULL), m_pVertexShader(NULL), m_pPixelShader(NULL),
      m_pShaderManager(NULL), m_pSpriteRenderer(NULL), m_pRenderFrame(NULL), m_pGPUTimer(NULL),
      m_pRenderQueue(NULL) {
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
    m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    DWORD flags = D3DCREATE_HARDWARE_VERTEXPROCESSING;

    HRESULT hr = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL, flags, &m_d3dpp, &m_pDevice);
    if (FAILED(hr)) { OutputDebugStringA("Device Create Failed!\n"); return hr; }

    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

    if (!m_pShaderManager) {
        OutputDebugStringA("[Renderer] WARNING: m_pShaderManager is NULL - call SetShaderManager() before Initialize()\n");
    }

    D3DVERTEXELEMENT9 decl[] = {
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        { 0, 20, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
        { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
        D3DDECL_END()
    };
    HRESULT hrDecl = m_pDevice->CreateVertexDeclaration(decl, &m_pVertexDecl);
    if (FAILED(hrDecl)) {
        OutputDebugStringA("[Renderer] ERROR: CreateVertexDeclaration failed!\n");
        char errorMsg[256];
        sprintf_s(errorMsg, "[Renderer] CreateVertexDeclaration failed with HRESULT: 0x%08x\n", hrDecl);
        OutputDebugStringA(errorMsg);
    } else {
        OutputDebugStringA("Vertex declaration created OK\n");
    }

    SetProjectionMatrix(1280.0f, 720.0f);

    hr = m_pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer);
    if (FAILED(hr)) {
        OutputDebugStringA("[Renderer] ERROR: GetBackBuffer failed!\n");
        return hr;
    }

    m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

    m_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    m_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    m_pGPUTimer = new GPUTimer();
    if (m_pGPUTimer) {
        m_pGPUTimer->Initialize(m_pDevice);
    }

    m_pRenderQueue = new RenderQueue();
    OutputDebugStringA("[Renderer] RenderQueue created\n");

    m_pRenderFrame = new RenderFrame();
    if (m_pRenderFrame) {
        m_pRenderFrame->Initialize(m_pDevice);
        m_pRenderFrame->SetRenderQueue(m_pRenderQueue);
        m_pRenderFrame->SetSpriteRenderer(m_pSpriteRenderer);
        m_pRenderFrame->SetGPUTimer(m_pGPUTimer);
        OutputDebugStringA("[Renderer] RenderFrame initialized\n");
    }

    return S_OK;
}

void Renderer::SetSpriteRenderer(SpriteRenderer* pSpriteRenderer) {
    m_pSpriteRenderer = pSpriteRenderer;
    if (m_pSpriteRenderer) {
        m_pSpriteRenderer->SetProjectionMatrix(m_projMatrix);
    }
    if (m_pRenderFrame) {
        m_pRenderFrame->SetSpriteRenderer(m_pSpriteRenderer);
    }
    char buf[256];
    sprintf(buf, "[Renderer] SetSpriteRenderer: %p, RenderFrame spriteRenderer=%p\n",
            pSpriteRenderer, m_pRenderFrame ? m_pRenderFrame->GetSpriteRenderer() : NULL);
    OutputDebugStringA(buf);
}

HRESULT Renderer::LoadShader(ShaderID id, const char* filepath, const char* techniqueName) {
    if (!m_pShaderManager) return E_FAIL;
    return m_pShaderManager->LoadShader(id, filepath, techniqueName);
}

bool Renderer::SetShader(ShaderID id) {
    if (!m_pShaderManager) return false;
    return m_pShaderManager->SetActiveShader(id);
}

void Renderer::ResetToDefaultShader() {
    if (m_pShaderManager) {
        m_pShaderManager->SetActiveShader(SHADER_SPRITE_CONSTANT_INSTANCED);
    }
}

void Renderer::Setup2DRenderStates() {
    if (!m_pDevice) return;

    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
}

void Renderer::PrepareForUI() {
    if (!m_pDevice) return;

    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
}

void Renderer::Shutdown() {
    if (m_pRenderQueue) {
        delete m_pRenderQueue;
        m_pRenderQueue = nullptr;
    }
    if (m_pRenderFrame) {
        m_pRenderFrame->Shutdown();
        delete m_pRenderFrame;
        m_pRenderFrame = nullptr;
    }
    if (m_pGPUTimer) {
        m_pGPUTimer->Shutdown();
        delete m_pGPUTimer;
        m_pGPUTimer = nullptr;
    }

    if (m_pVertexShader) { m_pVertexShader->Release(); m_pVertexShader = NULL; }
    if (m_pPixelShader) { m_pPixelShader->Release(); m_pPixelShader = NULL; }
    if (m_pVertexDecl) { m_pVertexDecl->Release(); m_pVertexDecl = NULL; }
    if (m_pBackBuffer) { m_pBackBuffer->Release(); m_pBackBuffer = NULL; }
    if (m_pDevice) { m_pDevice->Release(); m_pDevice = NULL; }
    if (m_pD3D) { m_pD3D->Release(); m_pD3D = NULL; }
}

void Renderer::BeginFrame() {
    if (!m_pDevice) return;
    m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    m_pDevice->BeginScene();
}

void Renderer::EndSceneOnly() {
    if (!m_pDevice) return;
    m_pDevice->EndScene();
}

void Renderer::EndFrame() {
    if (!m_pDevice) return;
    m_pDevice->EndScene();
    m_pDevice->Present(NULL, NULL, NULL, NULL);
}

void Renderer::Clear(D3DCOLOR color) {
    if (m_pDevice) m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 1.0f, 0);
}

void Renderer::OnLostDevice() {
    if (m_pShaderManager) {
        m_pShaderManager->OnLostDevice();
    }
}

void Renderer::OnResetDevice() {
    if (m_pShaderManager) {
        m_pShaderManager->OnResetDevice();
    }

    m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    m_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    m_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    SetProjectionMatrix(1280.0f, 720.0f);

    HRESULT hr;
    hr = m_pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer);
    if (FAILED(hr)) {
        OutputDebugStringA("[Renderer] ERROR: GetBackBuffer failed!\n");
        return;
    }
}

void Renderer::SetProjectionMatrix(float width, float height) {
    D3DXMatrixOrthoOffCenterLH((D3DXMATRIX*)m_projMatrix, 0.0f, width, height, 0.0f, 0.0f, 1.0f);
    if (m_pSpriteRenderer) {
        m_pSpriteRenderer->SetProjectionMatrix(m_projMatrix);
    }
}
